#include <iostream>
#include "Bytes.h"
#include "Str.h"
#include "MqttIn.h"
#include "MqttOut.h"
#include <stdio.h>
#include <time.h>
#include "Queue.h"
#include "Event.h"
#include "Thread.h"
#include "Stream.h"
#include <iostream>
#include <list>

#include "Timer.h"
//#include "Publisher.h"
#include "CircBuf.h"
#include "Mqtt.h"
#include "Tcp.h"
#include "Property.h"
#include "Strpack.h"
#include "Logger.h"

#define TIME_PING   5000
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 100
#define TIME_BETWEEN_PROPERTIES 100
#define MAX_MSG_SIZE    256
#define SLEEP(xxx)  timeout(xxx); \
    PT_YIELD_UNTIL(&t,timeout());

EventId TIMER_TICK = Event::nextEventId ( ( char* const ) "TIMER_TICK" );
EventId MQTT_CONNECTED = Event::nextEventId ( ( char* const ) "MQTT_CONNECTED" );
EventId MQTT_DISCONNECTED = Event::nextEventId ( ( char* const ) "MQTT_DISCONNECTED" );
EventId PROP_CHANGED = Event::nextEventId ( ( char* const ) "PROP_CHANGED" );

Str prefix ( 30 );
Str putPrefix ( 30 );
Str getPrefix ( 30 );
class Mqtt;
Mqtt* mqtt;

typedef enum { CMD_GET, CMD_DESC, CMD_PUT } Cmd;
typedef void( *Xdr )( void*, Cmd ,  Strpack& );


class TopicObject {
        virtual void doNothing() {};
    };
typedef  void ( TopicObject::*TopicMethod ) ( Strpack& );

class Prop;
#define PROPS_MAX   20
Prop* propList[PROPS_MAX];
uint32_t propListCount = 0;

class  Prop {
    public :
        const char* _name;
        void* _instance;
        Xdr _xdr;
        Flags _flags;
    public :
        Prop ( const char* name, void* instance, Xdr xdr,
               Flags flags ) {
            _name = name;
            _instance = instance;
            _xdr = xdr;
            _flags = flags;
            propList[propListCount++] = this;
            _flags.publishValue = true;
            _flags.publishMeta = true;
            }
        Prop ( TopicObject* instance ) {
            _instance = instance;
            }
        static Prop* findProp ( Str& name ) {
            uint32_t i;
            for ( i = 0; i < propListCount; i++ ) {
                if ( propList[i] )
                    if ( name.equals( propList[i]->_name ) ) {
                        return  propList[i];
                        }
                }
            return 0;
            }
        static void set( Str& topic, Strpack& message, uint8_t header ) {
            Str str( 30 );
            str.substr( topic, getPrefix.length() );
            Prop* p = findProp( str );
            if ( p ) {
                if ( p->_xdr )
                    p->_xdr( p->_instance, CMD_PUT, message );
//                    ( ( *( p->_instance ) ).* ( p->_setter ) ) ( message );
                p->_flags.publishValue = true;
                }
            Sequence::publish( 0, PROP_CHANGED, 0 );
            }
    };



class TimerThread : public Thread, public Sequence {

    public:

        TimerThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority ) {
            Sys::upTime();
            };
        void run() {
            struct timespec deadline;
            clock_gettime ( CLOCK_MONOTONIC, &deadline );
            Sys::_upTime = deadline.tv_sec * 1000 + deadline.tv_nsec / 1000000;
            while ( true ) {
                deadline.tv_nsec += 1000000000; // Add the time you want to sleep, 1 sec now
                if ( deadline.tv_nsec >= 1000000000 ) { // Normalize the time to account for the second boundary
                    deadline.tv_nsec -= 1000000000;
                    deadline.tv_sec++;
                    }
                clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL );
                Sys::_upTime = deadline.tv_sec * 1000 + deadline.tv_nsec / 1000000;
                publish ( this, TIMER_TICK, 0 );
                }
            }
        int handler ( Event* ev ) {
            return 0;
            };
    };
//____________________________________________________________________________________________

#include "pt.h"

uint16_t gMessageId = 1;
MqttOut mqttOut ( MAX_MSG_SIZE );

uint16_t nextMessageId() {
    return gMessageId++;
    }

/*****************************************************************************
*   MQTT EVENT filter
******************************************************************************/

bool eventIsMqtt ( Event* event, uint8_t type, uint16_t messageId , uint8_t qos ) {
    if ( event->id() != Tcp::MQTT_MESSAGE ) return false;
    MqttIn* mqttIn;
    mqttIn = ( MqttIn* )( event->data() );
    if ( type )
        if ( mqttIn->type() != type ) return false;
    if ( messageId )
        if ( mqttIn->messageId() != messageId ) return false;
    if ( qos )
        if ( mqttIn->qos() != qos ) return false;
    return true;
    }


/*****************************************************************************
   HANDLE MQTT connection and subscribe sequence
******************************************************************************/

class Mqtt : public Sequence {
    private:
        struct pt t;
        Tcp* _tcp;
        uint16_t _messageId;
        bool _isConnected;
        Str str;
        Str msg;

    public :

        Mqtt ( Tcp* tcp ) : str ( 30 ), msg ( 10 ) {
            _tcp = tcp;
            PT_INIT ( &t );
            _messageId = 2000;
            _isConnected = false;
            };

        int handler ( Event* event ) {
            PT_BEGIN ( &t );
            while ( 1 ) {
                _messageId = nextMessageId();
                PT_YIELD_UNTIL ( &t, _tcp->isConnected() );
                mqttOut.Connect ( 0, "clientId", MQTT_CLEAN_SESSION,
                                  "system/online", "false", "", "", TIME_KEEP_ALIVE / 1000 );
                _tcp->send ( &mqttOut );
                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_CONNACK, 0, 0 ) || timeout() );
                if ( timeout() ) {
                    _tcp->disconnect();
                    publish ( this, MQTT_DISCONNECTED, 0 );
                    continue;
                    }



                ( str = putPrefix ) += "#" ;
                mqttOut.Subscribe ( 0, str, ++_messageId, MQTT_QOS1_FLAG );
                _tcp->send ( &mqttOut );

                ( str = getPrefix ) += "#" ;
                mqttOut.Subscribe ( 0, str, ++_messageId, MQTT_QOS1_FLAG );
                _tcp->send ( &mqttOut );

                str = "system/online";
                msg = "true";
                mqttOut.Publish ( MQTT_QOS1_FLAG, str, msg, ++_messageId );
                _tcp->send ( &mqttOut );


                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBACK, _messageId, 0 ) || timeout() );
                if ( timeout() ) {
                    _tcp->disconnect();
                    publish ( this, MQTT_DISCONNECTED, 0 );
                    continue;
                    }
                publish ( this, MQTT_CONNECTED, 0 );
                _isConnected = true;
                PT_YIELD_UNTIL ( &t, event->is ( _tcp, Tcp::TCP_DISCONNECTED ) );

                publish ( this, MQTT_DISCONNECTED, 0 );
                _isConnected = false;
                }
            PT_END ( &t );
            }

        Erc send ( Bytes& pb ) {
            if ( _tcp->isConnected() )
                return _tcp->send ( &pb );
            else return E_AGAIN;
            }
        bool isConnected() {
            return _isConnected;
            };
        Erc disconnect() {
            return _tcp->disconnect();
            }
    };
/*****************************************************************************
*   HANDLE PING keep alive
******************************************************************************/
class MqttPing : public Sequence {
    private:
        struct pt t;
        uint16_t _retryCount;
    public:
        MqttPing (  ) {
            PT_INIT ( &t );
            //TODO in destructor free memory
            }
        int handler ( Event* event ) {
            PT_BEGIN ( &t );
            while ( true ) {
                _retryCount = 0;
                while ( mqtt->isConnected() ) {
                    mqttOut.PingReq();
                    mqtt->send ( mqttOut );

                    timeout ( TIME_WAIT_REPLY );
                    PT_YIELD_UNTIL ( &t, timeout() || eventIsMqtt ( event, MQTT_MSG_PINGRESP, 0 , 0 ) );
                    if ( timeout() ) {
                        _retryCount++;
                        if (  _retryCount >= 3 ) {
                            mqtt->disconnect();
                            PT_RESTART ( &t );
                            }
                        }
                    else _retryCount = 0;
                    SLEEP ( TIME_PING );
//               timeout(TIME_PING);
//               PT_YIELD_UNTIL(&t,timeout() );   // sleep between pings
                    }
                PT_YIELD ( &t );
                }
            PT_END ( &t );
            }
    };


/*****************************************************************************
*   HANDLE MQTT send publish message with qos=2 and 1
******************************************************************************/

class MqttPubQos0 : public Sequence {
    private :

    public:

        MqttPubQos0 (  ) {
            }
        void send( Prop* prop ) {
            uint8_t header = 0;
            Str topic( prop->_name );
            Strpack message( 256 );
            if ( prop->_flags.retained ) header += MQTT_RETAIN_FLAG;
            prop->_xdr( prop->_instance, CMD_GET, message );
            mqttOut.Publish ( header, topic, message, nextMessageId() );
            mqtt->send ( mqttOut );
            }
        int handler ( Event* event ) {
            return PT_YIELDED;
            }
    };

class MqttPubQos1 : public Sequence {
    private:
        struct pt t;
        uint32_t _retryCount;
        uint16_t _messageId;
        Prop* prop;
        bool _isReady;
    public:
        MqttPubQos1 (  ) {
            PT_INIT ( &t );
            _isReady = true;
            }
        void send( Prop* p ) {
            _messageId = nextMessageId();
            prop = p;
            _isReady = false;
            }
        bool isReady() {
            return _isReady;
            }
        int handler ( Event* event ) {
            Str topic( 50 );
            Strpack message( 256 );
            uint8_t header = MQTT_QOS1_FLAG;
            PT_BEGIN ( &t );
            while ( true ) {
                PT_YIELD_UNTIL( &t, _isReady == false );
                _retryCount = 0;
                while ( _retryCount < 3 ) {
                    header = MQTT_QOS1_FLAG;
                    if ( prop->_flags.retained ) header += MQTT_RETAIN_FLAG;
                    if ( _retryCount ) header += MQTT_DUP_FLAG;

                    topic.set( prop->_name );
                    prop->_xdr( prop->_instance, CMD_GET, message );
                    mqttOut.Publish ( header, topic, message, _messageId );
                    mqtt->send ( mqttOut );

                    timeout ( TIME_WAIT_REPLY );
                    PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBACK, _messageId, 0 ) );
                    if ( timeout() )
                        _retryCount++;
                    else
                        break;
                    }
                _isReady = true;
                }
            PT_END ( &t );
            }

    };

class MqttPubQos2 : public Sequence {
    private:
        struct pt t;
        uint16_t _messageId;
        Prop* prop;
        bool _isReady;
        uint8_t _retryCount;
    public:
        MqttPubQos2 ( ) {
            PT_INIT ( &t );
            _isReady = true;
            }
        void send( Prop* p ) {
            _messageId = nextMessageId();
            prop = p;
            _isReady = false;
            }
        bool isReady() {
            return _isReady;
            }
        int handler ( Event* event ) {
            Str topic( 50 );
            Strpack message( 256 );
            uint8_t header = MQTT_QOS2_FLAG;
            PT_BEGIN ( &t );
            while( true ) {
                PT_YIELD_UNTIL( &t, _isReady == false );
                while( !_isReady ) {
                    _retryCount = 0;
                    while( _retryCount < 3 ) {
                        if ( prop->_flags.retained ) header += MQTT_RETAIN_FLAG;
                        if ( _retryCount ) header += MQTT_DUP_FLAG;

                        topic.set( prop->_name );
                        prop->_xdr( prop->_instance, CMD_GET, message );
                        mqttOut.Publish ( header, topic, message, _messageId );
                        mqtt->send ( mqttOut );

                        timeout ( TIME_WAIT_REPLY );
                        PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREC, _messageId, 0 ) );
                        if ( timeout() ) _retryCount++;
                        else break;
                        };
                    if ( _retryCount == 3 )  break;
                    _retryCount = 0;
                    while( _retryCount < 3 ) {
                        mqttOut.PubRel ( _messageId );
                        mqtt->send ( mqttOut );

                        timeout ( TIME_WAIT_REPLY );
                        PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBCOMP, _messageId, 0 ) );
                        if ( timeout() ) _retryCount++;
                        else break;
                        };
                    }
                _isReady = true;
                }
            PT_END ( &t );
            }

    };

MqttPubQos0 mqttPubQos0;
MqttPubQos1 mqttPubQos1;
MqttPubQos2 mqttPubQos2;

/*****************************************************************************
*   HANDLE Property scan for changes
******************************************************************************/

/*****************************************************************************
*   HANDLE MQTT received publish message with qos=2
******************************************************************************/
class MqttSubQos0 : public Sequence {
    private:
        struct pt t;
    public:
        MqttSubQos0() {
            PT_INIT ( &t );
            }
        int handler ( Event* event ) {
            PT_BEGIN ( &t );
            while ( true ) {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , 0 ) );
                MqttIn* mqttIn = ( MqttIn* )event->data();
                if ( mqttIn->qos() == 0 )
                    Prop::set ( *mqttIn->topic(), *mqttIn->message() , mqttIn->qos() ) ;
                }
            PT_END ( &t );
            }

    };
class MqttSubQos1 : public Sequence {
    private:
        struct pt t;
    public:
        MqttSubQos1() {
            PT_INIT ( &t );
            }
        int handler ( Event* event ) {
            PT_BEGIN ( &t );
            while ( true ) {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , MQTT_QOS1_FLAG ) );
                MqttIn* mqttIn = ( MqttIn* )event->data();
                mqttOut.PubAck ( mqttIn->messageId() ); // send PUBACK
                mqtt->send ( mqttOut );
                Prop::set ( *mqttIn->topic(), *mqttIn->message() , mqttIn->qos() ) ;

                }
            PT_END ( &t );
            }

    };

class MqttSubQos2 : public Sequence {
    private:
        struct pt t;
        uint16_t _messageId;
        MqttIn save;
        bool _isReady;
    public:
        MqttSubQos2 ( ) : save ( MAX_MSG_SIZE ) {
            _messageId = 0;
            _isReady = true;
            PT_INIT ( &t );
            }
        int handler ( Event* event ) {
            MqttIn* mqttIn;
            PT_BEGIN ( &t );
            while( true ) {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , MQTT_QOS2_FLAG ) );
                _isReady = false;
                mqttIn = ( MqttIn* )( event->data() );
                _messageId = mqttIn->messageId();

                mqttOut.PubRec ( _messageId );
                mqtt->send ( mqttOut );

                save.clone ( * ( ( MqttIn* ) ( event->data() ) ) ); // clone the message
                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREL, _messageId, 0 ) );
                if ( timeout() ) {
                    _isReady = true;
                    continue; // abandon and go for destruction
                    }

                mqttOut.PubComp ( _messageId );
                mqtt->send ( mqttOut );
                Prop::set ( *save.topic(), *save.message() , save.qos() ) ;
                _isReady = true;
                }
            PT_END ( &t );
            }

    };
/*****************************************************************************
*   HANDLE Property publishing
******************************************************************************/
#include <malloc.h>

uint32_t getAllocSize ( void ) {
    uint32_t memoryAllocated;
    struct mallinfo mi;

    mi = mallinfo();
    memoryAllocated =  mi.uordblks;
    return memoryAllocated;
    }
//____________________________________________________________________________________________
//
class MainThread : public Thread {
    private :

    public:
        MainThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority ) {
            };
        void run() {
            Str line ( 200 );
            while ( true ) {
                Event event;
                Queue::getDefaultQueue()->get ( &event ); // dispatch eventually IDLE message
                if ( event.id() != TIMER_TICK ) {
                    line.set ( " EVENT : " );
                    event.toString ( line );

                    Sys::getLogger().append ( line );
                    Sys::getLogger().flush();
                    line.clear();
                    std::cout << Sys::upTime() << " | EVENT " << event.getEventIdName() << std::endl;
                    }

                int i;
                for ( i = 0; i < MAX_SEQ; i++ )
                    if ( Sequence::activeSequence[i] ) {
                        if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED ) {
                            Sequence* seq = Sequence::activeSequence[i];
                            seq->unreg();
                            delete seq;
                            };
                        }
                if ( event.data()  )
                    delete  ( MqttIn* )event.data();
                }
            }
    };
//__________________________________________________________________________________
//
//__________________________________________________________________________________
//

#define BOARD RASPBERRY_PI
#include "gnublin.h"
#include "Strpack.h"
gnublin_gpio gpio;

class Gpio : public TopicObject {
    private:
        int _pin;
        char _dir;
        char _value;
    public:
        Gpio ( int pin ) {
            _pin = pin;
            _dir='I';
            gpio.pinMode ( _pin, INPUT );
            }

        static void mode( void *instance, Cmd cmd, Strpack& strp ) {
            static const char* const desc = "desc:'mode for pin',set:['I','O']" ;
            Gpio* pin = ( Gpio* )instance;
            if ( cmd == CMD_PUT ) {
                char mode = strp.peek( 0 );
                if ( mode == 'O' ) {
                    pin->_dir='O';
                    gpio.pinMode ( pin->_pin, OUTPUT );
                    }
                else if ( mode == 'I' ) {
                    pin->_dir='I';
                    gpio.pinMode ( pin->_pin, INPUT );
                    }
                }
            else if ( cmd == CMD_GET ) {
                strp.append(pin->_dir);
                }
            else if ( cmd == CMD_DESC ) {
                strp.append( desc );
                }
            }
        static void input( void *instance, Cmd cmd, Strpack& strp ) {
            static const char* const desc = "desc:'input for pin',set:['1','0']" ;
            Gpio* pin = ( Gpio* )instance;
            if ( cmd == CMD_PUT ) {
                // cannot be set
                }
            else if ( cmd == CMD_GET ) {
                strp.append(gpio.digitalRead(pin->_pin));
                }
            else if ( cmd == CMD_DESC ) {
                strp.append( desc );
                }
            }
        static void output( void *instance, Cmd cmd, Strpack& strp ) {
            static const char* const desc = "desc:'output for pin',set:['1','0']" ;
            Gpio* pin = ( Gpio* )instance;
            if ( cmd == CMD_PUT ) {
                if (strp.length()!=1) return;
                char out=strp.peek(0);
                if ( out == '1') {
                    gpio.digitalWrite(pin->_pin,HIGH);
                    }
                else if ( out=='0') {
                    gpio.digitalWrite(pin->_pin,HIGH);
                    }
                }
            else if ( cmd == CMD_GET ) {
                // cannot be read
                }
            else if ( cmd == CMD_DESC ) {
                strp.append(desc);
                }
            }

        static void getInput( void *th, Strpack& value ) {
            Gpio* pin = ( ( Gpio* )th );
            value.append( "01"[gpio.digitalRead ( pin->_pin ) ] );
            }

        static void setOutput( void *th, Strpack& value ) {
            Gpio* pin = ( ( Gpio* )th );
            char o = value.peek ( 0 ) ;

            if ( o == '1' ) {
                pin->_value = '1';
                gpio.digitalWrite ( pin->_pin, HIGH );
                }
            else if ( o == '0' ) {
                pin->_value = '1';
                gpio.digitalWrite ( pin->_pin, LOW );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        char getInput() {
            return "01"[gpio.digitalRead ( _pin ) ];
            }


    };

class SysObject : public TopicObject {
    private:
        Prop* memoryProp;
        static Str logLine;
    public:
        SysObject() {
            memoryProp = new Prop ( "sys/heapMemory"
                                    , this, memory
            ,  ( Flags ) {
                T_UINT32, M_READ, QOS_1, I_OBJECT, true
                } );
            }
        static void memory ( void *instance, Cmd cmd, Strpack& value ) {
            static const char* const desc = " desc:'heapMemory'";
            if ( cmd == CMD_PUT ) {
                }
            else if ( cmd == CMD_GET ) {
                value.append ( getAllocSize() );
                }
            else if ( cmd == CMD_DESC ) {
                value.append( desc );
                }
            }
        static void log(void *instance,Cmd cmd,Strpack& value){
            static const char* const desc = " desc:'logMessage'";
            if ( cmd == CMD_PUT ) {
                }
            else if ( cmd == CMD_GET ) {
                value.append ( SysObject::logLine );
                }
            else if ( cmd == CMD_DESC ) {
                value.append( desc );
                }
        }
        static void log(const char* line){
            logLine.set(line);
            Sequence::publish(0,PROP_CHANGED,0);
        }
    };
Str SysObject::logLine(100);

SysObject sys;
Gpio pin_1 ( 1 );
Gpio pin_2 ( 2 );
Gpio pin_3 ( 3 );

Flags modeFlags= {
    T_STR, M_WRITE, QOS_1, I_OBJECT, true
    };
Flags inputFlags= {
    T_STR, M_READ, QOS_1, I_OBJECT, true
    };
Prop pin1_mode( "gpio/1/mode", &pin_1, Gpio::mode, modeFlags ); //
Prop pin2_mode( "gpio/2/mode", &pin_2, Gpio::mode, modeFlags ); //
Prop pin3_mode( "gpio/3/mode", &pin_3, Gpio::mode, modeFlags ); //
Prop pin1_output( "gpio/1/output", &pin_1, Gpio::output, modeFlags ); //
Prop pin2_output( "gpio/2/output", &pin_2, Gpio::output, modeFlags ); //
Prop pin3_output( "gpio/3/output", &pin_3, Gpio::output, modeFlags ); //
Prop pin1_input( "gpio/1/input", &pin_1, Gpio::input, inputFlags ); //
Prop pin2_input( "gpio/2/input", &pin_2, Gpio::input, inputFlags ); //
Prop pin3_input( "gpio/3/input", &pin_3, Gpio::input, inputFlags ); //
Prop sys_log("sys/log",&sys,SysObject::log,inputFlags);



/*****************************************************************************
*   Scan for topic changes and publish with right qos and retain
******************************************************************************/

class PropertyListener : public Sequence {
    private:
        struct pt t;
        uint32_t i;
    public:
        PropertyListener(  ) {
            PT_INIT ( &t );
            }
        int handler ( Event* event ) {
            Prop *p;
            Flags *flags;
            PT_BEGIN ( &t );
            while ( true ) {
                while ( mqtt->isConnected() ) { // new things to publish found
                    for ( i = 0; i <  propListCount ; i++ ) {
                        flags = & propList[i]->_flags;

                        if ( flags->publishValue ) {
                            p =  propList[i];
                            if ( p->_xdr ) {
                                p->_flags.publishValue = false;
                                if ( p->_flags.qos == QOS_0 ) mqttPubQos0.send( p );
                                else if ( ( p->_flags.qos == QOS_1 )  && ( mqttPubQos1.isReady() ) ) mqttPubQos1.send( p );
                                else if ( ( p->_flags.qos == QOS_2 ) && ( mqttPubQos2.isReady() ) )mqttPubQos2.send( p );
                                else  p->_flags.publishValue = true;
//                               p->_xdr( p->_instance, CMD_GET, message );
                                timeout ( TIME_WAIT_REPLY );
                                PT_YIELD_UNTIL ( &t, timeout() );
                                }
                            }
                        else if ( flags->publishMeta ) {
                            // convert flags to string
                            // add desc
                            flags->publishMeta = false;
                            }
                        }
                    timeout ( 5000 ); // sleep between scans
                    PT_YIELD_UNTIL ( &t, timeout() || event->is( PROP_CHANGED ) );
                    }
                PT_YIELD ( &t ); // yield during tcp disconnects
                }
            PT_END ( &t );
            }

    };

//_____________________________________________________________________________________________________
//
#include <unistd.h> // sleep
#include "Timer.h"

//_____________________________________________________________________________________________________
//



int main ( int argc, char *argv[] ) {
    prefix = Sys::getDeviceName();
    prefix += "/";
    mqttOut.prefix ( prefix );
    getPrefix = "GET/";
    getPrefix += prefix;
    putPrefix = "PUT/";
    putPrefix += prefix;



    Queue::getDefaultQueue()->clear();//linux queues are persistent
    sys.log("started... ");
    // static sequences
    Tcp tcp ( "tcp", 1000, 1 );
    mqtt = new Mqtt ( &tcp );
    new MqttSubQos2();
    new MqttSubQos1();
    new MqttSubQos0();
    MqttPing pinger; // ping cycle when connected
    new PropertyListener(); // publish property when changed or interval timeout

    Gpio pin3 ( 3 );
    MainThread mt ( "messagePump", 1000, 1 ); // both threads could be combined
    TimerThread tt ( "timerThread", 1000, 1 );
    tcp.start();
    mt.start();
    tt.start();


    ::sleep ( 1000000 );


    }


















































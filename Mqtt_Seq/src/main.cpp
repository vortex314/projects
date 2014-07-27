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
#define TIME_WAIT_REPLY 1000
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

typedef void( *SG )( void*, Strpack& );


class TopicObject
    {
        virtual void doNothing() {};
    };
typedef  void ( TopicObject::*TopicMethod ) ( Strpack& );

class Prop;
#define PROPS_MAX   20
Prop* propList[PROPS_MAX];
uint32_t propListCount = 0;

class  Prop
    {
    public :
        Str _name;
        void* _instance;
        SG _getter;
        SG _setter;
        Flags _flags;
        const char*  _desc;
    public :
        Prop ( Str& name, TopicObject* instance, SG getter, SG setter,
               Flags flags, const char* const desc ) : _name ( name )
            {
            _instance = instance;
            _getter = getter;
            _setter = setter;
            _flags = flags;
            _desc = desc;
            propList[propListCount++] = this;
            _flags.publishValue = true;
            _flags.publishMeta = true;
            }
        Prop ( TopicObject* instance )
            {
            _instance = instance;
            }
        static Prop* findProp ( Str& name )
            {
            uint32_t i;
            for ( i = 0; i < propListCount; i++ )
                {
                if ( propList[i] )
                    if ( propList[i]->_name == name )
                        {
                        return  propList[i];
                        }
                }
            return 0;
            }
        static void set( Str& topic, Strpack& message, uint8_t header )
            {
            Str str( 30 );
            str.substr( topic, getPrefix.length() );
            Prop* p = findProp( str );
            if ( p )
                {
                if ( p->_setter )
                    p->_setter( p->_instance, message );
//                    ( ( *( p->_instance ) ).* ( p->_setter ) ) ( message );
                p->_flags.publishValue = true;
                }
            Sequence::publish( 0, PROP_CHANGED, 0 );
            }
    };



class TimerThread : public Thread, public Sequence
    {

    public:

        TimerThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority )
            {
            Sys::upTime();
            };
        void run()
            {
            struct timespec deadline;
            clock_gettime ( CLOCK_MONOTONIC, &deadline );
            Sys::_upTime = deadline.tv_sec * 1000 + deadline.tv_nsec / 1000000;
            while ( true )
                {
                deadline.tv_nsec += 1000000000; // Add the time you want to sleep, 1 sec now
                if ( deadline.tv_nsec >= 1000000000 )   // Normalize the time to account for the second boundary
                    {
                    deadline.tv_nsec -= 1000000000;
                    deadline.tv_sec++;
                    }
                clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL );
                Sys::_upTime = deadline.tv_sec * 1000 + deadline.tv_nsec / 1000000;
                publish ( this, TIMER_TICK, 0 );
                }
            }
        int handler ( Event* ev )
            {
            return 0;
            };
    };
//____________________________________________________________________________________________

#include "pt.h"

uint16_t gMessageId = 1;
MqttOut mqttOut ( MAX_MSG_SIZE );

uint16_t nextMessageId()
    {
    return gMessageId++;
    }

/*****************************************************************************
*   MQTT EVENT filter
******************************************************************************/

bool eventIsMqtt ( Event* event, uint8_t type, uint16_t messageId , uint8_t qos )
    {
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

class Mqtt : public Sequence
    {
    private:
        struct pt t;
        Tcp* _tcp;
        uint16_t _messageId;
        bool _isConnected;
        Str str;
        Str msg;

    public :

        Mqtt ( Tcp* tcp ) : str ( 30 ), msg ( 10 )
            {
            _tcp = tcp;
            PT_INIT ( &t );
            _messageId = 2000;
            _isConnected = false;
            };

        int handler ( Event* event )
            {
            PT_BEGIN ( &t );
            while ( 1 )
                {
                _messageId = nextMessageId();
                PT_YIELD_UNTIL ( &t, _tcp->isConnected() );
                mqttOut.Connect ( 0, "clientId", MQTT_CLEAN_SESSION,
                                  "system/online", "false", "", "", TIME_KEEP_ALIVE / 1000 );
                _tcp->send ( &mqttOut );
                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_CONNACK, 0, 0 ) || timeout() );
                if ( timeout() )
                    {
                    _tcp->disconnect();
                    publish ( this, MQTT_DISCONNECTED, 0 );
                    continue;
                    }



                ( str = putPrefix ) += "#" ;
                mqttOut.Subscribe ( 0, str, _messageId, MQTT_QOS1_FLAG );
                _tcp->send ( &mqttOut );

                ( str = getPrefix ) += "#" ;
                mqttOut.Subscribe ( 0, str, _messageId + 2, MQTT_QOS1_FLAG );
                _tcp->send ( &mqttOut );

                str = "system/online";
                msg = "true";
                mqttOut.Publish ( MQTT_QOS1_FLAG, str, msg, nextMessageId() );
                _tcp->send ( &mqttOut );


                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_SUBACK, _messageId, 0 ) || timeout() );
                if ( timeout() )
                    {
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

        Erc send ( Bytes& pb )
            {
            if ( _tcp->isConnected() )
                return _tcp->send ( &pb );
            else return E_AGAIN;
            }
        bool isConnected()
            {
            return _isConnected;
            };
        Erc disconnect()
            {
            return _tcp->disconnect();
            }
    };
/*****************************************************************************
*   HANDLE PING keep alive
******************************************************************************/
class MqttPing : public Sequence
    {
    private:
        struct pt t;
        Mqtt* _mqtt;
        uint16_t _messageId;
        Strpack* _message;
        uint16_t _retryCount;
    public:
        MqttPing ( Mqtt* mqtt )
            {
            _mqtt = mqtt;
            _messageId = nextMessageId();
            PT_INIT ( &t );
            //TODO in destructor free memory
            }
        int handler ( Event* event )
            {
            PT_BEGIN ( &t );
            while ( true )
                {
                _retryCount = 0;
                while ( _mqtt->isConnected() )
                    {
                    mqttOut.PingReq();
                    _mqtt->send ( mqttOut );

                    timeout ( TIME_WAIT_REPLY );
                    PT_YIELD_UNTIL ( &t, timeout() || eventIsMqtt ( event, MQTT_MSG_PINGRESP, 0 , 0 ) );
                    if ( timeout() )
                        {
                        _retryCount++;
                        if (  _retryCount >= 3 )
                            {
                            _mqtt->disconnect();
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
Mqtt* mqtt;
class MqttPubQos0 : public Sequence
    {
    private :
        Str _topic;
        Strpack _message;
        Flags _flags;
    public:

        MqttPubQos0 ( Flags flags, Str& topic, Strpack& message ) : _topic ( topic ), _message ( message )
            {
            _flags = flags;
            }
        int handler ( Event* event )
            {
            uint8_t header = 0;
            if ( _flags.retained ) header += MQTT_RETAIN_FLAG;
            mqttOut.Publish ( header, _topic, _message, nextMessageId() );
            mqtt->send ( mqttOut );
            return PT_ENDED;
            }
    };

class MqttPubQos1 : public Sequence
    {
    private:
        struct pt t;
        uint32_t _retryCount;
        Str _topic;
        Strpack _message;
        Flags _flags;
        uint16_t _messageId;
    public:
        MqttPubQos1 ( Flags flags, Str& topic, Strpack& message ) : _topic ( topic ), _message ( message )
            {
            _flags = flags;
            PT_INIT ( &t );
            _messageId = nextMessageId();
            }
        int handler ( Event* event )
            {
            uint8_t header = 0;
            PT_BEGIN ( &t );
            _retryCount = 0;
            while ( _retryCount < 3 )
                {
                header = MQTT_QOS1_FLAG;
                if ( _flags.retained ) header += MQTT_RETAIN_FLAG;
                if ( _retryCount ) header += MQTT_DUP_FLAG;

                mqttOut.Publish ( header, _topic, _message, _messageId );
                mqtt->send ( mqttOut );

                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBACK, _messageId, 0 ) );
                if ( timeout() )
                    _retryCount++;
                else
                    return PT_ENDED;
                }
            PT_END ( &t );
            }

    };

class MqttPubQos2 : public Sequence
    {
    private:
        struct pt t;
        Str _topic;
        Strpack _message;
        Flags _flags;
        uint16_t _messageId;
    public:
        MqttPubQos2 ( Flags flags, Str& topic, Strpack& message ) : _topic ( topic ), _message ( message )
            {
            _flags = flags;
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            uint8_t header = MQTT_QOS2_FLAG;
            PT_BEGIN ( &t );
            if ( _flags.retained ) header += MQTT_RETAIN_FLAG;

            mqttOut.Publish ( header, _topic, _message, _messageId );
            mqtt->send ( mqttOut );

            timeout ( TIME_WAIT_REPLY );
            PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREC, _messageId, 0 ) );

            mqttOut.PubRel ( _messageId );
            mqtt->send ( mqttOut );

            timeout ( TIME_WAIT_REPLY );
            PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBCOMP, _messageId, 0 ) );
            PT_END ( &t );
            }

    };


void doPublish ( Flags flags, Str& topic, Strpack& message )
    {
    if ( flags.qos == QOS_0 ) new MqttPubQos0 ( flags, topic, message );
    if ( flags.qos == QOS_1 ) new MqttPubQos1 ( flags, topic, message );
    if ( flags.qos == QOS_2 ) new MqttPubQos2 ( flags, topic, message );
    }



/*****************************************************************************
*   HANDLE Property scan for changes
******************************************************************************/

/*****************************************************************************
*   HANDLE MQTT received publish message with qos=2
******************************************************************************/
class MqttSubQos0 : public Sequence
    {
    private:
        struct pt t;
    public:
        MqttSubQos0()
            {
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            PT_BEGIN ( &t );
            while ( true )
                {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , 0 ) );
                MqttIn* mqttIn = ( MqttIn* )event->data();
                if ( mqttIn->qos() == 0 )
                    Prop::set ( *mqttIn->topic(), *mqttIn->message() , mqttIn->qos() ) ;
                }
            PT_END ( &t );
            }

    };
class MqttSubQos1 : public Sequence
    {
    private:
        struct pt t;
    public:
        MqttSubQos1()
            {
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            PT_BEGIN ( &t );
            while ( true )
                {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , MQTT_QOS1_FLAG ) );
                MqttIn* mqttIn = ( MqttIn* )event->data();
                mqttOut.PubAck ( mqttIn->messageId() ); // send PUBACK
                mqtt->send ( mqttOut );
                Prop::set ( *mqttIn->topic(), *mqttIn->message() , mqttIn->qos() ) ;

                }
            PT_END ( &t );
            }

    };

class MqttSubQos2 : public Sequence
    {
    private:
        struct pt t;
        uint16_t _messageId;
        MqttIn save;
        bool _isReady;
    public:
        MqttSubQos2 ( ) : save ( MAX_MSG_SIZE )
            {
            _messageId = 0;
            _isReady = true;
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            MqttIn* mqttIn;
            PT_BEGIN ( &t );
            while( true )
                {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 , MQTT_QOS2_FLAG ) );
                _isReady = false;
                mqttIn = ( MqttIn* )( event->data() );
                _messageId = mqttIn->messageId();

                mqttOut.PubRec ( _messageId );
                mqtt->send ( mqttOut );

                save.clone ( * ( ( MqttIn* ) ( event->data() ) ) ); // clone the message
                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREL, _messageId, 0 ) );
                if ( timeout() )
                    {
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

uint32_t getAllocSize ( void )
    {
    uint32_t memoryAllocated;
    struct mallinfo mi;

    mi = mallinfo();
    memoryAllocated =  mi.uordblks;
    return memoryAllocated;
    }
//____________________________________________________________________________________________
//
class MainThread : public Thread
    {
    private :

    public:
        MainThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority )
            {
            };
        void run()
            {
            Str line ( 200 );
            while ( true )
                {
                Event event;
                Queue::getDefaultQueue()->get ( &event ); // dispatch eventually IDLE message
                if ( event.id() != TIMER_TICK )
                    {
                    line.set ( " EVENT : " );
                    event.toString ( line );

                    Sys::getLogger().append ( line );
                    Sys::getLogger().flush();
                    line.clear();
                    std::cout << Sys::upTime() << " | EVENT " << event.getEventIdName() << std::endl;
                    }

                int i;
                for ( i = 0; i < MAX_SEQ; i++ )
                    if ( Sequence::activeSequence[i] )
                        {
                        if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED )
                            {
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
enum Cmd { CMD_GET,CMD_PUT,CMD_META };
#include "Strpack.h"
gnublin_gpio gpio;

class Gpio : public TopicObject
    {
    private:
        int _pin;
        char _value;
        struct pt t;
    public:
        Gpio ( int pin )
            {
            _pin = pin;
            gpio.pinMode ( _pin, INPUT );
            PT_INIT ( &t );
            Str className ( 20 );
            className.set( "gpio/" ).append ( pin ).append ( "/" );
            Str str( 30 );
            str = className;
            str += "mode";

            new Prop ( str, this, getMode, setMode, ( Flags )
                {
                T_STR, M_WRITE, QOS_1, I_OBJECT, true
                } , "desc:'Direction for pin',set:['I','O']" );
            str = className;
            str += "output";
            new Prop ( str, this, 0 , setOutput, ( Flags )
                {
                T_STR, M_WRITE, QOS_1, I_OBJECT, true
                } , "desc:'Output for pin',set:['1','0']" );
            str = className;
            str += "input";
            new Prop ( str, this, getInput, 0, ( Flags )
                {
                T_STR, M_WRITE, QOS_1, I_OBJECT, true
                } , "desc:'Input for pin',set:['1','0']" );
            }

        static void mode(void *instance,enum Cmd cmd,Strpack& strp){
            static Flags flags={T_STR, M_WRITE, QOS_1, I_OBJECT, true};
            static desc= "desc:'Input for pin',set:['1','0']" ;
            Gpio* gpio=(Gpio*)instance;
            if ( cmd == 1 ) {
            }
        })
        static void setMode( void *th, Strpack& strp )
            {
            Gpio* pin = ( ( Gpio* )th );
            char mode = strp.peek( 0 );
            if ( mode == 'O' )
                {
                gpio.pinMode ( pin->_pin, OUTPUT );
                }
            else if ( mode == 'I' )
                {
                gpio.pinMode ( pin->_pin, INPUT );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        static void getMode( void *th, Strpack& strp )
            {
            Gpio* pin = ( ( Gpio* )th );
            char mode = strp.peek( 0 );
            if ( mode == 'O' )
                {
                gpio.pinMode ( pin->_pin, OUTPUT );
                }
            else if ( mode == 'I' )
                {
                gpio.pinMode ( pin->_pin, INPUT );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        static void getInput( void *th, Strpack& value )
            {
            Gpio* pin = ( ( Gpio* )th );
            value.append( "01"[gpio.digitalRead ( pin->_pin ) ] );
            }

        static void setOutput( void *th, Strpack& value )
            {
            Gpio* pin = ( ( Gpio* )th );
            char o = value.peek ( 0 ) ;

            if ( o == '1' )
                {
                pin->_value = '1';
                gpio.digitalWrite ( pin->_pin, HIGH );
                }
            else if ( o == '0' )
                {
                pin->_value = '1';
                gpio.digitalWrite ( pin->_pin, LOW );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        char getInput()
            {
            return "01"[gpio.digitalRead ( _pin ) ];
            }


    };

class SysObject : public TopicObject
    {
    private:
        Prop* memoryProp;
    public:
        SysObject()
            {
            Str s( "sys/heapMemory" );
            //       memoryProp = new Prop(s).setter(getMemory).type(T_UINT32).mode(M_WRITE).qos(QOS_1);
            memoryProp = new Prop ( s, this, getMemory, 0, ( Flags )
                {
                T_UINT32, M_WRITE, QOS_1, I_OBJECT, true
                } , "desc:'Memory allocated to heap'" );
            }
        static void getMemory ( void *instance, Strpack& value )
            {
            SysObject* sys = ( SysObject* )instance;
            value.append ( getAllocSize() );
            sys->memoryProp->_flags.publishValue = true;
            }
    };


SysObject sys;
Gpio pin_1 ( 1 );
Gpio pin_2 ( 2 );
Gpio pin_3 ( 3 );

new Prop("gpio/1/mode",&pin_1,Gpio::mode)



/*****************************************************************************
*   Scan for topic changes and publish with right qos and retain
******************************************************************************/

class PropertyListener : public Sequence
    {
    private:
        struct pt t;
        uint32_t i;
        Strpack message;
    public:
        PropertyListener(  ) : message ( MAX_MSG_SIZE )
            {
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            Prop *p;
            Flags *flags;
            PT_BEGIN ( &t );
            while ( true )
                {
                while ( mqtt->isConnected() )  // new things to publish found
                    {
                    for ( i = 0; i <  propListCount ; i++ )
                        {
                        flags = & propList[i]->_flags;

                        if ( flags->publishValue )
                            {
                            message.clear();
                            p =  propList[i];
                            if ( p->_getter )
                                {
                                p->_getter( p->_instance, message );
                                doPublish( *flags, p->_name, message );
                                flags->publishValue = false;
                                timeout ( TIME_WAIT_REPLY );
                                PT_YIELD_UNTIL ( &t, timeout() );
                                }
                            }
                        else if ( flags->publishMeta )
                            {
                            // convert flags to string
                            // add desc
                            flags->publishMeta = false;
                            }
                        }
                    timeout ( 1000 ); // sleep between scans
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



int main ( int argc, char *argv[] )
    {
    prefix = Sys::getDeviceName();
    prefix += "/";
    mqttOut.prefix ( prefix );
    getPrefix = "GET/";
    getPrefix += prefix;
    putPrefix = "PUT/";
    putPrefix += prefix;



    Queue::getDefaultQueue()->clear();//linux queues are persistent
    // static sequences
    Tcp tcp ( "tcp", 1000, 1 );
    mqtt = new Mqtt ( &tcp );
    new MqttSubQos2();
    new MqttSubQos1();
    new MqttSubQos0();
    MqttPing pinger ( mqtt ); // ping cycle when connected
    new PropertyListener(); // publish property when changed or interval timeout

    Gpio pin3 ( 3 );
    MainThread mt ( "messagePump", 1000, 1 ); // both threads could be combined
    TimerThread tt ( "timerThread", 1000, 1 );
    tcp.start();
    mt.start();
    tt.start();


    ::sleep ( 1000000 );


    }







































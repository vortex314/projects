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
EventId PROPERTY_SET = Event::nextEventId ( ( char* const ) "PROPERTY_SET" );
Str prefix ( 30 );
Str putPrefix ( 30 );
Str getPrefix ( 30 );

class PropertySet : public EventData
    {
    public:
        Str& _topic;
        Strpack& _message;

        PropertySet ( Str& topic, Strpack& message )
            : _topic ( topic ), _message ( message )
            {

            }
        ~PropertySet()
            {
            }
        void toString ( Str& str )
            {
            str.append ( "{ topic : " ).append ( _topic );
            str.append ( ", message : " ).append ( _message );
            str.append ( " }" );
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

bool eventIsMqtt ( Event* event, uint8_t type, uint16_t messageId )
    {
    if ( event->id() == Tcp::MQTT_MESSAGE )
        if ( ( ( MqttIn* ) event->data() )->type() == type )
            {
            if ( messageId )
                {
                if ( ( ( MqttIn* ) event->data() )->messageId() == messageId )
                    return true;
                }
            else
                return true;
            }

    return false;;;
    }


/*****************************************************************************
   HANDLE MQTT connection and subscribe sequence
******************************************************************************/
EventId MQTT_CONNECTED = Event::nextEventId ( ( char* const ) "MQTT_CONNECTED" );
EventId MQTT_DISCONNECTED = Event::nextEventId ( ( char* const ) "MQTT_DISCONNECTED" );

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

        Mqtt ( Tcp* tcp ) : str ( 30 ),msg(10)
            {
            _tcp = tcp;
            PT_INIT ( &t );
            _messageId = 2000;
//        mqttOut.prefix(Sys::getDeviceName());
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
                                  "pcdebian/system/online", "false", "", "", TIME_KEEP_ALIVE / 1000 );
                _tcp->send ( &mqttOut );
                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_CONNACK, 0 ) || timeout() );
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

                (str = prefix) += "system/online";
                msg="true";
                mqttOut.Publish(MQTT_QOS1_FLAG,str,msg,nextMessageId());
                _tcp->send ( &mqttOut );


                timeout ( TIME_WAIT_REPLY );
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_SUBACK, _messageId ) || timeout() );
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
            return _tcp->send ( &pb );
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
                while ( _mqtt->isConnected() )
                    {
                    mqttOut.PingReq();
                    _mqtt->send ( mqttOut );

                    timeout ( TIME_WAIT_REPLY );
                    PT_YIELD_UNTIL ( &t, timeout() || eventIsMqtt ( event, MQTT_MSG_PINGRESP, 0 ) );
                    if ( timeout() )
                        {
                        _mqtt->disconnect();
                        PT_RESTART ( &t );
                        }
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
            mqttOut.prefix ( Sys::getDeviceName() );
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
                PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBACK, _messageId ) );
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
            PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREC, _messageId ) );

            mqttOut.PubRel ( _messageId );
            mqtt->send ( mqttOut );

            timeout ( TIME_WAIT_REPLY );
            PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBCOMP, _messageId ) );
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

class MqttSubQos2 : public Sequence
    {
    private:
        struct pt t;
        Mqtt* _mqtt;
        uint16_t _messageId;
        MqttIn save;
    public:
        MqttSubQos2 ( Mqtt* mqtt, uint16_t messageId ) : save ( MAX_MSG_SIZE )
            {
            _mqtt = mqtt;
            _messageId = messageId;
            PT_INIT ( &t );
            }
        int handler ( Event* event )
            {
            PT_BEGIN ( &t );
            PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, _messageId ) );
            // no timeout since the sequence is created because of event is there
            mqttOut.PubRec ( _messageId );
            _mqtt->send ( mqttOut );
            //TODO clone message
            save.clone ( * ( ( MqttIn* ) ( event->data() ) ) );
            timeout ( TIME_WAIT_REPLY );
            PT_YIELD_UNTIL ( &t, timeout() ||  eventIsMqtt ( event, MQTT_MSG_PUBREL, _messageId ) );
            if ( timeout() )
                {
                return PT_ENDED; // abandon and go for destruction
                }

            mqttOut.PubComp ( _messageId );
            _mqtt->send ( mqttOut );

            publish ( this, PROPERTY_SET, new PropertySet ( * ( save.topic() ), * ( save.message() ) ) );
            // clone will be deleted in default destructor
            PT_END ( &t );
            }

    };



/*****************************************************************************
*   DISPATCH MQttMessages depending on case
******************************************************************************/
class MqttDispatcher : public Sequence
    {
    private:
        struct pt t;
        uint16_t _messageId;

    public :
        MqttDispatcher( )
            {
            PT_INIT ( &t );
            _messageId = nextMessageId();
            };


        int handler ( Event* event )
            {
            MqttIn* mqttIn;
            PT_BEGIN ( &t );
            while ( 1 )
                {
                PT_YIELD_UNTIL ( &t, eventIsMqtt ( event, MQTT_MSG_PUBLISH, 0 ) );
                    {
                    mqttIn = ( MqttIn* ) event->data();
                    switch ( mqttIn->qos() )
                        {
                        case MQTT_QOS0_FLAG :
                                {
                                publish ( this
                                          , PROPERTY_SET
                                          , new PropertySet ( *mqttIn->topic(), *mqttIn->message() ) );

                                break;
                                }
                        case MQTT_QOS1_FLAG:
                                {
                                mqttOut.PubAck ( mqttIn->messageId() ); // send PUBACK
                                mqtt->send ( mqttOut );
                                publish ( this
                                          , PROPERTY_SET
                                          , new PropertySet ( *mqttIn->topic(), *mqttIn->message() ) );


                                break;
                                }
                        case MQTT_QOS2_FLAG:
                                {
                                new MqttSubQos2 ( mqtt, mqttIn->messageId() ); // new sequence to handle
                                break;
                                }
                        }
                    }
                PT_YIELD ( &t );
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
                if ( event.data() != NULL );
//               delete  (Bytes*)event.data();
                }
            }
    };


//__________________________________________________________________________________
//

//__________________________________________________________________________________
//

uint32_t p = 1234;
bool online = true;

Property property2 ( &online, ( Flags )
    {
    T_BOOL, M_WRITE, QOS_2, I_ADDRESS
    }, "system/online", "$META" );

const PropertyConst pc = { { ( void* ) &p}, "NAME", "META", {
        T_BOOL, M_READ, QOS_1, I_ADDRESS
        }
    };
Property property1 ( &p, ( Flags )
    {
    T_UINT32, M_READ, QOS_1, I_ADDRESS
    }, "ikke/P", "$META" );

Property pp ( &pc );



#define BOARD RASPBERRY_PI

#include "gnublin.h"
/*
   gpio_1/mode .set .get .meta
   gpio/1/out
   gpio/1/in
   gpio/pin/1/mode
   gpio/pin/1 -> true or false

   gpio/0/out      - 0/1 uint8_t
   gpio/0/in       - 0/1
   gpio/0/mode     - R/W STR

   spi/0/out       -> O
   spi/0/in        -> I
   spi/0/clock     -> I/O

   pcdebian/spi_0_/out
   pcdebian/spi_0_/in
   pcdebian/spi_0_/clock
*/
/*
class

Property pin_0(Setter setter, Getter getter, {T_STR,M_WRITE,QOS_1,I_SETTER},
             "gpio/pin_0/mode", "{ mode : }");

    Properties gpio/pin_xxx/mode -> gpio.pinMode(xxx,mode) -> setter & getter
    properties gpio/pin_xxx/mode ->
*/
#include "Strpack.h"
gnublin_gpio gpio;

/*
    pcdebian/gpio/pin/ class Pin
    pcdebian/gpio/pin/1/ class Pin instance 1
        0 mode,in,out  .set,.upd,.meta

    <class/><instance/><property><.method> <value>

    <class/><instance/><property>.meta => object.publish(META,prop);
    <class/><instance/><property>.upd => object.publish(VALUE,prop);
    <class/><instance/><property>.set => object.setValue(prop,value);
    value changed => object.publish(VALUE,property);
    startup => object.publish(ALL,"");

    topic owner device listens to
        .set
        .upd
        .meta
    topic owner publishes topic

    master device listens to topic

    master publishes .set,.upd,.metaGET_ANY_UPDATE,GET_DESC
};
*/

enum Cmd { CMD_GET, CMD_PUT, CMD_META , CMD_FLAGS};

class TopicObject
    {

    };
typedef  void ( TopicObject::*TopicMethod ) ( Strpack& );

class Prop;
#define PROPS_MAX   10
Prop* propList[PROPS_MAX];
uint32_t propListCount = 0;

class  Prop
    {
    public :
        Str _name;
        TopicObject _instance;
        TopicMethod _getter;
        TopicMethod _setter;
        Flags _flags;
        const char*  _desc;
    public :
        Prop ( const char* const name, TopicObject instance, TopicMethod getter, TopicMethod setter,
               Flags flags, const char* const desc ) : _name ( name )
            {
            _instance = instance;
            _getter = getter;
            _setter = setter;
            _flags = flags;
            _desc = desc;
            propList[propListCount++] = this;
            }
        Prop ( TopicObject instance )
            {
            _instance = instance;
            }
        static Prop* findProp ( Str& name )
            {
            uint32_t i;
            for ( i = 0; i < propListCount; i++ )
                {
                if ( propList[i]->_name == name )
                    {
                    return  propList[i];
                    }
                }
            return 0;
            }
    };

class GpioPin : public TopicObject
    {
    private:
        int _pin;
        char _mode;
        char _value;
        struct pt t;
    public:
        GpioPin ( int pin )
            {
            _pin = pin;
            setMode ( 'I' );
            PT_INIT ( &t );
            Str name ( 20 );
            name = "gpio/";
            name.append ( pin ).append ( "/" );
            new Prop ( "gpio/1/mode", *this, ( TopicMethod ) &GpioPin::getMode , ( TopicMethod ) &GpioPin::setMode,
                       ( Flags )
                {
                T_STR, M_WRITE, QOS_1, I_OBJECT, true
                } , "desc:'Direction for pin',set:['I','O']" );
//            new Prop ( "gpio/1/mode", *this, (TopicMethod)&GpioPin::getMode, (TopicMethod)&setMode, "desc:'Direction for pin',set:['I','O']" )
//       setOutput('0'));
            }

        void setMode ( Strpack& value )
            {
            setMode ( value.peek ( 0 ) );
            }
        void getMode ( Strpack& value )
            {
            value.append ( _mode );
            }
        Flags input ( enum Cmd cmd, Strpack& value )
            {
            static Flags flags = {T_STR, M_READ, QOS_1, I_OBJECT, true};
            switch ( cmd )
                {
                case CMD_PUT :
                        {
                        flags.publishValue = true;
                        break;
                        }
                case CMD_GET :
                        {
                        value.append ( getInput() );
                        flags.publishValue = false;
                        break;
                        }
                case  CMD_META :
                        {
                        value.append ( "desc:'Input for pin',set:['0','1']" );
                        break;
                        }
                default:
                        {
                        }
                }
            return flags;
            }
        Flags output ( enum Cmd cmd, Strpack& value )
            {
            static Flags flags = {T_STR, M_READ, QOS_1, I_OBJECT, true};
            switch ( cmd )
                {
                case CMD_PUT :
                        {
                        setOutput ( value.peek ( 0 ) );
                        flags.publishValue = true;
                        break;
                        }
                case CMD_GET :
                        {
                        value.append ( getInput() );
                        flags.publishValue = false;
                        break;
                        }
                case  CMD_META :
                        {
                        value.append ( "desc:'Output for pin',set:['0','1']" );
                        flags.publishMeta = false;
                        break;
                        }
                default:
                        {
                        }
                }
            return flags;
            }

        void setMode ( char mode )
            {
            if ( mode == 'O' )
                {
                _mode = 'O';
                gpio.pinMode ( _pin, OUTPUT );
                }
            else if ( mode == 'I' )
                {
                _value = 'I';
                gpio.pinMode ( _pin, INPUT );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        void setOutput ( char o )
            {
            if ( o == '1' )
                {
                _value = '1';
                gpio.digitalWrite ( _pin, HIGH );
                }
            else if ( o == '0' )
                {
                _value = '1';
                gpio.digitalWrite ( _pin, LOW );
                }
            else Sys::logger ( "invalid pin value for GPIO" );
            }
        char getInput()
            {
            return "01"[gpio.digitalRead ( _pin ) ];
            }


    };

/*

prop(cmd,Strpack)

prop(CMD_PUT,flags,strp)
prop(CMD_GET,flags,strp)
prop(GET_ANY_UPDATE,null)
prop(CMD_META,strp)

*/




class SysObject : public TopicObject
    {
    public:
        SysObject() { }
        Flags topicObject ( Cmd cmd, Strpack& str )
            {
            static Flags flags = {T_INT32, M_READ, QOS_0, I_OBJECT, false};
            return flags;
            }
        void getMemory ( Strpack& value )
            {
            value.append ( getAllocSize() );
            }
    };


SysObject sys;
GpioPin pin_1 ( 1 );
GpioPin pin_2 ( 2 );
GpioPin pin_3 ( 3 );



class TopicListener : public Sequence
    {
    private:
        struct pt t;
        struct TopicEntry* topicEntry;
        Flags flags;

    public:
        TopicListener()
            {
            PT_INIT ( &t );
            }

        int handler ( Event* event )
            {
            PropertySet* ps;
            Str topic ( 30 );
            Str str = Sys::getLogger();
            PT_BEGIN ( &t );
            while ( true )
                {
                PT_YIELD_UNTIL ( &t, event->is ( PROPERTY_SET ) );

                ps = ( PropertySet* ) event->data();

                str = " PROPERTY SET : ";
                str += ps->_topic;
                str += " :" ;
                str += ps->_message;
                Sys::getLogger().flush();

                topic.clear();

                if ( ps->_topic.startsWith ( putPrefix ) )
                    {
                    topic.substr ( ps->_topic, putPrefix.length() );
                    Prop* p = Prop::findProp(ps->_topic);
                    if ( p != 0 )
                    ( ( p->_instance ).* ( p->_setter ) ) ( ps->_message );
                    }
                else if ( ps->_topic.startsWith ( getPrefix ) )
                    {
                    topic.substr ( ps->_topic, putPrefix.length() );
                    Prop* p = Prop::findProp(ps->_topic);
                    if ( p != 0 )
                        {
                        if ( ps->_message.startsWith ( "VALUE" ) )
                            {
                            p->_flags.publishValue=true;
                            }
                        else if ( ps->_message.startsWith ( "META" ) )
                            {
                            p->_flags.publishMeta=true;
                            }
                        }
                    }
                PT_YIELD ( &t );
                }
            PT_END ( &t );
            };

    };

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
            PT_BEGIN ( &t );
            while ( true )
                {
                    while ( true )  // new things to publish found
                        {
                            for ( i = 0; i <  propListCount ; i++ )
                                {
                                    Flags* flags = & propList[i]->_flags;

                                    if ( flags->publishValue )
                                        {
                                            Prop* p =  propList[i];
                                            ( ( p->_instance ).* ( p->_getter ) ) ( message );
//                        doPublish(*flags,topicList[i].name,message);
                                            flags->publishValue = false;
                                        }
                                    else if ( flags->publishMeta )
                                        {
                                            // convert flags to string
                                            // add desc
                                            flags->publishMeta = false;
                                        }
                                }

                        }
                    timeout ( 5000 );
                    PT_YIELD_UNTIL ( &t, timeout() );
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
    Strpack strp;
    prefix = Sys::getDeviceName();
    prefix += "/";

//   prefix.clear().append(Sys::getDeviceName()).append("/");

    getPrefix = "GET/";
    getPrefix += prefix;
    putPrefix = "PUT/";
    putPrefix += prefix;

    Queue::getDefaultQueue()->clear();//linux queues are persistent
    // static sequences
    Tcp tcp ( "tcp", 1000, 1 );
    mqtt = new Mqtt ( &tcp );
    MqttDispatcher mqttDispatcher( );  // start dynamic sequences
    MqttPing pinger ( mqtt ); // ping cycle when connected
    new TopicListener();
    PropertyListener pl(); // publish property when changed or interval timeout
//   SysObject sys;
    GpioPin pin3 ( 3 );
    MainThread mt ( "messagePump", 1000, 1 ); // both threads could be combined
    TimerThread tt ( "timerThread", 1000, 1 );
    tcp.start();
    mt.start();
    tt.start();


    ::sleep ( 1000000 );

    // common thread approach
    /*
    LOOP {
        look for smallest timeout
        read tcp with timeout
        if ( timeout) generate timer-event
        if ( data on tcp ) generate data-event
        if ( disconnect on tcp ) generate disconnection event
        execute waiting-events
    }
    */


    }






























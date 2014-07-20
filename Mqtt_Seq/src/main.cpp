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
#define SLEEP(xxx)  timeout(xxx); \
                PT_WAIT_UNTIL(&t,timeout());

EventId TIMER_TICK= Event::nextEventId(( char* const)"TIMER_TICK");
EventId PROPERTY_SET= Event::nextEventId(( char* const)"PROPERTY_SET");

class PropertySet
{
public:
    Str& _topic;
    Strpack& _message;

    PropertySet(Str& topic,Strpack& message)
        : _topic(topic),_message(message)
    {

    }
    ~PropertySet()
    {
    }
};

class TimerThread : public Thread,public Sequence
{

public:

    TimerThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
        Sys::upTime();
    };
    void run()
    {
        struct timespec deadline;
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        Sys::_upTime= deadline.tv_sec*1000 + deadline.tv_nsec/1000000;
        while(true)
        {
// Add the time you want to sleep
            deadline.tv_nsec += 100000000;

// Normalize the time to account for the second boundary
            if(deadline.tv_nsec >= 1000000000)
            {
                deadline.tv_nsec -= 1000000000;
                deadline.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
            Sys::_upTime= deadline.tv_sec*1000 + deadline.tv_nsec/1000000;
//        Timer::checkAll();
            publish(this,TIMER_TICK,0);
        }

    }
    int handler(Event* ev)
    {
        return 0;
    };


};
//____________________________________________________________________________________________

#include "pt.h"

uint16_t gMessageId=1;
MqttOut* mqttOut=new MqttOut(256);

uint16_t nextMessageId()
{
    return gMessageId++;
}

/*****************************************************************************
*   MQTT EVENT filter
******************************************************************************/

bool eventIsMqtt(Event* event,uint8_t type, uint16_t messageId)
{
    if ( event->id() == Tcp::MQTT_MESSAGE )
        if ( ((MqttIn*)event->data())->type()==type)
        {
            if ( messageId )
            {
                if ( ((MqttIn*)event->data())->messageId()==messageId)
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
EventId MQTT_CONNECTED=Event::nextEventId(( char* const)"MQTT_CONNECTED");
EventId MQTT_DISCONNECTED=Event::nextEventId((char* const)"MQTT_DISCONNECTED");
class Mqtt : public Sequence
{
private:
    struct pt t;
    Tcp* _tcp;
    uint16_t _messageId;
    bool _isConnected;

public :

    Mqtt(Tcp* tcp)
    {
        _tcp=tcp;
        PT_INIT(&t);
        _messageId=2000;
        mqttOut->prefix(Sys::getDeviceName());
        _isConnected = false;
    };

    int handler(Event* event)
    {
        PT_BEGIN(&t);
        while(1)
        {
            _messageId=nextMessageId();
            PT_WAIT_UNTIL(&t,_tcp->isConnected());
            mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION,
                             "system/online", "false", "", "", TIME_KEEP_ALIVE/1000);
            _tcp->send(mqttOut);
            timeout(TIME_WAIT_REPLY);
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,MQTT_MSG_CONNACK,0) || timeout() );
            if ( timeout() )
            {
                _tcp->disconnect();
                publish(this,MQTT_DISCONNECTED,0);
                continue;
            }

            timeout(TIME_WAIT_REPLY);
            mqttOut->Subscribe(0,"#",_messageId,MQTT_QOS1_FLAG);
            _tcp->send(mqttOut);
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,MQTT_MSG_SUBACK,_messageId) || timeout());
            if ( timeout() )
            {
                _tcp->disconnect();
                publish(this,MQTT_DISCONNECTED,0);
                continue;
            }

            publish(this,MQTT_CONNECTED,0);
            _isConnected=true;
            PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_DISCONNECTED));

            publish(this,MQTT_DISCONNECTED,0);
            _isConnected=false;
        }
        PT_END(&t);
    }

    Erc send(Bytes* pb)
    {
        return _tcp->send(pb);
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
    MqttPing(Mqtt* mqtt)
    {
        _mqtt=mqtt;
        _messageId=nextMessageId();
        PT_INIT(&t);
        //TODO in destructor free memory
    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        while(true)
        {
            while ( _mqtt->isConnected())
            {
                mqttOut->PingReq();
                _mqtt->send(mqttOut);

                timeout(TIME_WAIT_REPLY);
                PT_WAIT_UNTIL(&t,timeout() || eventIsMqtt(event,MQTT_MSG_PINGRESP,0));
                if ( timeout())
                {
                    _mqtt->disconnect();
                    PT_RESTART(&t);
                }
                SLEEP(TIME_PING);
//               timeout(TIME_PING);
//               PT_WAIT_UNTIL(&t,timeout() );   // sleep between pings
            }
            PT_YIELD(&t);
        }
        PT_END(&t);
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

    MqttPubQos0(Flags flags,Str& topic,Strpack& message) : _topic(topic),_message(message)
    {
        _flags=flags;
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        if ( _flags.retained ) header += MQTT_RETAIN_FLAG;
        mqttOut->prefix(Sys::getDeviceName());
        mqttOut->Publish(header,_topic,_message,nextMessageId());
        mqtt->send(mqttOut);
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
    MqttPubQos1(Flags flags,Str& topic,Strpack& message): _topic(topic),_message(message)
    {
        _flags=flags;
        PT_INIT(&t);
        _messageId=nextMessageId();
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        PT_BEGIN(&t);
        _retryCount=0;
        while ( _retryCount<3 )
        {
            header = MQTT_QOS1_FLAG;
            if ( _flags.retained ) header += MQTT_RETAIN_FLAG;
            if ( _retryCount ) header+=MQTT_DUP_FLAG;

            mqttOut->Publish(header,_topic,_message,_messageId);
            mqtt->send(mqttOut);

            timeout(TIME_WAIT_REPLY);
            PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBACK,_messageId));
            if ( timeout() )
                _retryCount++;
            else
                return PT_ENDED;
        }
        PT_END(&t);
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
    MqttPubQos2(Flags flags,Str& topic,Strpack& message): _topic(topic),_message(message)
    {
        _flags=flags;
        PT_INIT(&t);
    }
    int handler(Event* event)
    {
        uint8_t header=MQTT_QOS2_FLAG;
        PT_BEGIN(&t);
        if ( _flags.retained ) header += MQTT_RETAIN_FLAG;

        mqttOut->Publish(header,_topic,_message,_messageId);
        mqtt->send(mqttOut);

        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBREC,_messageId));

        mqttOut->PubRel(_messageId);
        mqtt->send(mqttOut);

        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBCOMP,_messageId));
        PT_END(&t);
    }

};


void doPublish(Flags flags,Str& topic,Strpack& message)
{
    if ( flags.qos == QOS_0) new MqttPubQos0(flags,topic,message);
    if ( flags.qos == QOS_1) new MqttPubQos1(flags,topic,message);
    if ( flags.qos == QOS_2) new MqttPubQos2(flags,topic,message);
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
    MqttIn* clone;
public:
    MqttSubQos2(Mqtt* mqtt,uint16_t messageId)
    {
        _mqtt=mqtt;
        _messageId=messageId;
        PT_INIT(&t);
    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        PT_WAIT_UNTIL(&t,eventIsMqtt(event,MQTT_MSG_PUBLISH,_messageId));
        // no timeout since the sequence is created because of event is there
        mqttOut->PubRec(_messageId);
        _mqtt->send(mqttOut);
        //TODO clone message
        clone = new MqttIn(*((MqttIn*)(event->data())));
        clone->parse();
        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBREL,_messageId));
        if ( timeout() )
        {
            delete clone;
            return PT_ENDED; // abandon and go for destruction
        }

        mqttOut->PubComp(_messageId);
        _mqtt->send(mqttOut);

        publish(this,PROPERTY_SET,new PropertySet(*clone->topic(),*clone->message()));
        delete clone;// free cloned messages
        PT_END(&t);
    }

};
/*****************************************************************************
*   DISPATCH MQttMessages depending on case
******************************************************************************/
class MqttDispatcher : public Sequence
{
private:
    struct pt t;
    Mqtt* _mqtt;
    MqttOut* _mqttOut;
    uint16_t _messageId;

public :
    MqttDispatcher(Mqtt* mqtt)
    {
        _mqttOut = new MqttOut(100);
        _mqtt=mqtt;
        PT_INIT(&t);
        _messageId=3000;
    };


    int handler(Event* event)
    {
        MqttIn* mqttIn;
        PT_BEGIN(&t);
        while(1)
        {
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,MQTT_MSG_PUBLISH,0));
            {
                mqttIn=(MqttIn*)event->data();
                switch(mqttIn->qos())
                {
                case MQTT_QOS0_FLAG :
                {
                    publish(this
                            ,PROPERTY_SET
                            ,new PropertySet(*mqttIn->topic(),*mqttIn->message()));

                    break;
                }
                case MQTT_QOS1_FLAG:
                {
                    _mqttOut->PubAck(mqttIn->messageId()); // send PUBACK
                    _mqtt->send(_mqttOut);
                    publish(this
                            ,PROPERTY_SET
                            ,new PropertySet(*mqttIn->topic(),*mqttIn->message()));


                    break;
                }
                case MQTT_QOS2_FLAG:
                {
                    new MqttSubQos2(_mqtt,mqttIn->messageId()); // new sequence to handle
                    break;
                }
                }
            }
            PT_YIELD(&t);
        }
        PT_END(&t);
    }
};
/*****************************************************************************
*   HANDLE Property publishing
******************************************************************************/

#include <malloc.h>

uint32_t getAllocSize(void)
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
    MainThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
    void run()
    {
        Str line(100);
        while(true)
        {
            Event event;
            Queue::getDefaultQueue()->get(&event); // dispatch eventually IDLE message
            if ( event.id() != TIMER_TICK )
            {
                line.append(" EVENT : ");
                event.toString(&line);
                Sys::getLogger().append(&line);
                Sys::getLogger().flush();
                line.clear();
                std::cout<< Sys::upTime() << " | EVENT "<< event.getEventIdName()<<std::endl;
            }

            getAllocSize();
            int i;
            for(i=0; i<MAX_SEQ; i++)
                if(Sequence::activeSequence[i])
                {
                    if ( Sequence::activeSequence[i]->handler(&event) == PT_ENDED )
                    {
                        Sequence* seq= Sequence::activeSequence[i];
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

uint32_t p=1234;
bool online=true;

Property property2(&online, (Flags)
{
    T_BOOL,M_WRITE,QOS_2,I_ADDRESS
}, "system/online","$META");

const PropertyConst pc= { {(void*)&p},"NAME","META",{
        T_BOOL,M_READ,QOS_1,I_ADDRESS
    }
};
Property property1(&p, (Flags)
{
    T_UINT32,M_READ,QOS_1,I_ADDRESS
}, "ikke/P","$META");

Property pp(&pc);



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

*/
#include "Property.h"

enum Cmd
{
    GET_VALUE,GET_META,SET_VALUE,GET_ANY_UPDATE,GET_DESC
};
class TopicObject
{
    virtual Flags topicObject(Cmd cmd,Strpack& str)=0;
};
typedef  Flags (TopicObject::*TopicMethod)(enum Cmd , Strpack& );


class GpioPin :public TopicObject
{
private:
    int _pin;
    char _mode;
    char _value;
    struct pt t;
public:
    GpioPin(int pin)
    {
        _pin=pin;
        setMode('I');
        PT_INIT(&t);
//       setOutput('0'));
    }
    Flags topicObject(Cmd cmd,Strpack& str)
    {
        static Flags flags = {T_STR,M_WRITE,QOS_1,I_OBJECT,true};
        return flags;
    }
    Flags mode(enum Cmd cmd,Strpack& value)
    {
        static Flags flags = {T_STR,M_WRITE,QOS_1,I_OBJECT,true};
        switch(cmd)
        {
        case SET_VALUE :
        {
            setMode(value.peek(0));
            break;
        }
        case GET_VALUE :
        {
            value.append(_mode);
            break;
        }
        case  GET_META :
        {
            value.append("desc:'Direction for pin',set:['I','O']");
            break;
        }
        default:
        {
        }
        }
        return flags;
    }
    Flags input(enum Cmd cmd,Strpack& value)
    {
        static Flags flags = {T_STR,M_READ,QOS_1,I_OBJECT,true};
        switch(cmd)
        {
        case SET_VALUE :
        {
            break;
        }
        case GET_VALUE :
        case GET_ANY_UPDATE:
        {
            value.append(getInput());
            break;
        }
        case  GET_META :
        {
            value.append("desc:'Input for pin',set:['0','1']");
            break;
        }
        default:
        {
        }
        }
        return flags;
    }
    Flags output(enum Cmd cmd,Strpack& value)
    {
        static Flags flags = {T_STR,M_READ,QOS_1,I_OBJECT,true};
        switch(cmd)
        {
        case SET_VALUE :
        {
            setOutput(value.peek(0));
            break;
        }
        case GET_VALUE :
        case GET_ANY_UPDATE:
        {
            value.append(getInput());
            break;
        }
        case  GET_META :
        {
            value.append("desc:'Output for pin',set:['0','1']");
            break;
        }
        default:
        {
        }
        }
        return flags;
    }

    void setMode(char mode)
    {
        if ( mode=='O')
        {
            _mode='O';
            gpio.pinMode(_pin,OUTPUT);
        }
        else if ( mode=='I')
        {
            _value='I';
            gpio.pinMode(_pin,INPUT);
        }
        else Sys::logger("invalid pin value for GPIO");
    }
    void setOutput(char o)
    {
        if ( o=='1')
        {
            _value='1';
            gpio.digitalWrite(_pin,HIGH);
        }
        else if ( o=='0')
        {
            _value='1';
            gpio.digitalWrite(_pin,LOW);
        }
        else Sys::logger("invalid pin value for GPIO");
    }
    char getInput()
    {
        return "01"[gpio.digitalRead(_pin)];
    }


};

/*

prop(cmd,Strpack)

prop(SET_VALUE,flags,strp)
prop(GET_VALUE,flags,strp)
prop(GET_ANY_UPDATE,null)
prop(GET_META,strp)

*/




class SysObject : public TopicObject
{
public:
    SysObject() { }
    Flags topicObject(Cmd cmd,Strpack& str)
    {
        static Flags flags = {T_INT32,M_READ,QOS_0,I_OBJECT,false};
        return flags;
    }
    Flags memory(enum Cmd cmd,Strpack& value)
    {
        static Flags flags = {T_INT32,M_READ,QOS_0,I_OBJECT,false};
        if ( cmd == GET_VALUE )
        {
            value.append(getAllocSize());
        }
        else if (cmd == GET_DESC)
        {
            value.append("desc='allocated heap memory'");
        }
        else if ( cmd == GET_ANY_UPDATE )
        {
            value.append(getAllocSize());
        };
        return flags;
    }
};


SysObject sys;
GpioPin pin_1(1);
GpioPin pin_2(2);
GpioPin pin_3(3);

struct TopicEntry
{
    const char* const name;
    TopicObject& instance;
    TopicMethod method;
} topicList[]=
{
    { "system/memory"       ,sys    ,(TopicMethod)&SysObject::memory        }, // ff
    { "gpio/1/mode"         ,pin_1  ,(TopicMethod)&GpioPin::mode            }, // ff
    { "gpio/1/output"       ,pin_1  ,(TopicMethod)&GpioPin::output          }, // ff
    { "gpio/1/input"        ,pin_1  ,(TopicMethod)&GpioPin::input           }, // ff
    { "gpio/2/mode"         ,pin_2  ,(TopicMethod)&GpioPin::mode            }, // ff
    { "gpio/2/output"       ,pin_2  ,(TopicMethod)&GpioPin::output          }, // ff
    { "gpio/2/input"        ,pin_2  ,(TopicMethod)&GpioPin::input           }, // ff
    { "gpio/3/mode"         ,pin_3  ,(TopicMethod)&GpioPin::mode            }, // ff
    { "gpio/3/output"       ,pin_3  ,(TopicMethod)&GpioPin::output          }, // ff
    { "gpio/3/input"        ,pin_3  ,(TopicMethod)&GpioPin::input           }, // ff

};


class TopicListener : public Sequence
{
private:
    struct pt t;
    struct TopicEntry* topicEntry;
public:
    TopicListener()
    {
        PT_INIT(&t);
    }
    int handler(Event* event)
    {
        PropertySet* ps;
        const char* deviceName="pcdebian/";
        PT_BEGIN(&t);
        while(true)
        {
            PT_WAIT_UNTIL(&t,event->is(PROPERTY_SET));
            ps = (PropertySet*)event->data();
            Sys::getLogger().append(" PROPERTY SET : ")
            .append(&ps->_topic)
            .append(" :" )
            .append(&ps->_message)
            ;
            Sys::getLogger().flush();
            // strip device Name
            // define cmd based on extensions
            // remove extensions
            // find in topicObjects
            // execute method
            if ( ps->_topic.startsWith(deviceName))
            {
                Str topic(30);

                Cmd cmd=GET_ANY_UPDATE;
                ps->_topic.offset(strlen(deviceName));
                topic.sub(&ps->_topic,ps->_topic.length()-strlen(deviceName));
                int i;
                topicEntry=0;
                for(i=0; i<(sizeof(topicList)/sizeof(struct TopicEntry));i++)
                {
                    if ( topic.startsWith(topicList[i].name))
                    {
                        topicEntry = & topicList[i];
                        break;
                    }
                }
                if ( topicEntry == 0 ) PT_RESTART(&t);

                if ( ps->_topic.endsWith(".set"))
                {
                    cmd=SET_VALUE;
                    topic.length(topic.length()-4);
                    ((topicEntry->instance).*(topicEntry->method))(cmd,ps->_message);

                }
                else if ( topic.endsWith(".meta"))
                {Strpack strp(100);
                    cmd=GET_META;
                    topic.length(topic.length()-5);
                    ((topicEntry->instance).*(topicEntry->method))(cmd,strp);
                }
                else if ( topic.endsWith(".upd"))
                {
                    Strpack strp(100);
                    cmd=GET_VALUE;
                    topic.length(topic.length()-4);
                    ((topicEntry->instance).*(topicEntry->method))(cmd,strp);
                };
            }
            PT_YIELD(&t);
        };
        PT_END(&t);
    }

};


//_____________________________________________________________________________________________________
//
#include <unistd.h> // sleep
#include "Timer.h"

//_____________________________________________________________________________________________________
//

int main(int argc, char *argv[])
{
    Strpack strp;

    ((topicList[1].instance).*(topicList[1].method))(GET_VALUE,strp);
//    MqttThread mqtt("mqtt",1000,1);
    Queue::getDefaultQueue()->clear();//linux queues are persistent
    // static sequences
    Tcp tcp("tcp",1000,1);
    mqtt=new Mqtt(&tcp);
    MqttDispatcher mqttDispatcher(mqtt); // start dynamic sequences
    MqttPing pinger(mqtt); // ping cycle when connected
    new TopicListener();
//   PropertyListener pl(&mqtt); // publish property when changed or interval timeout
//   SysObject sys;
    GpioPin pin3(3);
    MainThread mt("messagePump",1000,1); // both threads could be combined
    TimerThread tt("timerThread",1000,1);
    tcp.start();
    mt.start();
    tt.start();


    ::sleep(1000000);

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












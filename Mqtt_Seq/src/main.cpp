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

EventId TIMER_TICK= Event::nextEventId(( char* const)"TIMER_TICK");
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

public :

    Mqtt(Tcp* tcp)
    {
        _tcp=tcp;
        PT_INIT(&t);
        _messageId=2000;
        mqttOut->prefix(Sys::getDeviceName());
    };

    int handler(Event* event)
    {
        PT_BEGIN(&t);
        while(1)
        {
            _messageId=nextMessageId();
            PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_CONNECTED));
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
            PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_DISCONNECTED));

            publish(this,MQTT_DISCONNECTED,0);
        }
        PT_END(&t);
    }

    Erc send(Bytes* pb)
    {
        return _tcp->send(pb);
    }

    bool isConnected()
    {
        _tcp->isConnected();
    };
    Erc disconnect()
    {
        _tcp->disconnect();
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
        uint8_t header=0;
        PT_BEGIN(&t);
        while(true)
        {
            while ( _mqtt->isConnected())
            {
                timeout(TIME_PING);
                mqttOut->PingReq();
                _mqtt->send(mqttOut);
                timeout(TIME_WAIT_REPLY);
                PT_WAIT_UNTIL(&t,timeout() || eventIsMqtt(event,MQTT_MSG_PINGRESP,0));
                if ( timeout())
                {
                    _mqtt->disconnect();
                    PT_RESTART(&t);
                }
                timeout(TIME_PING);
                PT_WAIT_UNTIL(&t,timeout() );   // sleep between pings
            }
            PT_YIELD(&t);
        }
        PT_END(&t);
    }
};


/*****************************************************************************
*   HANDLE MQTT send publish message with qos=2 and 1
******************************************************************************/
class MqttPubQos1 : public Sequence
{
private:
    struct pt t;
    Mqtt* _mqtt;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    MqttPubQos1(Mqtt* mqtt,Property* p)
    {
        _mqtt=mqtt;
        _messageId=nextMessageId();
        _p=p;
        PT_INIT(&t);
        //TODO in destructor free memory
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        PT_BEGIN(&t);
        if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;
        _message = new Strpack(100);
        _p->toPack(*_message);
        mqttOut->Publish(header+MQTT_QOS1_FLAG,_p->name(),_message,_messageId);
        _mqtt->send(mqttOut);
        delete _message;
        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBACK,_messageId));
        PT_END(&t);
    }
};

class MqttPubQos2 : public Sequence
{
private:
    struct pt t;
    Mqtt* _mqtt;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    MqttPubQos2(Mqtt* mqtt,Property* p)
    {
        _mqtt=mqtt;
        _messageId=nextMessageId();
        PT_INIT(&t);
        _p=p;
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        PT_BEGIN(&t);
        if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;
        _message = new Strpack(100);
        _p->toPack(*_message);
        mqttOut->Publish(header+MQTT_QOS2_FLAG,_p->name(),_message,_messageId);
        _mqtt->send(mqttOut);
        delete _message;
        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBREC,_messageId));

        mqttOut->PubRel(_messageId);
        _mqtt->send(mqttOut);
        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBCOMP,_messageId));
        PT_END(&t);
    }

};

/*****************************************************************************
*   HANDLE Property scan for changes
******************************************************************************/
class PropertyListener : public Sequence
{
private:
    struct pt t;
    Mqtt* _mqtt;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    PropertyListener(Mqtt* mqtt)
    {
        _mqtt=mqtt;

        PT_INIT(&t);
    }
    void publishCurrentProperty()
    {
        uint8_t header=0;
        if ( _p->flags().qos == QOS_0  )
        {
            if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;
            mqttOut->prefix(Sys::getDeviceName());
            _message = new Strpack(100);
            _p->toPack(*_message);
            mqttOut->Publish(header,_p->name(),_message,_messageId);
            _mqtt->send(mqttOut);
            delete _message;
        }
        else if ( _p->flags().qos == QOS_1  )
        {
            new MqttPubQos1(_mqtt,_p);
        }
        else if ( _p->flags().qos == QOS_2  )
        {
            new MqttPubQos2(_mqtt,_p);
        }
    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        PT_WAIT_UNTIL(&t,event->is(Tcp::TCP_CONNECTED));
        while(1)
        {
            timeout(TIME_WAIT_REPLY);
            PT_WAIT_UNTIL(&t,timeout());
            _p = Property::first();
            while(_p)
            {
                if ( _p->isUpdated() )
                {
                    publishCurrentProperty();
                    _p->published();
                }
                _p = Property::next(_p);
                timeout(100);
                PT_WAIT_UNTIL(&t,timeout());
            }
            timeout(TIME_BETWEEN_PROPERTIES);
            PT_WAIT_UNTIL(&t,timeout());
            Property::updatedAll();
        }
        PT_END(&t);
    }

};
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
        timeout(TIME_WAIT_REPLY);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,MQTT_MSG_PUBREL,_messageId));
        if ( timeout() )
        {
            delete clone;
            return PT_ENDED; // abandon and go for destruction
        }
        mqttOut->PubComp(_messageId);
        _mqtt->send(mqttOut);
        Property::set(clone->topic(),clone->message());
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
                    Property::set(mqttIn->topic(),mqttIn->message());

                    break;
                }
                case MQTT_QOS1_FLAG:
                {
                    _mqttOut->PubAck(mqttIn->messageId()); // send PUBACK
                    _mqtt->send(_mqttOut);
                    Property::set(mqttIn->topic(),mqttIn->message());

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
uint32_t memoryAllocated;
Property memoryAlloced(&memoryAllocated,(Flags)
{
    T_UINT32,M_READ,QOS_1,I_ADDRESS
},"system/memory_used","META");

void getAllocSize(void)
{
    struct mallinfo mi;

    mi = mallinfo();
    memoryAllocated =  mi.uordblks;
    Property::updated(&memoryAllocated);
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
            if ( event.data() != NULL )
                delete  (Bytes*)event.data();
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


//_____________________________________________________________________________________________________
//
#include <unistd.h> // sleep
#include "Timer.h"

//_____________________________________________________________________________________________________
//
int main(int argc, char *argv[])
{
//    MqttThread mqtt("mqtt",1000,1);
    Queue::getDefaultQueue()->clear();//linux queues are persistent
    Tcp tcp("tcp",1000,1);
    Mqtt mqtt(&tcp);
    MqttDispatcher mqttDispatcher(&mqtt);
    MqttPing pinger(&mqtt);
    PropertyListener pl(&mqtt);
//   Mqtt mqtt(&tcp);
//   MqttPublisher mqttPublisher;

//   mqttPublisher.init(&mqtt);
//   MqttSubscriber mqttSubscriber;
//   mqttSubscriber.init(&mqtt);


    MainThread mt("messagePump",1000,1);
    TimerThread tt("timerThread",1000,1);
    tcp.start();
    mt.start();
    tt.start();

    ::sleep(1000000);

}











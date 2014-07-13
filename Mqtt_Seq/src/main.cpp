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
//____________________________________________________________________________________________

#include "pt.h"

uint16_t gMessageId=1;
MqttOut* mqttOut=new MqttOut(256);

uint16_t nextMessageId()
{
    return gMessageId++;
}


bool eventIsMqtt(Event* event,Tcp* tcp,uint8_t type, uint16_t messageId)
{
    if (event->src()==tcp)
        if ( event->id() == Tcp::TCP_RXD )
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
*   HANDLE MQTT onnection and subscribe sequence
******************************************************************************/

class MqttConnectSeq : public Sequence
{
private:
    struct pt t;
    Tcp* _tcp;
    uint16_t _messageId;

public :
    enum MqttEvents {MQTT_CONNECTED=EV('M','Q','_','C'), MQTT_DISCONNECTED=EV('M','Q','_','D')};
    MqttConnectSeq(Tcp* tcp)
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
                             "system/online", "false", "", "", 1000);
            _tcp->send(mqttOut);
            timeout(1000);
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,_tcp,MQTT_MSG_CONNACK,0) || timeout() );
            if ( timeout() )
            {
                _tcp->disconnect();
                publish(new Event(this,MQTT_DISCONNECTED,0));
                continue;
            }

            timeout(1000);
            mqttOut->Subscribe(0,"#",_messageId,MQTT_QOS1_FLAG);
            _tcp->send(mqttOut);
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,_tcp,MQTT_MSG_SUBACK,_messageId) || timeout());
            if ( timeout() )
            {
                _tcp->disconnect();
                publish(new Event(this,MQTT_DISCONNECTED,0));
                continue;
            }

            publish(new Event(this,MQTT_CONNECTED,0));
            PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_DISCONNECTED));

            publish(new Event(this,MQTT_DISCONNECTED,0));
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
    Tcp* _tcp;
    uint16_t _messageId;
    MqttIn* clone;
public:
    MqttSubQos2(Tcp* tcp,uint16_t messageId)
    {
        _tcp=tcp;
        _messageId=messageId;
        PT_INIT(&t);
    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        PT_WAIT_UNTIL(&t,eventIsMqtt(event,_tcp,MQTT_MSG_PUBLISH,_messageId));
        // no timeout since the sequence is created because of event is there
        mqttOut->PubRec(_messageId);
        _tcp->send(mqttOut);
        //TODO clone message
        clone = new MqttIn(*((MqttIn*)(event->data())));
        timeout(1000);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBREL,_messageId));
        if ( timeout() )
        {
            Sys::free(clone);
            return PT_ENDED; // abandon and go for destruction
        }
        mqttOut->PubComp(_messageId);
        _tcp->send(mqttOut);
        Property::set(clone->topic(),clone->message());
        Sys::free(clone); // free cloned messages
        PT_END(&t);
    }

};

/*****************************************************************************
*   HANDLE MQTT send publish message with qos=2
******************************************************************************/
class MqttPubQos1 : public Sequence
{
private:
    struct pt t;
    Tcp* _tcp;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    MqttPubQos1(Tcp* tcp,Property* p)
    {
        _tcp=tcp;
        _messageId=nextMessageId();
        _message = new Strpack(100);
        _p=p;
        PT_INIT(&t);
        //TODO in destructor free memory
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        PT_BEGIN(&t);
        if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;
        _p->toPack(*_message);
        mqttOut->Publish(header+MQTT_QOS1_FLAG,_p->name(),_message,_messageId);
        _tcp->send(mqttOut);
        Sys::free(_message);
        timeout(1000);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBACK,_messageId));
        PT_END(&t);
    }
};

class MqttPubQos2 : public Sequence
{
private:
    struct pt t;
    Tcp* _tcp;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    MqttPubQos2(Tcp* tcp,Property* p)
    {
        _tcp=tcp;
        _messageId=nextMessageId();
        _message = new Strpack(100);
        _p=p;
    }
    int handler(Event* event)
    {
        uint8_t header=0;
        PT_BEGIN(&t);
        if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;

        _p->toPack(*_message);
        mqttOut->Publish(header+MQTT_QOS2_FLAG,_p->name(),_message,_messageId);
        _tcp->send(mqttOut);
        Sys::free(_message);
        timeout(1000);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBREC,_messageId));

        mqttOut->PubRel(_messageId);
        _tcp->send(mqttOut);
        timeout(1000);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBCOMP,_messageId));
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
    Tcp* _tcp;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    PropertyListener(Tcp* tcp)
    {
        _tcp=tcp;
        _message = new Strpack(100);
        PT_INIT(&t);
    }
    void publishCurrentProperty()
    {
        uint8_t header=0;
        if ( _p->flags().qos == QOS_0  )
        {
            if ( _p->flags().retained ) header += MQTT_RETAIN_FLAG;
            mqttOut->prefix(Sys::getDeviceName());
            _p->toPack(*_message);
            mqttOut->Publish(header,_p->name(),_message,_messageId);
            _tcp->send(mqttOut);
        }
        else if ( _p->flags().qos == QOS_1  )
        {
            new MqttPubQos1(_tcp,_p);
        }
        else if ( _p->flags().qos == QOS_2  )
        {
            new MqttPubQos2(_tcp,_p);
        }



    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_CONNECTED));
        while(1)
        {
            timeout(1000);
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
            timeout(1000);
            PT_WAIT_UNTIL(&t,timeout());
            Property::updatedAll();
        }
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
    Tcp* _tcp;
    MqttOut* _mqttOut;
    uint16_t _messageId;

public :
    MqttDispatcher(Tcp* tcp)
    {
        _mqttOut = new MqttOut(100);
        _tcp=tcp;
        PT_INIT(&t);
        _messageId=3000;
    };


    int handler(Event* event)
    {
        MqttIn* mqttIn;
        PT_BEGIN(&t);
        while(1)
        {
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,_tcp,MQTT_MSG_PUBLISH,0));
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
                    _tcp->send(_mqttOut);
                    Property::set(mqttIn->topic(),mqttIn->message());
                    break;
                }
                case MQTT_QOS2_FLAG:
                {
                    new MqttSubQos2(_tcp,mqttIn->messageId()); // new sequence to handle
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

        while(true)
        {
            Event event;
            Queue::getDefaultQueue()->get(&event);
            union
            {
                uint32_t i;
                char c[4];
            } u;
            u.i = event.id();
            std::cout<< Sys::upTime() << " | "<<u.c<<std::endl;

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
                Sys::free(event.data());
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
/*
*

ST_WAIT_PUBACK
    PUBACK , mId ok -> getNext, ST_NEXT_PROPERTY
    TIMEOUT -> retry < 3 -> send again : ST_WAIT_PUBACK
    * : ST_SLEEP
ST_WAIT_PUBREC
    PUBREC , mID ok -> ST_WAIT_PUBCOMP
    TIMEOUT -> retry < 3
    * : ignore
ST_WAIT_PUBCOMP
    PUBCOMP
    TIMEOUT
    * : ignore -> ST_NEXT_PROPERTY
ST_NEXT_PROPERTY :
    TIMEOUT : publishProperty
    *



*/

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
    MqttConnectSeq mqtt(&tcp);
    MqttDispatcher mqttDispatcher(&tcp);
    PropertyListener pl(&tcp);
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










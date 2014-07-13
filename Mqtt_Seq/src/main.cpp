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

MqttOut* mqttOut=new MqttOut(256);
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
    enum MqttEvents {MQTT_CONNECTED,MQTT_DISCONNECTED};
    MqttConnectSeq(Tcp* tcp)
    {
        _tcp=tcp;
        PT_INIT(&t);
        _messageId=2000;
    };

    int handler(Event* event)
    {
        PT_BEGIN(&t);
        while(1)
        {
            _messageId++;
            PT_WAIT_UNTIL(&t,event->is(_tcp,Tcp::TCP_CONNECTED));
            mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION,
                             "ikke/alive", "false", "", "", 1000);
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
            PT_WAIT_UNTIL(&t,eventIsMqtt(event,_tcp,MQTT_MSG_SUBACK,0) || timeout());
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
        if ( timeout() ) return PT_ENDED; // abandon and go for destruction
        mqttOut->PubComp(_messageId);
        _tcp->send(mqttOut);
        //TODO setter
        //TODO delete cloned message
        Sys::free(clone);
        PT_END(&t);
    }

};

/*****************************************************************************
*   HANDLE MQTT send publish message with qos=2
******************************************************************************/

class MqttPubQos2 : public Sequence
{
private:
    struct pt t;
    Tcp* _tcp;
    uint16_t _messageId;
    Property * _p;
    Strpack* _message;
public:
    MqttPubQos2(Tcp* tcp,uint16_t messageId,Property* p)
    {
        _tcp=tcp;
        _messageId=messageId;
        _message = new Strpack(100);
        _p=p;
        //TODO in destructor free memory
    }
    int handler(Event* event)
    {
        PT_BEGIN(&t);
        _p->toPack(*_message);
        mqttOut->Publish(MQTT_QOS2_FLAG,_p->name(),_message,_messageId);
        _tcp->send(mqttOut);
        Sys::free(_message);
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBREC,_messageId));
        mqttOut->PubRel(_messageId);
        _tcp->send(mqttOut);
        //TODO clone message
        PT_WAIT_UNTIL(&t,timeout() ||  eventIsMqtt(event,_tcp,MQTT_MSG_PUBCOMP,_messageId));
        //TODO setter
        //TODO delete cloned message
        PT_END(&t);
    }

};
/*****************************************************************************
*   HANDLE MQTT received publish message with qos=2
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
                    //TODO setter
                    break;
                }
                case MQTT_QOS1_FLAG:
                {
                    // puback
                    _mqttOut->PubAck(mqttIn->messageId());
                    _tcp->send(_mqttOut);
                    //TODO setter
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
        Queue::getDefaultQueue()->clear();
        while(true)
        {
            Event event;
            Queue::getDefaultQueue()->get(&event);

            int i;
            for(i=0; i<MAX_SEQ; i++)
                if(Sequence::activeSequence[i])
                {
                    if ( Sequence::activeSequence[i]->handler(&event) == PT_ENDED )
                    {
                        Sequence::activeSequence[i]->unreg();
                        delete Sequence::activeSequence[i];
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
bool alive=true;
Property property1(&p, (Flags)
{
    T_UINT32,M_READ,QOS_1,I_ADDRESS
}, "ikke/P","$META");
Property property2(&alive, (Flags)
{
    T_BOOL,M_READ,QOS_2,I_ADDRESS
}, "ikke/alive","$META");

const PropertyConst pc= { {(void*)&p},"NAME","META",{
        T_BOOL,M_READ,QOS_1,I_ADDRESS
    }
};

Property pp(&pc);


#include <unistd.h> // gethostname
char* getDeviceName()
{
    static    char hostname[20]="";

    gethostname(hostname,20);
    strcat(hostname,"/");
    return hostname;
}

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
#define TIME_BETWEEN_PROPERTY   100
#define TIME_BETWEEN_FULL_CYCLE 5000
class MqttPublisher : public TimerListener,public MqttListener
{
private:

    Mqtt* _mqtt;
    bool _isConnected;
    Timer* _timer;
    Property* _currentProperty;
    enum State { ST_DISCONNECTED, ST_SLEEP, ST_PUB_PROPERTY, ST_WAIT_PUBACK, ST_WAIT_PUBREC,ST_WAIT_PUBCOMP} _state;
    uint16_t _retryCount;
    uint16_t _currentMessageId;
    MqttOut* _mqttOut;
    uint8_t _currentQos;
    uint32_t _errorCount;


public:
    MqttPublisher()
    {
        _isConnected=false;
        _timer=new Timer();
        _currentProperty = & property2;
        _currentMessageId=1000;
        _retryCount=0;
        _mqttOut = new MqttOut(256);
        _errorCount=0;

    };
    void init(Mqtt* mqtt)
    {
        _mqtt = mqtt;
        _mqtt->addListener(this);
//        _tcp=tcp;
//       _tcp->addListener(this);
        _timer->addListener(this);

    }
    bool getNextProperty()
    {
        _currentMessageId++;
        _currentProperty = Property::next(_currentProperty);
        if (_currentProperty == NULL ) return false;
        return true;
    }
    bool getFirstProperty()
    {
        _currentMessageId++;
        _currentProperty = Property::first();
        if (_currentProperty == NULL ) return false;
        return true;
    }
    void onTimerExpired(Timer* timer)
    {
        if ( _state == ST_WAIT_PUBACK )
        {
            if ( _retryCount++< 3 )
                publishCurrentProperty();
            else _state=ST_PUB_PROPERTY;
        }
        else if ( _state == ST_WAIT_PUBREC || _state==ST_WAIT_PUBCOMP )
        {
            _state=ST_PUB_PROPERTY;
        }
        else if (_state==ST_PUB_PROPERTY )
        {
            if ( getNextProperty() )
                publishCurrentProperty();
            else
                _state=ST_SLEEP;
        }
        else if ( _state == ST_SLEEP )
        {
            if ( getFirstProperty() )
                publishCurrentProperty();
            else
                _state=ST_SLEEP;
        }
        else if (_state==ST_DISCONNECTED)
        {

        }
        else
        {
            _errorCount++;
        }
        if (_state == ST_SLEEP || _state==ST_DISCONNECTED ) _timer->startOnce(TIME_BETWEEN_FULL_CYCLE);
        else _timer->startOnce(TIME_BETWEEN_PROPERTY);
    }
    void onMqttMessage(Mqtt* src,MqttIn* msg)
    {
        if ( _state == ST_WAIT_PUBACK &&  msg->type() == MQTT_MSG_PUBACK && msg->messageId() == _currentMessageId )
        {
            _timer->stop();
            _state = ST_PUB_PROPERTY;
        }
        else if ( _state == ST_WAIT_PUBREC &&  msg->type() == MQTT_MSG_PUBREC && msg->messageId() == _currentMessageId )
        {
            _mqttOut->PubRel(_currentMessageId);
            _mqtt->send(_mqttOut);
            _state = ST_WAIT_PUBCOMP;
        }
        else if ( _state == ST_WAIT_PUBCOMP &&  msg->type() == MQTT_MSG_PUBCOMP && msg->messageId() == _currentMessageId )
        {
            _state = ST_PUB_PROPERTY;
        }
        else if (msg->type() == MQTT_MSG_PUBACK  || msg->type() == MQTT_MSG_PUBREC  || msg->type() == MQTT_MSG_PUBCOMP  )
        {
            _errorCount++;
        } // else not interested
        _timer->startOnce(TIME_BETWEEN_PROPERTY);
    }
    void onMqttConnect(Mqtt* src)
    {
        _isConnected = true;
        _state = ST_SLEEP;
        _timer->startOnce(TIME_BETWEEN_PROPERTY);
        _retryCount=0;
    };
    void onMqttDisconnect(Mqtt* src)
    {
        _isConnected = false;
        _state = ST_DISCONNECTED;
    };


    void publishCurrentProperty()
    {
        uint8_t header=0;
        if ( _currentProperty->flags().qos == QOS_0  )
        {
            header = 0;
            _state = ST_PUB_PROPERTY;
        }
        else if ( _currentProperty->flags().qos == QOS_1  )
        {
            header = MQTT_QOS1_FLAG;
            _state = ST_WAIT_PUBACK;
        }
        else if ( _currentProperty->flags().qos == QOS_2  )
        {
            header = MQTT_QOS2_FLAG;
            _state = ST_WAIT_PUBREC;
        }
        if ( _currentProperty->flags().retained )
            header += MQTT_RETAIN_FLAG;

        _mqttOut->prefix(getDeviceName());
        Strpack  message(20);
        _currentProperty->toPack(message);

        _mqttOut->Publish(header,_currentProperty->name(),&message,_currentMessageId);
        _mqtt->send(_mqttOut);
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
//    MqttThread mqtt("mqtt",1000,1);
    Tcp tcp("tcp",1000,1);
    MqttConnectSeq mqtt(&tcp);
    MqttDispatcher mqttDispatcher(&tcp);
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










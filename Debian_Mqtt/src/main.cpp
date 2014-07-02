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
#include <time.h>
#include "Timer.h"
#include "Publisher.h"
#include "CircBuf.h"
#include "Mqtt.h"
#include "Tcp.h"
#include "Property.h"
#include "Strpack.h"
class TimerThread : public Thread
{

public:

    TimerThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
public :
    void run()
    {
        while(true)
        {

            struct timespec deadline;
            clock_gettime(CLOCK_MONOTONIC, &deadline);

// Add the time you want to sleep
            deadline.tv_nsec += 1000;

// Normalize the time to account for the second boundary
            if(deadline.tv_nsec >= 1000000000)
            {
                deadline.tv_nsec -= 1000000000;
                deadline.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
            Timer::decAll();
        }
    }
};

//_________________________________________________________
// Queue q(sizeof(Event),10);

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
            Stream::getDefaultQueue()->get(&event);
            Listener* listener = Stream::getListeners();
            while(listener!=NULL)
            {
                if ( event.src() == listener->src )
                    if (( listener->id== -1) || ( listener->id == event.id()))
                        listener->dst->eventHandler(&event);
                listener = listener->next;
            }
            if ( event.data() != NULL )
                Sys::free(event.data());
        }
    }
};
//____________________________________________________________________________________________


//__________________________________________________________________________________
//
class Mqtt;

class MqttListener
{
public:
    virtual void onMqttMessage(Mqtt* mqtt,MqttIn* msg)=0;
    virtual void onMqttConnect(Mqtt* mqtt)=0;
    virtual void onMqttDisconnect(Mqtt* mqtt)=0;
};


class Mqtt : public TcpListener,public TimerListener,public Publisher<MqttListener>

{
private:

    Tcp* _tcp;
    MqttOut* _mqttOut;
    Timer* _timer;
    MqttListener* _listener;

public:
    enum { MQTT_CONNECTED=100, MQTT_DISCONNECTED } Events;

    Mqtt( Tcp* tcp)
    {
        _tcp=tcp;
        _tcp->addListener(this);
        _timer =  new Timer();
        _timer->addListener(this);
        _mqttOut = new MqttOut(256);
    }
    void onTcpConnect(Tcp* src)
    {
        _mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION,
                          "ikke/alive", "false", "", "", 1000);
        _tcp->send(_mqttOut);
    }
    void onTcpDisconnect(Tcp* src)
    {
        _listener->onMqttDisconnect(this);
    }
    void onTcpMessage(Tcp* src,Bytes* data)
    {
        MqttIn *packet=(MqttIn*)data;
        if ( packet->type() == MQTT_MSG_CONNACK)
        {
            if ( packet->_returnCode == MQTT_RTC_CONN_ACC)
            {
                _listener->onMqttConnect(this);
            }
            else
            {
                std::cerr << "Mqtt Connect failed , return code : " << packet->_returnCode;
                _tcp->disconnect();
            }
        }
        else
        {
            _listener->onMqttMessage(this,packet);
        }
    };
    void onTimerExpired(Timer* timer)
    {
    }

};
//__________________________________________________________________________________
//

uint32_t p=1234;
Property property(&p, T_INT32, M_READ, "ikke/P","$META");

//_____________________________________________________________________________________________________
class MqttPublisher : public TimerListener,public MqttListener
{
private:

    Tcp* _tcp;
    Mqtt* _mqtt;
    bool _isConnected;
    Timer* _timer;
    Property* _currentProperty;
    enum { ST_SLEEP, ST_WAIT_ACK, } _state;
    uint16_t _retryCount;
    uint16_t _messageId;

public:
    MqttPublisher()
    {
        _isConnected=false;
        _timer=new Timer();
        _currentProperty = & property;
        _messageId=1000;
        _retryCount=0;

    };
    void init(Mqtt* mqtt)
    {
        _mqtt = mqtt;
        _mqtt->addListener(this);
//        _tcp=tcp;
//       _tcp->addListener(this);
        _timer->addListener(this);

    }
    void onTimerExpired(Timer* timer)
    {
        if ( _state == ST_WAIT_ACK )
        {
            if ( _retryCount++< 3 )
                publishCurrentProperty();
        }
    }
    void onMqttMessage(Mqtt* src,MqttIn* msg)
    {
        if ( msg->type() == MQTT_MSG_PUBACK)
        {
            _timer->stop();
            _state = ST_SLEEP;

        }
        else if ( msg->type() == MQTT_MSG_PUBREC)
        {

        }
        else if ( msg->type() == MQTT_MSG_PUBCOMP)
        {

        }
    }
    void onMqttConnect(Mqtt* src)
    {
        _isConnected = true;

        _state = ST_WAIT_ACK;
        _retryCount=0;
    };
    void onMqttDisconnect(Mqtt* src)
    {
        _isConnected = false;
        _state = ST_SLEEP;
    };


    void publishCurrentProperty()
    {
        MqttOut out(256);
        Strpack  message(20);
        Str topic(20);
        topic.append(_currentProperty->name());
        _currentProperty->toPack(message);
        out.Publish(MQTT_QOS1_FLAG,&topic,&message,123);
        _tcp->send(&out);
        _timer->start(1000);
    }
};
//_____________________________________________________________________________________________________
//
class MqttSubscriber : public MqttListener
{
private:

    Tcp* _tcp;
    Mqtt* _mqtt;
    bool _isConnected;
    uint16_t _messageId;
    MqttOut* _mqttOut;
    Str* _tempTopic;
    Str* _tempMessage;
    enum { DISCONNECTED, SUBSCRIBING , SUBSCRIBED } _state;

public:
    MqttSubscriber()
    {
        _isConnected=false;
        _messageId=1000;
        _mqttOut = new MqttOut(256);
        _tempTopic = new Str(30);
        _tempMessage = new Str(100);
    };
    void init(Mqtt* mqtt)
    {
        _mqtt = mqtt;
        _mqtt->addListener(this);
//        _tcp=tcp;
//       _tcp->addListener(this);
    }

    void onTimerExpired(Timer* timer)
    {

    }

    void onMqttMessage(Mqtt* src,MqttIn* msg)
    {
        if ( msg->type() == MQTT_MSG_SUBACK)
        {
            _state=SUBSCRIBED;
        }
        else if ( msg->type() == MQTT_MSG_PUBLISH)
        {
            Str topic(100);
            Str message(100);
            topic.write(&msg->_topic);
            message.write(&msg->_message);
            std::cout << topic.data() << ":" << message.data() << " QOS " << (msg->_header & MQTT_QOS_MASK )<< std::endl;
            if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS1_FLAG )
            {
                _mqttOut->PubAck(msg->messageId());
                _tcp->send(_mqttOut);
                // execute setter
            }
            else if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS2_FLAG )
            {
                _mqttOut->PubRec(msg->messageId());
                _tcp->send(_mqttOut);
                _tempTopic->clear();
                _tempMessage->clear();
                _tempTopic->write(&msg->_topic);
                _tempMessage->write(&msg->_message);
            }
            else
            {
                // execute setter
            }

        }
        else if ( msg->type() == MQTT_MSG_PUBREL)
        {
            // execute setter with _temp
            std::cout << " PUBREL " << _tempTopic->data() << ":" << _tempMessage->data() << std::endl;
            _mqttOut->PubComp(msg->messageId());
            _tcp->send(_mqttOut);
        }
    }
    void onMqttConnect(Mqtt* src)
    {
        _state = SUBSCRIBING;
        _isConnected = true;
        MqttOut out(256);
        out.Subscribe(0, "ikke/#", ++ _messageId, MQTT_QOS1_FLAG);
        _tcp->send(&out);
    };
    void onMqttDisconnect(Mqtt* src)
    {
        _isConnected = false;
        _state = DISCONNECTED;
    };

};
#include <unistd.h> // sleep
//_____________________________________________________________________________________________________
//
int main(int argc, char *argv[])
{
//    MqttThread mqtt("mqtt",1000,1);
    Tcp tcp("tcp",1000,1);
    Mqtt mqtt(&tcp);
    MqttPublisher mqttPublisher;

    mqttPublisher.init(&mqtt);
    MqttSubscriber mqttSubscriber;
    mqttSubscriber.init(&mqtt);
    TimerThread tt("TimerThread",1000,1);
    MainThread mt("messagePump",1000,1);


    ::sleep(1000000);

}

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


//__________________________________________________________________________________
//

//__________________________________________________________________________________
//

uint32_t p=1234;
bool alive=true;
Property property1(&p, T_INT32, M_READ, "ikke/P","$META");
Property property2(&alive, T_BOOL, M_READ, "ikke/alive","$META");
#include <unistd.h> // gethostname
char* getDeviceName()
{
    static    char hostname[20]="";

    gethostname(hostname,20);
    strcat(hostname,"/");
    return hostname;
}

//_____________________________________________________________________________________________________
class MqttPublisher : public TimerListener,public MqttListener
{
private:

    Mqtt* _mqtt;
    bool _isConnected;
    Timer* _timer;
    Property* _currentProperty;
    enum { ST_SLEEP, ST_WAIT_ACK, } _state;
    uint16_t _retryCount;
    uint16_t _messageId;
    MqttOut* _mqttOut;



public:
    MqttPublisher()
    {
        _isConnected=false;
        _timer=new Timer();
        _currentProperty = & property1;
        _messageId=1000;
        _retryCount=0;
        _mqttOut = new MqttOut(256);

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
        else if (_state==ST_SLEEP )
        {
            publishCurrentProperty();
        _state = ST_WAIT_ACK;
        }
    }
    void onMqttMessage(Mqtt* src,MqttIn* msg)
    {
        if ( msg->type() == MQTT_MSG_PUBACK)
        {
            _timer->stop();
            _state = ST_SLEEP;
            _messageId++;
            _timer->startOnce(1000);

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
        publishCurrentProperty();
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
        _mqttOut->prefix(getDeviceName());
        Strpack  message(20);
        Str topic(20);
        topic.append(_currentProperty->name());
        _currentProperty->toPack(message);
        _mqttOut->Publish(MQTT_QOS1_FLAG,&topic,&message,_messageId);
        _mqtt->send(_mqttOut);
        _timer->startOnce(1000);
        p++;
    }
};
//_____________________________________________________________________________________________________
//
class MqttSubscriber : public MqttListener
{
private:
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
        _mqttOut->prefix(getDeviceName());
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
            Sys::getLogger().clear();
            Sys::getLogger().append(topic.data()).append(":");
            Sys::getLogger().append(message.data()).
            append(" QOS ").append((msg->_header & MQTT_QOS_MASK ));
            Sys::getLogger().flush();
            if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS1_FLAG )
            {
                _mqttOut->PubAck(msg->messageId());
                _mqtt->send(_mqttOut);
                // execute setter
            }
            else if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS2_FLAG )
            {
                _mqttOut->PubRec(msg->messageId());
                _mqtt->send(_mqttOut);
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
            _mqtt->send(_mqttOut);
        }
    }
    void onMqttConnect(Mqtt* src)
    {
        _state = SUBSCRIBING;
        _isConnected = true;
        MqttOut out(256);
        out.prefix(getDeviceName());
        out.Subscribe(0, "ikke/#", ++ _messageId, MQTT_QOS1_FLAG);
        _mqtt->send(&out);
    };
    void onMqttDisconnect(Mqtt* src)
    {
        _isConnected = false;
        _state = DISCONNECTED;
    };

};
#include <unistd.h> // sleep
#include "Timer.h"
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


    MainThread mt("messagePump",1000,1);
    TimerThread tt("timerThread",1000,1);
    tcp.start();
    mt.start();
    tt.start();

    ::sleep(1000000);

}

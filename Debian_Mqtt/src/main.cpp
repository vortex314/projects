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
        Str topic(20);
        topic.append(_currentProperty->name());
        _currentProperty->toPack(message);

        _mqttOut->Publish(header,&topic,&message,_currentMessageId);
        _mqtt->send(_mqttOut);
    }
};
//_____________________________________________________________________________________________________
//
#include <unistd.h> // sleep
#include "Timer.h"
class MqttSubscriber : public MqttListener
{
private:
    Mqtt* _mqtt;
    bool _isConnected;
    uint16_t _messageId;
    MqttOut* _mqttOut;
    Str* _tempTopic;
    Str* _tempMessage;
    uint16_t _tempMessageId;
    enum { ST_DISCONNECTED, ST_SUBSCRIBING , ST_SUBSCRIBED,ST_WAIT_PUBLISH, ST_WAIT_PUBREL } _state;
    Timer* _timer;
    uint32_t _errorCount;

public:
    MqttSubscriber()
    {
        _isConnected=false;
        _messageId=1000;
        _mqttOut = new MqttOut(256);
        _mqttOut->prefix(getDeviceName());
        _tempTopic = new Str(30);
        _tempMessage = new Str(100);
        _timer = new Timer();
        _errorCount=0;
    };
    void init(Mqtt* mqtt)
    {
        _mqtt = mqtt;
        _mqtt->addListener(this);
        _state = ST_DISCONNECTED;
//        _tcp=tcp;
//       _tcp->addListener(this);
    }

    void propertySetter(MqttIn* msg)
    {
        Str topic(100);
        Str message(100);
        topic.write(&msg->_topic);
        message.write(&msg->_message);
        propertySetter(&msg->_topic,&msg->_message);
    }

    void propertySetter(Str* topic,Str* message)
    {
        Str log(100);
        log.clear();
        log.append(" SET ");
        log.write(topic);
        log.append(" = " );
        log.write(message);

        Sys::getLogger().clear();
        Sys::getLogger().append(&log);
        Sys::getLogger().flush();
    }

    void doSubcribe()
    {
        MqttOut out(256);
        out.prefix(getDeviceName());
        out.Subscribe(0, "#", ++ _messageId, MQTT_QOS1_FLAG);
        _mqtt->send(&out);
    }

    void onTimerExpired(Timer* timer)
    {
        doSubcribe();
        _timer->startOnce(TIME_BETWEEN_PROPERTY);
    }

    void onMqttMessage(Mqtt* src,MqttIn* msg)
    {
        if ( msg->type() == MQTT_MSG_SUBACK)
        {
            _timer->stop();
            _state=ST_WAIT_PUBLISH;
        }
        else if ( msg->type() == MQTT_MSG_PUBLISH && _state == ST_WAIT_PUBLISH )
        {
            _messageId = msg->messageId();

            if ( ( msg->_header & MQTT_QOS_MASK )== 0  )
            {
                propertySetter(msg);
            }
            else if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS1_FLAG )
            {
                propertySetter(msg);

                _mqttOut->PubAck(_messageId);
                _mqtt->send(_mqttOut);

            }
            else if (( msg->_header & MQTT_QOS_MASK )== MQTT_QOS2_FLAG )
            {
                _mqttOut->PubRec(_messageId);
                _mqtt->send(_mqttOut);

                _tempTopic->clear();
                _tempMessage->clear();
                _tempTopic->write(&msg->_topic);
                _tempMessage->write(&msg->_message);
                _tempMessageId = _messageId;

                _state = ST_WAIT_PUBREL;
            }
            else
            {
                _errorCount++;
                _state= ST_WAIT_PUBLISH;
            }

        }
        else if ( (msg->type() == MQTT_MSG_PUBREL)
                  && (_state == ST_WAIT_PUBREL)
                  && (_tempMessageId == msg->messageId()))
        {
            // execute setter with _temp
            _mqttOut->PubComp(msg->messageId());
            _mqtt->send(_mqttOut);
            propertySetter(_tempTopic,_tempMessage);
            _state = ST_WAIT_PUBLISH;
        }
        else if ( msg->type() == MQTT_MSG_PUBREL || msg->type() == MQTT_MSG_PUBLISH )
        {
            _errorCount++;
            _state=ST_WAIT_PUBLISH;
        }
    }
    void onMqttConnect(Mqtt* src)
    {
        _state = ST_SUBSCRIBING;
        _isConnected = true;
        doSubcribe();
        _timer->startOnce(TIME_BETWEEN_PROPERTY);
    };
    void onMqttDisconnect(Mqtt* src)
    {
        _isConnected = false;
        _state = ST_DISCONNECTED;
        _timer->stop();
    };

};

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





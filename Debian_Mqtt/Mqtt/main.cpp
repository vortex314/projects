#include <iostream>
#include "Bytes.h"
#include "Str.h"
#include "MqttIn.h"
#include "MqttOut.h"



#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include "Queue.h"
#include "Event.h"
#include "Thread.h"
#include "Stream.h"
#include <iostream>
#include <list>

extern "C" void* pvTaskCode(void *pvParameters)
{
    (static_cast<Thread*>(pvParameters))->run();
    return NULL;
}

Thread::Thread(const char *name, unsigned short stackDepth, char priority)
{
    _ref = Sys::malloc(sizeof(pthread_t));
    pthread_t* pThread = (pthread_t*)_ref;
    /* create threads */
    pthread_create (pThread, NULL,  pvTaskCode, (void *) this);
}

void Thread::run()
{
    while(true)
    {
        ::sleep(10);
    }
}
void Thread::sleep(uint32_t time)
{
    ::sleep(time/1000);
}

template <class TL>
class Publisher
{private :
    typedef struct ListenerStruct {
        struct ListenerStruct *next;
        TL* listener;
    } ListenerList;
public:
    void addListener(TL* listener)
    {
        _listener = listener;
    }

private:
    ListenerList _first;
};


//______________________________________________________________________________________
#include <time.h>
#define MAX_TIMERS 10
class Timer;
class TimerListener
{
public:
    virtual void onTimerExpired(Timer* src)=0;
};
class Timer: public Publisher<TimerListener>
{
public:


    Timer();
    ~Timer();
    Timer(Stream* stream);
//   void addListener(TimerListener* tl);
    Timer(const Timer& ref);
    Timer(uint32_t value, bool reload);
    void interval(uint32_t interval);
    void start();
    void start(uint32_t interval);
    void stop();
    void reload(bool automatic);
    void dec();
    Erc event(Event* event);
    static void decAll();
    bool expired();

private:
    static Timer* timers[MAX_TIMERS];
    static int timerCount;
    uint32_t _counterValue;
    uint32_t _reloadValue;
    bool _isAutoReload;
    bool _isActive;
    bool _isExpired;
    TimerListener* _listener;
};


int Timer::timerCount=0;
Timer* Timer::timers[MAX_TIMERS];

void Timer::decAll()
{
    uint32_t i;
    for (i = 0; i < timerCount; i++)
        timers[i]->dec();
}
/*
void Timer::addListener(TimerListener* tl){
    _listener = tl;
}
*/

Timer::Timer()
{
    //register timer
    timers[timerCount++] = this;
    _isActive = false;
    _isAutoReload = false;
    _reloadValue = 1000;
    _counterValue = 1000;
    _isExpired = false;
}

Timer::Timer(uint32_t value, bool reload)
{
    _isAutoReload = reload;
    _isActive = false;
    _reloadValue = value;
    _counterValue = value;
    timers[timerCount++] = this;
    _isExpired = false;
}

void Timer::interval(uint32_t interval)
{
    _reloadValue = interval;
    _counterValue = interval;
}

void Timer::start()
{
    _isActive = true;
    _counterValue = _reloadValue;
    _isExpired = false;
}

bool Timer::expired()
{
    return _isExpired;
}

void Timer::start(uint32_t value)
{
    interval(value);
    start();
}

void Timer::stop()
{
    _isActive = false;
}

void Timer::reload(bool automatic)
{
    _isAutoReload = automatic;
}

void Timer::dec()
{
    if (_isActive)
        if (--_counterValue == 0)
        {
            _listener->onTimerExpired(this);
            _isExpired = true;
            if (_isAutoReload)
                _counterValue = _reloadValue;
            else
                _isActive = false;
        }
}

Erc Timer::event(Event* ev)
{
    ASSERT(false);
    return E_NO_ACCESS;
}

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
#include "CircBuf.h"
#include "Mqtt.h"
class Tcp;

class TcpListener
{
public:
    virtual void onTcpConnect(Tcp* src)=0;
    virtual void onTcpDisconnect(Tcp* src)=0;
    virtual void onTcpMessage(Tcp* src,Bytes* data)=0;
};

class Tcp : public Thread
{
private:
    int _sockfd;
    TcpListener *_listener;
public:
    enum { TCP_CONNECTED, TCP_DISCONNECTED, TCP_PACKET } TcpEvents;
    Tcp( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };

    void addListener(TcpListener *listener)
    {
        _listener=listener;
    }

    Erc connect(char *ip,int portno)
    {
        struct sockaddr_in serv_addr;
        struct hostent *server;
        /* Create a socket point */
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            return E_INVAL;
        }
        server = gethostbyname(ip);
        if (server == NULL)
        {
            return E_NOT_FOUND;
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(portno);

        /* Now connect to the server */
        if (::connect(_sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
        {
            std::cerr << " socket connect() : " << strerror(errno) ;
            return E_CONN_LOSS;
        }
        return E_OK;
    }

    Erc disconnect()
    {
        close(_sockfd);
        return E_OK;
    }


    Erc recv(Bytes* pb)
    {
        int n;

        n=::read(_sockfd,pb->data(),pb->length()) ;
        if (n < 0)
        {
            return E_CONN_LOSS;
        }
        return E_OK;
    }

    int32_t read()
    {
        int n;
        uint8_t b;

        n=::read(_sockfd,&b,1) ;
        if (n <= 0)
        {
            return -1;
        }
        return b;
    }

    Erc send(Bytes* pb)
    {
        int n;
        n=write(_sockfd,pb->data(),pb->length()) ;
        if (n < 0)
        {
            return E_CONN_LOSS;
        }
        return E_OK;
    }

    void run()
    {
//       ::sleep(3);
        while(true)
        {
            while ( connect((char*)"localhost",1883) != E_OK )
                sleep(5000);
            _listener->onTcpConnect(this);

            while(true)
            {
                int32_t b=read();
                if ( b < 0 ) break;
                mqttRead(b);
            }
            _listener->onTcpDisconnect(this);
            disconnect();
            sleep(5000);
        }
    }
    void mqttRead(int32_t b)
    {
        static MqttIn* mqttIn=new MqttIn(256);

        mqttIn->add(b);
        if (  mqttIn->complete() )
        {
            mqttIn->parse();
            _listener->onTcpMessage(this,mqttIn);
            mqttIn=new MqttIn(256);
            mqttIn->reset();
        }

    }
private:
    MqttIn* mqttMsg;
    CircBuf rxdBuffer;
};

class MqttListener
{
    virtual void onMqttMessage(MqttIn* msg)=0;
    virtual void onMqttConnect()=0;
    virtual void onMqttDisconnect()=0;
};


class Mqtt : public TcpListener,public TimerListener,public Publisher<MqttListener>

{
private:

    Tcp* _tcp;
    MqttOut* _mqttOut;
    Timer* _timer;

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

    }
    void onTcpMessage(Tcp* src,Bytes* data)
    {
        MqttIn *packet=(MqttIn*)data;
        if ( packet->type() == MQTT_MSG_CONNACK)
        {
            if ( packet->_returnCode == MQTT_RTC_CONN_ACC)
            {
            }
//                   publish(MQTT_CONNECTED);
            else
            {
                std::cerr << "Mqtt Connect failed , return code : " << packet->_returnCode;
                _tcp->disconnect();
            }
        }
    };
    void onTimerExpired(Timer* timer)
    {
    }
    /*   void eventHandler(Event* event)
       {
           if ( event->is(_tcp,Tcp::TCP_CONNECTED))
           {
               onConnect(_tcp);
           }
           else if ( event->is(_tcp,Tcp::TCP_DISCONNECTED))
           {
               onDisconnect(_tcp);

           }
           else if ( event->is(_tcp,Tcp::TCP_PACKET))
           {
               onMessage(_tcp,(MqttIn*)event->data());
           }
           else
           {
               std::cerr << "unexpected event : "<< event->id()<< std::endl;
           }
       }*/
};

#include "Property.h"
#include "Strpack.h"
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
    }
    void onMqttMessage(MqttIn* msg)
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
    void onMqttConnect()
    {
        _isConnected = true;

        _state = ST_WAIT_ACK;
        _retryCount=0;
    };
    void onMqttDisconnect()
    {
        _isConnected = false;
        _state = ST_SLEEP;
    };
    void eventHandler(Event* event)
    {
        /*      if ( event->is(_timer,Timer::EXPIRED))
              {
                  if ( _state == ST_WAIT_ACK )
                  {
                      if ( _retryCount++< 3 )
                          publishCurrentProperty();
                  }
              }
              else if ( event->is(_mqttConnector,MqttConnector::MQTT_CONNECTED))
              {

              }
              else if ( event->is(_mqttConnector,MqttConnector::MQTT_DISCONNECTED))
              {


              }
            else if ( event->is(_tcp,Tcp::TCP_PACKET))
              {


              }
              else
              {
                  std::cerr << "unexpected event : "<< event->id()<< std::endl;
              }*/
    }

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

    void onMqttMessage(MqttIn* msg)
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
    void onMqttConnect()
    {
        _state = SUBSCRIBING;
        _isConnected = true;
        MqttOut out(256);
        out.Subscribe(0, "ikke/#", ++ _messageId, MQTT_QOS1_FLAG);
        _tcp->send(&out);
    };
    void onMqttDisconnect()
    {
        _isConnected = false;
        _state = DISCONNECTED;
    };

};


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


    sleep(1000000);

}
/*
class EventSource
{
    uint32_t _eventMap;
    virtual uint32_t getEvents()
    {
        uint32_t temp=_eventMap;
        _eventMap=0;
        return temp;
    }
    EventSource()
    {
        _eventMap =0;
    };
    void setEvents(uint32_t event)
    {
        _eventMap |= event;
    }
};

class EventListener
{
    virtual void notify()=0;
};

class tcp : public EventSource
{
    enum { TCP_CONNECTED=1, TCP_DISCONNECTED =2, TCP_MESSAGE  =4 } Events;

};

class mqttConnector : public EventListener {
    private:
    tcp _t;
    public:
        void awake(){
            uint32_t ev;
            while (ev = _t.getEvents() ){
                if ( ev & TCP_CONNECTED )
            }
        }

};

*/







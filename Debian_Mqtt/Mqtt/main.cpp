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



//______________________________________________________________________________________
#include <time.h>


class Timer : public Thread
{
private:
    static Timer* _first;
public:

    Timer( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
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
/*
class MqttThread : public Stream,Thread
{
public:
    MqttThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
    void run()
    {
        while(true)
        {
            MqttIn mqttIn(256);
            MqttOut mqttOut(256);

            Str str(100);
            uint16_t messageId=1000;

            while ( tcp.connect("test.mosquitto.org",1883) != E_OK )
                sleep(5000);
            mqttOut.Connect(0, "clientId", MQTT_CLEAN_SESSION,
                            "ikke/alive", "false", "userName", "password", 1000);
            tcp.send(&mqttOut);
            mqttRead();

            Str topic(100),msg(100);
            topic.append("ikke/topic");
            msg.append("value");
            mqttOut.Publish(MQTT_QOS2_FLAG, &str, &msg, messageId);

            tcp.send(&mqttOut);
            mqttOut.PubRel(messageId);
            tcp.send(&mqttOut);
            mqttOut.Disconnect();
            tcp.send(&mqttOut);
            tcp.disconnect();
        }
    }
    void mqttRead()
    {
        MqttIn mqttIn(256);
        mqttIn.reset();
        while ( ! mqttIn.complete() )
        {
            mqttIn.add(tcp.read());
        }
        mqttIn.parse();
    }

};
*/
#include "CircBuf.h"
#include "Mqtt.h"

class Tcp : public Thread, public Stream
{
private:
    int _sockfd;
public:
    enum { TCP_CONNECTED, TCP_DISCONNECTED, TCP_PACKET } TcpEvents;
    Tcp( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };

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
            while ( connect("localhost",1883) != E_OK )
                sleep(5000);
            publish(TCP_CONNECTED);

            while(true)
            {
                int32_t b=read();
                if ( b < 0 ) break;
                mqttRead(b);
            }
            publish(TCP_DISCONNECTED);
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
            publish(TCP_PACKET,mqttIn);
            mqttIn=new MqttIn(256);
            mqttIn->reset();
        }

    }
private:
    MqttIn* mqttMsg;
    CircBuf rxdBuffer;
};

class MqttConnector : public Stream
{
private:

    Tcp* _tcp;
    MqttOut* _mqttOut;

public:
    enum { MQTT_CONNECTED=100, MQTT_DISCONNECTED } Events;

    MqttConnector( )
    {
    };
    void init(Tcp* tcp)
    {
        _tcp=tcp;
        _tcp->addListener(this);
        _mqttOut = new MqttOut(256);
    }
    void eventHandler(Event* event)
    {
        if ( event->is(_tcp,Tcp::TCP_CONNECTED))
        {
            _mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION,
                              "ikke/alive", "false", "", "", 1000);
            _tcp->send(_mqttOut);
        }
        else if ( event->is(_tcp,Tcp::TCP_DISCONNECTED))
        {
            publish(MQTT_DISCONNECTED);

        }
        else if ( event->is(_tcp,Tcp::TCP_PACKET))
        {
            MqttIn *packet=(MqttIn*)event->data();
            if ( packet->type() == MQTT_MSG_CONNACK)
            {
                if ( packet->_returnCode == MQTT_RTC_CONN_ACC)
                    publish(MQTT_CONNECTED);
                else
                {
                    std::cerr << "Mqtt Connect failed , return code : " << packet->_returnCode;
                    _tcp->disconnect();
                }
            }
        }
        else
        {
            std::cerr << "unexpected event : "<< event->id()<< std::endl;
        }
    }
};


//_____________________________________________________________________________________________________
class MqttPublisher : public Stream
{
private:

    Tcp* _tcp;
    MqttConnector* _mqttConnector;
    bool _isConnected;

public:
    MqttPublisher()
    {
        _isConnected=false;
    };
    void init(Tcp* tcp,MqttConnector* mqttConnector)
    {
        _mqttConnector = mqttConnector;
        _mqttConnector->addListener(this);
        _tcp=tcp;
        _tcp->addListener(this);
    }
    void eventHandler(Event* event)
    {
        if ( event->is(_mqttConnector,MqttConnector::MQTT_CONNECTED))
        {
            _isConnected = true;
            MqttOut out(256);
            Str topic(20);
            topic.append("ikke/alive");
            Str message(100);
            message.append("true");
            out.Publish(0,&topic,&message,123);
            _tcp->send(&out);
        }
        else if ( event->is(_mqttConnector,MqttConnector::MQTT_DISCONNECTED))
        {
            _isConnected = false;

        }
        else if ( event->is(_tcp,Tcp::TCP_PACKET))
        {
            MqttIn *packet=(MqttIn*)event->data();
            if ( packet->type() == MQTT_MSG_PUBACK)
            {

            }
            else if ( packet->type() == MQTT_MSG_PUBREC)
            {

            }
            else if ( packet->type() == MQTT_MSG_PUBCOMP)
            {

            }

        }
        else
        {
            std::cerr << "unexpected event : "<< event->id()<< std::endl;
        }
    }
};
//_____________________________________________________________________________________________________
class MqttSubscriber : public Stream
{
private:

    Tcp* _tcp;
    MqttConnector* _mqttConnector;
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
    void init(Tcp* tcp,MqttConnector* mqttConnector)
    {
        _mqttConnector = mqttConnector;
        _mqttConnector->addListener(this);
        _tcp=tcp;
        _tcp->addListener(this);
    }
    void eventHandler(Event* event)
    {
        if ( event->is(_mqttConnector,MqttConnector::MQTT_CONNECTED))
        {
            _state = SUBSCRIBING;
            _isConnected = true;
            MqttOut out(256);
            out.Subscribe(0, "ikke/#", ++ _messageId, MQTT_QOS1_FLAG);
            _tcp->send(&out);
        }
        else if ( event->is(_mqttConnector,MqttConnector::MQTT_DISCONNECTED))
        {
            _isConnected = false;
            _state = DISCONNECTED;

        }
        else if ( event->is(_tcp,Tcp::TCP_PACKET))
        {
            MqttIn *packet=(MqttIn*)event->data();
            if ( packet->type() == MQTT_MSG_SUBACK)
            {
                _state=SUBSCRIBED;
            }
            else if ( packet->type() == MQTT_MSG_PUBLISH)
            {
                Str topic(100);
                Str message(100);
                topic.write(&packet->_topic);
                message.write(&packet->_message);
                std::cout << topic.data() << ":" << message.data() << " QOS " << (packet->_header & MQTT_QOS_MASK )<< std::endl;
                if (( packet->_header & MQTT_QOS_MASK )== MQTT_QOS1_FLAG )
                {
                    _mqttOut->PubAck(packet->messageId());
                    _tcp->send(_mqttOut);
                    // execute setter
                } else if (( packet->_header & MQTT_QOS_MASK )== MQTT_QOS2_FLAG )
                {
                    _mqttOut->PubRec(packet->messageId());
                    _tcp->send(_mqttOut);
                    _tempTopic->clear();
                    _tempMessage->clear();
                    _tempTopic->write(&packet->_topic);
                    _tempMessage->write(&packet->_message);
                }  else {
                    // execute setter
                }

            }
            else if ( packet->type() == MQTT_MSG_PUBREL)
            {
                // execute setter with _temp
                std::cout << " PUBREL " << _tempTopic->data() << ":" << _tempMessage->data() << std::endl;
                _mqttOut->PubComp(packet->messageId());
                    _tcp->send(_mqttOut);
            }
        }
        else
        {
            std::cerr << "unexpected event : "<< event->id()<< std::endl;
        }
    }
};


int main(int argc, char *argv[])
{
//    MqttThread mqtt("mqtt",1000,1);
    Tcp tcp("tcp",1000,1);
    MqttConnector mqttConnector;
    mqttConnector.init(&tcp);
    MqttPublisher mqttPublisher;
    mqttPublisher.init(&tcp,&mqttConnector);
    MqttSubscriber mqttSubscriber;
    mqttSubscriber.init(&tcp,&mqttConnector);


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






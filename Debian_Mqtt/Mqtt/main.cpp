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

class Tcp
{
public :
    Tcp();
    Erc connect(char *ip,int port);
    Erc disconnect();
    Erc send(Bytes* pb);
    Erc recv(Bytes* pb);
    uint8_t read();
private :
    int _sockfd;

};

#include <time.h>

int nanosleep(const struct timespec *req, struct timespec *rem);


class Timer : public Thread
{
private:
    static Timer* _first;
public:

    Timer( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
public :
    Timer()
    {
    }
    void start()
    {
    }
}

    Tcp::Tcp()
    {
    }

    Erc Tcp::connect(char *ip,int portno)
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
            return E_CONN_LOSS;
        }
        return E_OK;
    }

    Erc Tcp::disconnect()
    {
        close(_sockfd);
        return E_OK;
    }


    Erc Tcp::recv(Bytes* pb)
    {
        int n;

        n=::read(_sockfd,pb->data(),pb->length()) ;
        if (n < 0)
        {
            return E_CONN_LOSS;
        }
        return E_OK;
    }

    uint8_t Tcp::read()
    {
        int n;
        uint8_t b;

        n=::read(_sockfd,&b,1) ;
        if (n < 0)
        {
            return 0;
        }
        return b;
    }

    Erc Tcp::send(Bytes* pb)
    {
        int n;
        n=write(_sockfd,pb->data(),pb->length()) ;
        if (n < 0)
        {
            return E_CONN_LOSS;
        }
        return E_OK;
    }


#include "Queue.h"
#include "Event.h"
#include "Thread.h"
#include "Stream.h"

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
    Erc Thread::sleep(uint32_t time)
    {
        ::sleep(time/1000);
    }
    Queue q(sizeof(Event),10);

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
                q.get(&event);
                event.dst()->eventHandler(&event);
            }
        }
    };

    class MqttThread : public Stream,Thread
    {
    private:

        Tcp tcp;

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

#include "CircBuf.h"

    class TcpThread : public Thread,Stream
    {


    public:
        enum { CONNECTED, DISCONNECTED, RXD } TcpEvents;
        TcpThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
        {
        };
        void run()
        {
            while(true)
            {
                while ( tcp.connect("test.mosquitto.org",1883) != E_OK )
                    sleep(5000);
                publish(CONNECTED);
                int n;
                uint8_t b;

                while(true)
                {
                    rxdBuffer.write(tcp.read());
                    publish(RXD);
                }
                tcp.disconnect();
                sleep(5000);
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
    private:

        Tcp tcp;
        CircBuf rxdBuffer;
    };

    class MqttPublisher
    {
    private:
        MqttPacketeer* _packeteer;
        Timer* _timer;

    public:
        void init(MqttPacketeer packeteer)
        {
            _timer = new Timer();
            _timer->addListener(this);
        }
        void eventHandler(Event* pEvent)
        {
            if ( event.is(MQTT_PUBACK,_packeteer))
            {
            }
            else if ( event.is(MqttReceiver::MQTT_PUBREC))
            {
                publishTo(packeteer,new MqttOut.PubRel(msgId);
                      }
                      else if ( event.is(TIMEOUT))
            {
                {
                }
                else if ( event.is(MQTT_CONNECTED) )
                {

                }
                else if ( event.is(PROPERTY_CHANGE))
                {
                }

            }
        }
    };

    class MqttPacketeer : public Stream
    {
        void eventHandler(Event* pEvent);
        {
            MqttIn mqttIn(256);
            mqttIn.reset();
            while ( ! mqttIn.complete() )
            {
                mqttIn.add(tcp.read());
            }
            mqttIn.parse();
        }
    }

    int main(int argc, char *argv[])
    {
        MainThread mt("messagePump",1000,1);
        MqttThread mqtt("mqtt",1000,1);
        TcpThread tcp("tcp",1000,1);

        Event* event=new Event();
        q.put(event);


    }



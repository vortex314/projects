#include <iostream>

using namespace std;
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>
#include <cstdlib>

#include "Str.h"
#include <time.h>
#include "Timer.h"
#include "MqttOut.h"
#include "MqttIn.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include "Sequence.h"
#include <unistd.h>
#include "Thread.h"
#include "pt.h"
#include "Sequence.h"
#include "Tcp.h"
#include "Usb.h"
#include <time.h>
#include "Logger.h"


Logger logger(256);

struct
{
    const char* host;
    uint16_t port;
    uint32_t baudrate;
    const char* device;
    Logger::Level logLevel;
} context= {"localhost",1883,115200,"/dev/ttyACM0",Logger::INFO};

Usb usb("/dev/ttyACM0");
Tcp tcp("localhost",1883);
//_______________________________________________________________________________________
//
// simulates RTOS generating events into queue : Timer::TICK,Usb::RXD,Usb::CONNECTED,...
//_______________________________________________________________________________________
class Mqtt
{
public:
    static const int RXD;
};


const int Mqtt::RXD=Event::nextEventId("Mqtt::RXD");
int Timer::TICK =Event::nextEventId("Timer::TICK");

class EventLogger : public Sequence
{
public:
    EventLogger()
    {
    }
    int handler ( Event* event )
    {
        if ( event->id() != Timer::TICK )
        {
            logger.debug() << " EVENT : " ;
            Str str(100);
            event->toString(str);
            logger << str;
            logger.flush();
        }
        return 0;
    };

};

void eventPump()
{
    Event event;
    while ( Sequence::get ( event ) == E_OK )
    {
        int i;
        for ( i = 0; i < MAX_SEQ; i++ )
            if ( Sequence::activeSequence[i] )
            {
                if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED )
                {
                    Sequence* seq = Sequence::activeSequence[i];
                    seq->unreg();
                    delete seq;
                };
            }
    }

};



class PollerThread : public Thread , public Sequence
{
public:
    PollerThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority )
    {
        Sys::upTime();
    };
    int handler ( Event* ev )
    {
        return 0;
    };
    void run()
    {
        while(true)
        {
            poller(usb.fd(),tcp.fd());
            eventPump();
        }
    }

    void poller(int usbFd,int tcpFd)
    {
        fd_set rfds;
        fd_set wfds;
        fd_set efds;
        struct timeval tv;
        int retval;

        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        if ( usbFd ) FD_SET(usbFd, &rfds);
        if ( tcpFd ) FD_SET(tcpFd,&rfds);
        if ( usbFd ) FD_SET(usbFd, &efds);
        if ( tcpFd )  FD_SET(tcpFd,&efds);

        /* Wait up to five seconds. */
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        int maxFd = usbFd < tcpFd ? tcpFd : usbFd;
        maxFd+=1;

        retval = select(maxFd, &rfds, NULL, &efds, &tv);

        if (retval < 0 )
        {
            logger.perror("select()");
            sleep(1);
        }
        else if (retval>0)   // one of the fd was set
        {
            if ( FD_ISSET(usbFd,&rfds) )
            {
                publish(Usb::RXD);
            }
            if ( FD_ISSET(tcpFd,&rfds) )
            {
                publish(Tcp::RXD);
            }
            if ( FD_ISSET(usbFd,&efds) )
            {
                publish(Usb::ERROR);
            }
            if ( FD_ISSET(tcpFd,&efds) )
            {
                publish(Tcp::ERROR);
            }
        }
        else
            //TODO publish TIMER_TICK
            Sequence::publish(Timer::TICK);
    }
};

MqttIn mqttIn(120);


class Gateway : public Sequence
{
private:
    struct pt t;
public:

    Gateway (  )
    {
        PT_INIT ( &t );
    }

    int handler ( Event* event )
    {
        PT_BEGIN ( &t );
        while(true)
        {
            PT_YIELD_UNTIL(&t,event->is(Tcp::MESSAGE) || event->is(Usb::MESSAGE));
            if ( event->is(Tcp::MESSAGE))
            {
                MqttIn* msg = tcp.recv();
                assert(msg!=NULL);
                usb.send(*msg);
            }
            else if ( event->is(Usb::MESSAGE))
            {
                MqttIn* msg = usb.recv();
                assert(msg!=NULL);
                MqttIn* mqttIn=(MqttIn*)(msg);
                if ( mqttIn->length() >1 )   // sometimes bad message
                {
                    mqttIn->parse();
                    Str str(100);
                    mqttIn->toString(str);

                    if ( tcp.isConnected() )
                    {
                        if ( mqttIn->type() == MQTT_MSG_CONNECT ) // simulate a reply
                        {
                            MqttOut m(10);
                            m.ConnAck(0);
//                           uint8_t CONNACK[]={0x20,0x02,0x00,0x00};
                            usb.send(m);
                        }
                        else
                        {
                            tcp.send(*msg);
                        }
                    }
                    else
                    {
                        if ( mqttIn->type() == MQTT_MSG_CONNECT )
                        {
                            tcp.connect();
                            tcp.send(*msg);
                        }
                        else
                        {
                            logger.info()<< "dropped packet, not connected.";
                            logger.flush();
                        }
                    }
                }

            }
            PT_YIELD ( &t );
        }

        PT_END ( &t );
    }
};

class UsbConnection : public Sequence
{
private:
    struct pt t;
    MqttOut msg;
public:

    UsbConnection (  ) :msg(256)
    {
        PT_INIT ( &t );
    }

    int handler ( Event* event )
    {
        Str topic(10);
        Str data(20);
        PT_BEGIN ( &t );
        while(true)
        {
            while ( true )
            {
                if ( usb.connect() == E_OK ) break;
                timeout(5000);
                PT_YIELD_UNTIL ( &t, timeout() );
            }
            while ( usb.isConnected() )
            {
                PT_YIELD ( &t );
            }
            tcp.disconnect();
        }
        PT_END ( &t );
    }

};



#include "Tcp.h"

void loadOptions(int argc,char* argv[])
{
    int c;
    while ((c = getopt (argc, argv, "h:p:d:b:")) != -1)
        switch (c)
        {
        case 'h':
            context.host=optarg;
            break;
        case 'p':
            context.port= atoi(optarg);
            break;
        case 'd':
            context.device = optarg;
            break;
        case 'b':
            context.baudrate= atoi(optarg);
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
            return ;
        default:
            abort ();
        }
}

#include <signal.h>

void SignalHandler(int signal_number)
{
    printf("Received signal: %s\n", strsignal(signal_number));
    sleep(1000000);
}

void interceptAllSignals()
{
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGTERM, SignalHandler);
}

extern bool testBytes();

int main(int argc, char *argv[] )
{

    if ( testBytes()!=true) exit(1);
    logger.module("Main");
    logger.level(Logger::INFO)<<"Start "<<argv[0]<<" version : "<<__DATE__ << " " << __TIME__ ;
    logger.flush();

    loadOptions(argc,argv);

    usb.setDevice(context.device);
    usb.setBaudrate(context.baudrate);
    tcp.setHost(context.host);
    tcp.setPort(context.port);

    PollerThread poller("",0,1);
//    poller.start();
    UsbConnection usbConnection;
//    EventLogger eventLogger;
    Gateway gtw;
//   sleep(100000);
    poller.run();
}












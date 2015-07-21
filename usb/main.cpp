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
#include <sys/time.h>
#include <sys/types.h>
//#include "Sequence.h"
#include <unistd.h>
#include "Thread.h"
// #include "Sequence.h"
#include "Tcp.h"
#include "Usb.h"
#include <time.h>
#include "Logger.h"
#include "Handler.h"


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
class System : public Handler
{
public:
    System(): Handler("OS")
    {
    }
    bool dispatch(Msg& msg)
    {
        return true;
    }
} ;
Handler* os=new System();

struct SignalEntry
{
    uint32_t value;
    const char *szValue;
} sigTable[]=
{
    { SIG_INIT ,"SIG_INIT" },
    { SIG_IDLE ,"SIG_DILE" },
    { SIG_ERC ,"SIG_ERC" },
//	{ SIG_TIMEOUT = 1 << 3,
    { SIG_TICK ,"SIG_TICK"},
    { SIG_CONNECTED ,"SIG_CONNECTED"},
    { SIG_DISCONNECTED ,"SIG_DISCONNECTED"},
    { SIG_RXD ,"SIG_RXD"},
    { SIG_TXD ,"SIG_TXD"},
    { SIG_START ,"SIG_START"},
    { SIG_STOP ,"SIG_STOP"},
    { SIG_DO ,"SIG_DO"}
};

const char* signalToString(uint32_t signal)
{
    uint32_t i;
    for(i=0; i<sizeof(sigTable)/sizeof(struct SignalEntry); i++)
        if (sigTable[i].value==signal) return sigTable[i].szValue;
    return "UNKNOWN";
       }

       void logMsg(Msg& msg)
{
    logger.debug() << " Msg : (" << msg.src->getName() << ","<< signalToString(msg.signal) << "," << msg.param <<")";
    logger.flush();
}

//_______________________________________________________________________________________
//
// simulates RTOS generating events into queue : Timer::TICK,Usb::RXD,Usb::CONNECTED,...
//_______________________________________________________________________________________


void poller(int usbFd,int tcpFd,uint64_t sleepTill)
{
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    struct timeval tv;
    int retval;
    uint64_t start;
    Str strEvents(100);
    uint64_t delta=1000;
    if ( usbFd == 0 && tcpFd==0 )
    {
        sleep(1);
        MsgQueue::publish(os,SIG_TICK,0,0);
        strEvents << " SIG_TICK ";
    }
    else
    {

        /* Watch usbFd and tcpFd  to see when it has input. */
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        if ( usbFd ) FD_SET(usbFd, &rfds);
        if ( tcpFd ) FD_SET(tcpFd,&rfds);
        if ( usbFd ) FD_SET(usbFd, &efds);
        if ( tcpFd )  FD_SET(tcpFd,&efds);

        /* Wait up to 1000 msec. */
        uint64_t delta=1000;
        if ( sleepTill > Sys::upTime())
        {
            delta = sleepTill- Sys::upTime();
        }

        tv.tv_sec = delta/1000;
        tv.tv_usec = (delta*1000)%1000000;

        int maxFd = usbFd < tcpFd ? tcpFd : usbFd;
        maxFd+=1;

        start=Sys::upTime();

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
                MsgQueue::publish(os,SIG_RXD,usbFd,0);
                strEvents << " USB_RXD ";
            }
            if ( FD_ISSET(tcpFd,&rfds) )
            {
                MsgQueue::publish(os,SIG_RXD,tcpFd,0);
                strEvents << " TCP_RXD ";
            }
            if ( FD_ISSET(usbFd,&efds) )
            {
                MsgQueue::publish(os,SIG_ERC,usbFd,0);
                strEvents << " USB_ERR ";
            }
            if ( FD_ISSET(tcpFd,&efds) )
            {
                MsgQueue::publish(os,SIG_ERC,tcpFd,0);
                strEvents << " TCP_ERR ";
            }
        }
        else
        {
            //TODO publish TIMER_TICK
            MsgQueue::publish(os,SIG_TICK,0,0);
            strEvents << " SIG_TICK ";

        }
    }
    uint64_t waitTime=Sys::upTime()-start;
    if ( waitTime > 1 )
    {
//        logger.info() << "waited " << waitTime << " / "<< delta << " msec." << strEvents ;
 //       logger.flush();
    }
}

MqttIn mqttIn(new Bytes(256));
MqttOut mqttOut(256);
/*_______________________________________________________________________________

Gateway role :
    - takes usb/tcp input message and send to tcp/usb
    - if connection request from usb , check if tcp is already open and send dummy connect_ok
    - msg.data points to a parsed MqttIn structure
    - if usb messages ( other then connect ) are received when tcp is not connected, they are discarded
    - if connect is received and tcp is not yet connected, make a connection on tcp
________________________________________________________________________________*/

class Gateway : public Handler
{
private:
    MqttIn* _mqttIn;
    Usb* _usb;
    Tcp* _tcp;
public:

    Gateway ( Usb* usb,Tcp* tcp )
    {
        _usb = usb;
        _tcp = tcp;
        restart();
    }

    bool dispatch(Msg& msg)
    {
        PT_BEGIN (  );
        while(true)
        {
            PT_YIELD_UNTIL( msg.is(_tcp,SIG_RXD) || msg.is(_usb,SIG_RXD) );
            if ( msg.is(_tcp,SIG_RXD))
            {
                MqttIn* mqttIn = (MqttIn*)msg.data;
                Str str(256);
                str << "MQTT TCP->USB:";
                mqttIn->toString(str);
                logger.info()<< str;
                logger.flush();
                usb.send(*mqttIn->getBytes());
            }
            else if ( msg.is(_usb,SIG_RXD))
            {
                MqttIn* mqttIn = (MqttIn*)msg.data;
                Str str(256);
                str << "MQTT USB->TCP:";
                mqttIn->toString(str);
                logger.info()<< str;
                logger.flush();

                if ( _tcp->isConnected() )
                {
                    if ( mqttIn->type() == MQTT_MSG_CONNECT ) // simulate a reply
                    {
                        MqttOut m(10);
                        m.ConnAck(0);
                        //                           uint8_t CONNACK[]={0x20,0x02,0x00,0x00};
                        logger.info()<< "CONNACK virtual,already tcp connected";
                        logger.flush();
                        usb.send(m);
                    }
                    else
                    {
                        tcp.send(*mqttIn->getBytes());
                    }
                }
                else
                {
                    if ( mqttIn->type() == MQTT_MSG_CONNECT )
                    {
                        tcp.connect();
                        tcp.send(*mqttIn->getBytes());
                    }
                    else
                    {
                        logger.info()<< "dropped packet, not connected.";
                        logger.flush();
                        usb.disconnect();
                    }
                }
            }
        }
        PT_END (  );
    }
};
/*_______________________________________________________________________________

UsbConnection  role :
    - establish usb connection ( repeat on failure with intermediary sleep )
    - wait for usb disconnection -> disconnect tcp
________________________________________________________________________________*/

class UsbConnection : public Handler
{
private:
    MqttOut msg;
    uint64_t wakeTime;
    Usb* _usb;
public:

    UsbConnection ( Usb* usb ) :msg(256)
    {
        _usb = usb;
        restart();
    }

    bool dispatch ( Msg& msg )
    {
        PT_BEGIN (  );
        while(true)
        {
            while ( usb.connect() != E_OK )
            {
                wakeTime=Sys::upTime()+5000;
                PT_YIELD_UNTIL ( wakeTime < Sys::upTime()) ;
            }

            PT_YIELD_UNTIL (  msg.is(_usb,SIG_DISCONNECTED,usb.fd(),0) );
        }
        PT_END (  );
    }

};

/*_______________________________________________________________________________

loadOptions  role :
    - parse commandline otions
    h : host of mqtt server
    p : port
    d : the serial device "/dev/ttyACM*"
    b : the baudrate set ( only usefull for a usb2serial box or a real serial port )
________________________________________________________________________________*/

#include "Tcp.h"

void loadOptions(int argc,char* argv[])
{
    int c;
    while ((c = getopt (argc, argv, "h:p:d:b:l:")) != -1)
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
        case 'l':
            if ( strcmp(optarg,"DEBUG")==0) context.logLevel = Logger::DEBUG;
            if ( strcmp(optarg,"INFO")==0) context.logLevel = Logger::INFO;
            if ( strcmp(optarg,"WARN")==0) context.logLevel = Logger::WARN;
            if ( strcmp(optarg,"ERROR")==0) context.logLevel = Logger::ERROR;
            if ( strcmp(optarg,"FATAL")==0) context.logLevel = Logger::FATAL;
            logger.setLevel(context.logLevel);
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
#include <execinfo.h>

void SignalHandler(int signal_number)
{
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:%s \n", signal_number,strsignal(signal_number));
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
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

#define TIMER_TICK 1000

int main(int argc, char *argv[] )
{

    if ( testBytes()!=true) exit(1);
    logger.module("Main");
    logger.level(Logger::INFO)<<"Start "<<argv[0]<<" version : "<<__DATE__ << " " << __TIME__ ;
    logger.flush();

    loadOptions(argc,argv);
    interceptAllSignals();

    usb.setDevice(context.device);
    usb.setBaudrate(context.baudrate);
    tcp.setHost(context.host);
    tcp.setPort(context.port);

    UsbConnection usbConnection(&usb);
    Gateway gtw(&usb,&tcp);
    uint64_t sleepTill=Sys::upTime()+TIMER_TICK;
    MsgQueue::publish(os,SIG_INIT);
    Msg msg;
    while(true)
    {
        poller(usb.fd(),tcp.fd(),sleepTill);
        sleepTill = Sys::upTime()+TIMER_TICK; // was 100000
        while (MsgQueue::get(msg))
        {
            logMsg(msg);
            Handler::dispatchToChilds(msg);
        }
    }
}




















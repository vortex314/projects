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


//_______________________________________________________________________________________
//
// simulates RTOS generating events into queue : Timer::TICK,Usb::RXD,Usb::CONNECTED,...
//_______________________________________________________________________________________


void poller(int tcpFd,uint64_t sleepTill)
{
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    struct timeval tv;
    int retval;
    uint64_t start;
    Str strEvents(100);

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    if ( tcpFd ) FD_SET(tcpFd,&rfds);
    if ( tcpFd )  FD_SET(tcpFd,&efds);

    /* Wait up to 1000 msec. */
    uint64_t delta=1000;
    if ( sleepTill > Sys::upTime())
    {
        delta = sleepTill- Sys::upTime();
    }

    tv.tv_sec = delta/1000;
    tv.tv_usec = (delta*1000)%1000000;

    int maxFd = 0 < tcpFd ? tcpFd : 0;
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
        if ( FD_ISSET(tcpFd,&rfds) )
        {
            Msg::publish(SIG_TCP_RXD);
            strEvents << " TCP_RXD ";
        }
        if ( FD_ISSET(tcpFd,&efds) )
        {
            Msg::publish(SIG_TCP_ERROR);
            strEvents << " TCP_ERR ";
        }
    }
    else
    {
        //TODO publish TIMER_TICK
        Msg::publish(SIG_TIMEOUT);
        strEvents << " TIM_OUT ";

    }
    uint64_t waitTime=Sys::upTime()-start;
    if ( waitTime > 1 )
    {
        logger.info() << "waited " << waitTime << " / "<< delta << " msec." << strEvents ;
        logger.flush();
    }
}



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

#include "Mqtt.h"


class UptimeTopic: public Prop {
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags ) { T_UINT64, M_READ, T_1SEC, QOS_0,
							NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Cbor msg(message);
		msg.add(Sys::upTime());
	}
};

UptimeTopic uptime;
/*
class TcpConnection : public Handler {
    TcpConnection()
}
*/
const char *strSignal[]={
        "SIG_INIT",//  0x1,
    "SIG_TIMEOUT",//  0x2,
    "SIG_TIMER_TICK",//  0x4 ,
    "SIG_LINK_CONNECTED",//  0x8 ,
    "SIG_LINK_DISCONNECTED",//  0x10 ,
    "SIG_LINK_RXD",//  0x20 ,
    "SIG_MQTT_CONNECTED",//  0x40,
    "SIG_MQTT_DISCONNECTED",//  0x80,
    "SIG_MQTT_MESSAGE",//  0x100 ,
    "SIG_MQTT_DO_PUBLISH",//  0x200,
    "SIG_MQTT_PUBLISH_FAILED",//  0x400,
    "SIG_MQTT_PUBLISH_OK",// "=0x800,
    "SIG_PROP_CHANGED", // "=0x1000,
    "SIG_IDLE",//  0x2000,
    "SIG_TCP_RXD", // 0x4000,
    "SIG_TCP_ERROR", // 0x8000,
    "SIG_USB_RXD", // 0x10000,
    "SIG_USB_ERROR", // 0x20000,
    "SIG_TCP_MESSAGE", // 0x40000,
    "SIG_USB_MESSAGE", // 0x80000,
    "SIG_USB_CONNECTED", // 0x100000,
    "SIG_TCP_CONNECTED", // 0x200000,
    "SIG_TCP_DISCONNECTED", // 0x400000,
    "SIG_USB_DISCONNECTED" // 0x800000
};
const char *sigToString(Signal signal){
    uint32_t s=signal;
for(int i=0;i<32;i++) {
    if ( s & 1 ) return strSignal[i];
    s = s >>1 ;
    }
    return "unknown signal";
}

PropMgr propMgr;
Tcp tcp("localhost",1883);

int main(int argc, char *argv[] )
{

    logger.module("Main");
    logger.level(Logger::INFO)<<"Start "<<argv[0]<<" version : "<<__DATE__ << " " << __TIME__  << "\n";

    loadOptions(argc,argv);
    interceptAllSignals();

    tcp.setHost(context.host);
    tcp.setPort(context.port);
 //   tcp.connect();
    Mqtt mqtt(tcp);
    char hostname[30];
    gethostname(hostname,sizeof(hostname));

    mqtt.setPrefix(hostname);

    propMgr.mqtt(mqtt);

    Msg initMsg(6);
    initMsg.sig(SIG_INIT);

    Msg msg;
    msg.create(10).sig(SIG_INIT).send();

    while(true)
    {
        poller(tcp.fd(),Handler::nextTimeout());
        while (true )
        {
            msg.open();
            if ( msg.sig()==SIG_IDLE) break;
            logger.info()<< "signal : " << sigToString(msg.sig()) <<"\n";
            Handler::dispatchAll(msg);
            msg.recv();
        }

    }
}



















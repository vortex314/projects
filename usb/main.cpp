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

    /* Watch stdin (fd 0) to see when it has input. */
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
            Msg::publish(SIG_USB_RXD);
        }
        if ( FD_ISSET(tcpFd,&rfds) )
        {
            Msg::publish(SIG_TCP_RXD);
        }
        if ( FD_ISSET(usbFd,&efds) )
        {
            Msg::publish(SIG_USB_ERROR);
        }
        if ( FD_ISSET(tcpFd,&efds) )
        {
            Msg::publish(SIG_TCP_ERROR);
        }
    }
    else
        //TODO publish TIMER_TICK
        Msg::publish(SIG_TIMEOUT);
    uint64_t waitTime=Sys::upTime()-start;
    if ( waitTime > 1 )
    {
        logger.info() << "waited " << waitTime << " / "<< delta << " msec.";
        logger.flush();
    }
}

MqttIn mqttIn(new Bytes(256));


class Gateway : public Handler
{
private:
    struct pt t;
    MqttIn* _mqttIn;
public:

    Gateway (  )
    {
        PT_INIT ( &t );
    }

    void dispatch(Msg& msg)
    {
        handler(msg);
    }

    int handler ( Msg& msg )
    {
        PT_BEGIN ( &t );
        while(true)
        {
            listen(SIG_TCP_MESSAGE | SIG_USB_MESSAGE);
            PT_YIELD(&t);
            if ( msg.sig()==SIG_TCP_MESSAGE )
            {
                Bytes bytes(0);
                msg.get(bytes);
                MqttIn mqttIn(&bytes);
                bytes.offset(0);
                if (mqttIn.parse())
                {
                    Str str(256);
                    str << "MQTT TCP->USB:";
                    mqttIn.toString(str);
                    logger.info()<< str;
                    logger.flush();
                    usb.send(*mqttIn.getBytes());
                }
            }
            else if ( msg.sig()==SIG_USB_MESSAGE )
            {
                Bytes bytes(0);
                msg.get(bytes);
                MqttIn mqttIn(&bytes);
                bytes.offset(0);
                if (mqttIn.parse())
                {
                    Str str(256);
                    str << "MQTT USB->TCP:";
                    mqttIn.toString(str);
                    logger.info()<< str;
                    logger.flush();

                    if ( tcp.isConnected() )
                    {
                        if ( mqttIn.type() == MQTT_MSG_CONNECT ) // simulate a reply
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
                            tcp.send(*mqttIn.getBytes());
                        }
                    }
                    else
                    {
                        if ( mqttIn.type() == MQTT_MSG_CONNECT )
                        {
                            tcp.connect();
                            tcp.send(*mqttIn.getBytes());
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

class UsbConnection : public Handler
{
private:
    struct pt t;
    MqttOut msg;
public:

    UsbConnection (  ) :msg(256)
    {
        PT_INIT ( &t );
    }

    void dispatch ( Msg& msg )
    {

        ptRun(msg);

    }
    int ptRun(Msg& msg)
    {
        PT_BEGIN ( &t );
        while(true)
        {
            usb.connect();
            listen(SIG_USB_DISCONNECTED );
            PT_YIELD ( &t );
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

    //    poller.start();
    UsbConnection usbConnection;
    //    EventLogger eventLogger;
    Gateway gtw;
    uint64_t sleepTill=Sys::upTime()+1000;
    Msg initMsg(6);
    initMsg.sig(SIG_INIT);
    Msg timeoutMsg(6);
    timeoutMsg.sig(SIG_TIMEOUT);
    Msg msg;
    msg.create(10).sig(SIG_INIT).send();

    while(true)
    {
        poller(usb.fd(),tcp.fd(),sleepTill);
        sleepTill = Sys::upTime()+10000;
        while (true )
        {
            msg.open();
            if ( msg.sig() == SIG_IDLE ) break;
            for(Handler* hdlr=Handler::first(); hdlr!=0; hdlr=hdlr->next())
            {
                if ( hdlr->accept(msg.sig()))
                {
                    if ( msg.sig() == SIG_TIMEOUT )
                    {
                        if ( hdlr->timeout() )
                            hdlr->dispatch(timeoutMsg);
                    }
                    else
                        hdlr->dispatch(msg);
                }
                if ( hdlr->accept(SIG_TIMEOUT))
                    if ( hdlr->getTimeout() < sleepTill )
                        sleepTill=hdlr->getTimeout();

            }
            msg.recv();
        }

    }
}



















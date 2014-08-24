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
#include "Log.h"
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


Usb usb("/dev/ttyACM0");
Tcp tcp("test.mosquitto.org",1883);
//_______________________________________________________________________________________
//
// simulates RTOS generating events into queue : Timer::TICK,Usb::RXD,Usb::CONNECTED,...
//_______________________________________________________________________________________
class Mqtt {
    public:
        static const int RXD;
    };


const int Mqtt::RXD=Event::nextEventId("Mqtt::RXD");

class EventLogger : public Sequence {
    public:
        EventLogger() {
            }
        int handler ( Event* event ) {
            if ( event->id() != Timer::TICK ) {
                Log::log() << " EVENT : " ;
                event->toString(Log::log());
                Log::log().flush();
                }
            return 0;
            };

    };

void eventPump() {
    Event event;
    while ( Sequence::get ( event ) == E_OK ) {
        int i;
        for ( i = 0; i < MAX_SEQ; i++ )
            if ( Sequence::activeSequence[i] ) {
                if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED ) {
                    Sequence* seq = Sequence::activeSequence[i];
                    seq->unreg();
                    delete seq;
                    };
                }
        }

    };



class PollerThread : public Thread , public Sequence {
    public:
        PollerThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority ) {
            Sys::upTime();
            };
        int handler ( Event* ev ) {
            return 0;
            };
        void run() {
            while(true) {
                poller(usb.fd(),tcp.fd());
                eventPump();
                }
            }

        void poller(int usbFd,int tcpFd) {
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

            if (retval < 0 ) {
                perror("select()");
                sleep(1);
                }
            else if (retval>0) { // one of the fd was set
                if ( FD_ISSET(usbFd,&rfds) ) {
                    publish(Usb::RXD);
                    }
                if ( FD_ISSET(tcpFd,&rfds) ) {
                    publish(Tcp::RXD);
                    }
                if ( FD_ISSET(usbFd,&efds) ) {
                    publish(Usb::ERROR);
                    }
                if ( FD_ISSET(tcpFd,&efds) ) {
                    publish(Tcp::ERROR);
                    }
                }
            else
                //TODO publish TIMER_TICK
                Sequence::publish(Timer::TICK);
            }
    };

MqttIn mqttIn(120);


class Gateway : public Sequence {
    private:
        struct pt t;
    public:

        Gateway (  )   {
            PT_INIT ( &t );
            }

        int handler ( Event* event ) {
            PT_BEGIN ( &t );
            while(true) {
                PT_YIELD_UNTIL(&t,event->is(Tcp::MESSAGE) || event->is(Usb::MESSAGE));
                if ( event->is(Tcp::MESSAGE))  {
                    MqttIn* msg = tcp.recv();
                    assert(msg!=NULL);
                    usb.send(*msg);
                    }
                else if ( event->is(Usb::MESSAGE))  {
                    MqttIn* msg = usb.recv();
                    assert(msg!=NULL);
                    MqttIn* mqttIn=(MqttIn*)(msg);
                    if ( mqttIn->length() >1 ) { // sometimes bad message
                        mqttIn->parse();
                        if ( mqttIn->type() == MQTT_MSG_CONNECT ) {
                            if ( !tcp.isConnected()) { // ignore connect mqqt messages when already connected
                                tcp.connect();
                                tcp.send(*msg);
                                }
                            }
                        else
                            tcp.send(*msg);
                        }
                    }
                PT_YIELD ( &t );
                }

            PT_END ( &t );
            }
    };

class UsbConnection : public Sequence {
    private:
        struct pt t;
        MqttOut msg;
    public:

        UsbConnection (  ) :msg(256) {
            PT_INIT ( &t );
            }

        int handler ( Event* event ) {
            Str topic(10);
            Str data(20);
            PT_BEGIN ( &t );
            while(true) {
                while ( true ) {
                    if ( usb.connect() == E_OK ) break;
                    timeout(5000);
                    PT_YIELD_UNTIL ( &t, timeout() );
                    }
                while ( usb.isConnected() ) {
                    PT_YIELD ( &t );
                    }
                tcp.disconnect();
                }
            PT_END ( &t );
            }

    };



#include "Tcp.h"

int main(int argc, char *argv[] ) {
    if ( argc>1 ) usb.setDevice(argv[1]);

    PollerThread poller("",0,1);
    poller.start();
    UsbConnection usbConnection;
//    EventLogger eventLogger;
    Gateway gtw;
    sleep(100000);
    }










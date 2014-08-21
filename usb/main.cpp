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

class PollerThread : public Thread , public Sequence {
    public:
        PollerThread ( const char *name, unsigned short stackDepth, char priority ) : Thread ( name, stackDepth, priority ) {
            Sys::upTime();
            };
        int handler ( Event* ev ) {
            return 0;
            };
        void run() {
            while(true) poller(usb.fd(),0);
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
            FD_SET(usbFd, &rfds);
            FD_SET(tcpFd,&rfds);
            FD_SET(usbFd, &efds);
            FD_SET(tcpFd,&efds);

            /* Wait up to five seconds. */
            tv.tv_sec = 0;
            tv.tv_usec = 1000000;

            retval = select(usbFd+2, &rfds, NULL, &efds, &tv);

            if (retval < 0 ) {
                perror("select()");
                sleep(1);
                }
            else if (retval>0) { // one of the fd was set
                if ( FD_ISSET(usbFd,&rfds) ) {
                    publish(Usb::RXD);
                    }
                if ( FD_ISSET(tcpFd,&rfds) ) {
                    }
                if ( FD_ISSET(usbFd,&efds) ) {
                    publish(Usb::ERROR);
                    }
                if ( FD_ISSET(tcpFd,&efds) ) {
                    }
                if ( FD_ISSET(usbFd,&wfds) ) {
                    publish(Usb::TXD);
                    }
                if ( FD_ISSET(tcpFd,&wfds) ) {
                    }
                }
            else
                //TODO publish TIMER_TICK
                Sequence::publish(Timer::TICK);
            }
    };

MqttIn mqttIn(120);

class UsbSeq : public Sequence {
    private:
        struct pt t;
        MqttOut msg;
    public:

        UsbSeq (  ) :msg(256) {
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
                    if ( event->is(Usb::RXD)) {
                        int32_t v;
                        while(true){
                            v=usb.read();
                            if ( v < 0 ) break;
                            if ( mqttIn.Feed(v) ){
                                mqttIn.Decode();
                                if ( mqttIn.isGoodCrc() ){
                                    mqttIn.RemoveCrc();
                                    mqttIn.parse();
                                }
                                }
                        }
                    } else if ( event->is(Tcp::RXD) ) {

                    }
                    PT_YIELD ( &t );
                    }
                    tcp.disconnect();
                }
            PT_END ( &t );
            }

    };


void loop() {
    Event event;
    Queue::getDefaultQueue()->get ( &event ); // dispatch eventually IDLE message
    if ( event.id() != Timer::TICK ) {
        Log::log() << " EVENT : " ;
        event.toString(Log::log());
        Log::log().flush();
        }

    int i;
    for ( i = 0; i < MAX_SEQ; i++ )
        if ( Sequence::activeSequence[i] ) {
            if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED ) {
                Sequence* seq = Sequence::activeSequence[i];
                seq->unreg();
                delete seq;
                };
            }

    };

#include "Tcp.h"

int main(int argc, char *argv[] ) {
    MqttOut msg(100);

    if ( argc >1 ) usb=Usb(argv[1]);
    else  usb=Usb("/dev/ttyACM0");
    PollerThread poller("",0,1);
    poller.start();
    UsbSeq usbSeq;

    while(true) {
        loop();
        }
    }








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


Log log;



#include "MqttOut.h"
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


EventId MQTT_CONNECTED = Event::nextEventId ( ( char* const ) "MQTT_CONNECTED" );
EventId MQTT_DISCONNECTED = Event::nextEventId ( ( char* const ) "MQTT_DISCONNECTED" );
EventId PROP_CHANGED = Event::nextEventId ( ( char* const ) "PROP_CHANGED" );

#include "Usb.h"
Usb usb;


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
                    Bytes bytes(256);
                    usb.read(bytes);
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
                    if ( usb.open() == E_OK ) break;
                    timeout(5000);
                    PT_YIELD_UNTIL ( &t, timeout() );
                    }
                while ( usb.isConnected() ) {
                    int i;
                    msg.clear();
                    for(i=0;i<26;i++) {
                        msg.write('a'+i);
                    }
                    if ( usb.write (msg) ) break;
                    timeout(1000);
                    PT_YIELD_UNTIL ( &t, timeout() );
                    }
                }
            PT_END ( &t );
            }
    };


void loop() {
    Event event;
    Queue::getDefaultQueue()->get ( &event ); // dispatch eventually IDLE message
    if ( event.id() != Timer::TICK ) {
        log << " EVENT : " ;
        event.toString(log);
        log.flush();
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








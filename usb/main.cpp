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
static uint64_t _upTime=0;

uint64_t upTime() { // time in msec since boot, only increasing
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    _upTime= deadline.tv_sec*1000 + deadline.tv_nsec/1000000;
    return _upTime;
    }

class Log : public Str {
    public:
        typedef uint64_t EOL;
        EOL eol;
        Log() : Str(100) {
            }
        void flush() {
            cout << upTime()/1000<<"."<< upTime() %1000 << " | " ;
            offset(0);

            while(hasData())
                std::cout << read();
            cout << std::endl;
            clear();
            }
    };

Log log;

const char *HEX="0123456789ABCDEF";
void toHex(Str& out,Bytes& bytes) {
    uint32_t  i;

    for(i=0; i<bytes.length(); i++) {
        uint8_t b;
        b=bytes.read();
        out << (char)HEX[b>>4]<< (char)HEX[b&0x0F] << " ";
        }
    bytes.offset(0);
    for(i=0; i<bytes.length(); i++) {
        uint8_t b;
        b=bytes.read();
        if ( isprint((char)b)) out << (char)b;
        else out << '.';
        }
    }

int open_port(void) {
    int fd;

    fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd == -1) {
        fprintf(stderr, "open_port: Unable to open /dev/ttyACM0 - %s\n",
                strerror(errno));
        }

    return (fd);
    }
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

EventId TIMER_TICK = Event::nextEventId ( ( char* const ) "TIMER_TICK" );
EventId MQTT_CONNECTED = Event::nextEventId ( ( char* const ) "MQTT_CONNECTED" );
EventId MQTT_DISCONNECTED = Event::nextEventId ( ( char* const ) "MQTT_DISCONNECTED" );
EventId PROP_CHANGED = Event::nextEventId ( ( char* const ) "PROP_CHANGED" );

class Timer {
    public :
        const static int TICK;
    };
const int Timer::TICK=Event::nextEventId ( ( char* const ) "Timer::TICK" );

class Usb {
    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,TXD;
        Usb() {
            _fd=0;
            }
        Usb(const char* device) {
            _device =  device;
            _isConnected=false;
            _fd=0;
            }
        int open() {
            struct termios options;
            _fd = ::open(_device, O_RDWR | O_NOCTTY | O_NDELAY);

            if (_fd == -1) {
                log <<  "USB open: Unable to open " << _device << " : "<< strerror(errno);
                log.flush();
                return errno;
                }
            fcntl(_fd, F_SETFL, FNDELAY);

            tcgetattr(_fd, &options);
            cfsetispeed(&options, B38400);
            cfsetospeed(&options, B38400);


            options.c_cflag |= (CLOCAL | CREAD);
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |=  CS8;
            options.c_cflag &= ~CRTSCTS;               /* Disable hardware flow control */


            options.c_lflag &= ~(ICANON | ECHO | ISIG);


            tcsetattr(_fd, TCSANOW, &options);
            log << "USB open " << _device << " succeeded.";
            log.flush();
            _isConnected=true;
            return E_OK;
            }
        int close() {
            _isConnected=false;
            if ( ::close(_fd) < 0 )   return errno;
            return E_OK;
            }
        int write(Bytes& bytes) {
            bytes.offset(0);
            Str line(100);
            toHex(line,bytes);
            log <<"USB write: " << (char*)line.data();
            log.flush();
            int count = ::write(_fd,bytes.data(),bytes.length());
            if ( count != bytes.length()) {
                close();
                return errno;
                }
            return E_OK;
            }
        int read(Bytes& bytes) {
            int count;
            int total=0;
            uint8_t b;
            while(::read(_fd,&b,1)>0){
                bytes.write(b);
                total++;
            }
            bytes.offset(0);
            Str line(100);
            toHex(line,bytes);
            log <<"USB read: " << (char*)line.data();
            log.flush();
            return total;
        }
        int fd() {
            return _fd;
            }
        bool isConnected() {
            return _isConnected;
            }
    private :
        int _fd;
        const char* _device;
        bool _isConnected;
    };
const int Usb::RXD=Event::nextEventId("USB::RXD");
const int Usb::ERROR=Event::nextEventId("USB::ERROR");
const int Usb::TXD=Event::nextEventId("USB::TXD");

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

            retval = select(usbFd+2, &rfds, NULL, NULL, &tv);

            if (retval < 0 ) {
                perror("select()");
                sleep(1);
                }
            else if (retval>0) { // one of the fd was set
                if ( FD_ISSET(usbFd,&rfds) ) {
                    Bytes bytes(100);
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

        UsbSeq (  ) :msg(100) {
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
                    for(i=0;i<16;i++) {
                        msg.write(i);
                    }
                    /*topic << "topic";
                    data << "data";
                    msg.Publish(0,topic,data,123);
                    msg.Encode();
                    msg.Frame();*/
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








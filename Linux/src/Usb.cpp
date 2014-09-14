#include "Usb.h"
const int Usb::RXD=Event::nextEventId("USB::RXD");
const int Usb::ERROR=Event::nextEventId("USB::ERROR");
const int Usb::MESSAGE=Event::nextEventId("USB::MESSAGE");
const int Usb::FREE=Event::nextEventId("USB::FREE");
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "Log.h"

Usb::Usb(const char* device) : msg(100),inBuffer(256) {
    _device =  device;
    isConnected(false);
    _fd=0;
    PT_INIT(&t);
    _isComplete = false;
    }

void Usb::setDevice(const char* device){
    _device =  device;
}

Erc Usb::connect() {
    struct termios options;
    Log::log() << " USB Connecting to " << _device  << " ...";
    Log::log().flush();

    _fd = ::open(_device, O_RDWR | O_NOCTTY | O_NDELAY);

    if (_fd == -1) {
        Log::log() <<  "USB connect: Unable to open " << _device << " : "<< strerror(errno);
        Log::log().flush();
        return E_AGAIN;
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
    Log::log() << "USB open " << _device << " succeeded.";
    Log::log().flush();
    isConnected(true);
    return E_OK;
    }
Erc Usb::disconnect() {
    isConnected(false);
    _isComplete=false;
    if ( ::close(_fd) < 0 )   return errno;
    return E_OK;
    }

Erc Usb::send(Bytes& bytes) {
 //   Log::log().message("USB send : ",bytes);
    bytes.AddCrc();
    bytes.Encode();
    bytes.Frame();
//    Log::log().message("USB send full : ",bytes);
    int count = ::write(_fd,bytes.data(),bytes.length());
    if ( count != bytes.length()) {
        disconnect();
        return errno;
        }
    return E_OK;
    }

uint8_t Usb::read() {
    uint8_t b;
    if (::read(_fd,&b,1)<0) {
        perror(" Usb read failed");
        return 0;
        }
    return b;
    }

    #include <sys/ioctl.h>
uint32_t Usb::hasData(){
    int count;
    int rc = ioctl(_fd, FIONREAD, (char *) &count);
    if (rc < 0) {
        perror("ioctl failed");
        isConnected(false);
        return 0;
        }
        return count;
}

int Usb::handler ( Event* event ) {
    uint8_t b;
    uint32_t i;
    uint32_t count;

    if ( event->is(Usb::ERROR )) {
        disconnect();
        connect();
        return 0;
        }
    PT_BEGIN ( &t );
    while(true) {
        while( isConnected()) {
            PT_YIELD_UNTIL(&t,event->is(RXD) || event->is(FREE) || ( inBuffer.hasData() && (_isComplete==false)) );
            if ( event->is(RXD) &&  hasData()) {
                count =hasData();
                for(i=0;i<count;i++){
                    b=read();
                    inBuffer.write(b);
                    }
                }
            else if ( event->is(FREE)) { // re-use buffer after message handled
                _isComplete=false;
                msg.clear();
                };
            if ( inBuffer.hasData() && (_isComplete==false) ) {
                while( inBuffer.hasData() ) {
                    if ( msg.Feed(inBuffer.read())) {
 //                       Log::log().message("USB recv : " ,msg);
                        msg.Decode();
                        if ( msg.isGoodCrc() ) {
                            msg.RemoveCrc();
                            Log::log().message("USB -> TCP : " ,msg);
                            publish(MESSAGE);
                            publish(FREE); // re-use buffer after message handled
                            _isComplete=true;
                            break;
                            }
                        else {
                            msg.clear(); // throw away bad data
                            }
                        }
                    }
                }
            PT_YIELD ( &t );
            }
        PT_YIELD_UNTIL ( &t,isConnected() );
        }

    PT_END ( &t );
    }

MqttIn* Usb::recv() {
    if ( _isComplete ) {
        return &msg;
        }
    return 0;
    }

int Usb::fd() {
    return _fd;
    }





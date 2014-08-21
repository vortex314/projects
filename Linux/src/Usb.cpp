#include "Usb.h"
const int Usb::RXD=Event::nextEventId("USB::RXD");
const int Usb::ERROR=Event::nextEventId("USB::ERROR");
const int Usb::TXD=Event::nextEventId("USB::TXD");
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "Log.h"

Usb::Usb() {
    _fd=0;
    }
Usb::Usb(const char* device) {
    _device =  device;
    isConnected(false);
    _fd=0;
    }
Erc Usb::connect() {
    struct termios options;
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
    if ( ::close(_fd) < 0 )   return errno;
    return E_OK;
    }
Erc Usb::send(Bytes& bytes) {
    bytes.offset(0);
    Log::log() <<"USB write: " ;
    Log::log().dump(bytes);
    Log::log().flush();
    bytes.AddCrc();
    bytes.Encode();
    bytes.Frame();
    int count = ::write(_fd,bytes.data(),bytes.length());
    if ( count != bytes.length()) {
        disconnect();
        return errno;
        }
    return E_OK;
    }
int32_t Usb::read() {
    uint8_t b;
    if (::read(_fd,&b,1)>0) {
        return b;
        }
    return -1;
    }
int Usb::fd() {
    return _fd;
    }



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
extern Log log;
        Usb::Usb() {
            _fd=0;
            }
        Usb::Usb(const char* device) {
            _device =  device;
            _isConnected=false;
            _fd=0;
            }
        int Usb::open() {
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
        int Usb::close() {
            _isConnected=false;
            if ( ::close(_fd) < 0 )   return errno;
            return E_OK;
            }
        int Usb::write(Bytes& bytes) {
            bytes.offset(0);
            log <<"USB write: " ;log.dump(bytes);
            log.flush();
            int count = ::write(_fd,bytes.data(),bytes.length());
            if ( count != bytes.length()) {
                close();
                return errno;
                }
            return E_OK;
            }
        int Usb::read(Bytes& bytes) {
            int total=0;
            uint8_t b;
            while(::read(_fd,&b,1)>0){
                bytes.write(b);
                total++;
            }
            bytes.offset(0);
            log <<"USB read: " ;log.dump(bytes);
            log.flush();
            return total;
        }
        int Usb::fd() {
            return _fd;
            }
        bool Usb::isConnected() {
            return _isConnected;
            }


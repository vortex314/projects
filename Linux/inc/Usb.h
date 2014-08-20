#ifndef USB_H
#define USB_H

#include <Sequence.h>


class Usb {
    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,TXD;
        Usb() ;
        Usb(const char* device) ;
        int open() ;
        int close() ;
        int write(Bytes& bytes) ;
        int read(Bytes& bytes) ;
        int fd();
        bool isConnected() ;
    private :
        int _fd;
        const char* _device;
        bool _isConnected;
    };
;


#endif // USB_H

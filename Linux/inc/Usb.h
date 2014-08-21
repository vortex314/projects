#ifndef USB_H
#define USB_H

#include <Sequence.h>
#include "Link.h"


class Usb :public Link {
    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,TXD;
        Usb() ;
        Usb(const char* device) ;
        Erc connect() ;
        Erc disconnect() ;
        Erc send(Bytes& bytes) ;
        int32_t read() ;
        int fd();
    private :
        int _fd;
        const char* _device;
    };
;


#endif // USB_H

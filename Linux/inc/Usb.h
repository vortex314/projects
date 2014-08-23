#ifndef USB_H
#define USB_H

#include <Sequence.h>
#include "Link.h"
#include "CircBuf.h"
#include "pt.h"


class Usb :public Link,public Sequence  {

    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,MESSAGE,FREE;

        Usb(const char* device) ;
        Erc connect() ;
        Erc disconnect() ;
        Erc send(Bytes& bytes) ;
        Bytes* recv() ;

        int handler(Event* event);


        int fd();
    private :
    int32_t read() ;
        struct pt t;
        int _fd;
        const char* _device;
        bool _isComplete;
        Bytes msg;
        CircBuf inBuffer;
    };
;


#endif // USB_H

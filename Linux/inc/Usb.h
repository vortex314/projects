#ifndef USB_H
#define USB_H

#include <Sequence.h>
#include "Link.h"
#include "CircBuf.h"
#include "pt.h"
#include "MqttIn.h"
#include "Msg.h"
#include "Handler.h"


class Usb :public Link,public Handler  {

    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,MESSAGE,FREE;

        Usb(const char* device) ;
        Erc connect() ;
        Erc disconnect() ;
        Erc send(Bytes& bytes) ;
        Bytes* recv() ;

        void dispatch(Msg& msg);
        int ptRun(Msg& msg);

        void setDevice(const char* device) ;
        void setBaudrate(uint32_t baud);
        void logStats();
        int fd();
    private :
    uint8_t read() ;
    uint32_t hasData();
        struct pt t;
        int _fd;
        const char* _device;
        uint32_t _baudrate;
        bool _isComplete;
        Bytes* _inBytes;
        CircBuf inBuffer;
    };
;


#endif // USB_H

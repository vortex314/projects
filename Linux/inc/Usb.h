#ifndef USB_H
#define USB_H

#include <Sequence.h>
#include "Link.h"
#include "CircBuf.h"
#include "pt.h"
#include "MqttIn.h"


class Usb :public Link,public Sequence  {

    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,MESSAGE,FREE;

        Usb(const char* device) ;
        Erc connect() ;
        Erc disconnect() ;
        Erc send(Bytes& bytes) ;
        MqttIn* recv() ;

        int handler(Event* event);

        void setDevice(const char* device) ;
        void setBaudrate(uint32_t baud);
        int fd();
    private :
    uint8_t read() ;
    uint32_t hasData();
        struct pt t;
        int _fd;
        const char* _device;
        uint32_t _baudrate;
        bool _isComplete;
        MqttIn msg;
        CircBuf inBuffer;
    };
;


#endif // USB_H

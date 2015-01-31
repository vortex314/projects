#ifndef USB_H
#define USB_H

// #include <Sequence.h>
#include "Link.h"
#include "CircBuf.h"
#include "MqttIn.h"
#include "Msg.h"
#include "Handler.h"


class Usb :public Link  {

    public:
        const static int CONNECTED,DISCONNECTED,RXD,ERROR,MESSAGE,FREE;

        Usb(const char* device) ;
        Erc connect() ;
        Erc disconnect() ;
        Erc send(Bytes& bytes) ;
        Bytes* recv() ;

        bool dispatch(Msg& msg);
        void free(void* ptr);

        void setDevice(const char* device) ;
        void setBaudrate(uint32_t baud);
        void logStats();
        int fd();
    private :
    uint8_t read() ;
    uint32_t hasData();
        int _fd;
        const char* _device;
        uint32_t _baudrate;
        bool _isComplete;
        Bytes _inBytes;
        Bytes _outBuffer;
        CircBuf inBuffer;
    };
;


#endif // USB_H

#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "Thread.h"
#include "EventSource.h"
#include "Sequence.h"
#include "Link.h"
#include "pt.h"


class Tcp : public Link,public Sequence
{
private:
    int _sockfd;
    bool _connected;
    const char* _host;
    uint16_t _port;
    Bytes msg;
    bool _isComplete;
    struct pt t;
public:

    static int CONNECTED,DISCONNECTED,RXD,MESSAGE,FREE,ERROR;
    Tcp( const char *host,uint16_t port);
    ~Tcp();

    Erc connect();
    Erc disconnect();
    bool isConnected();

    Bytes* recv();
    int32_t read();
    Bytes* getMessage(int msgIdx);

    Erc send(Bytes& pb);
    void run();
    void mqttRead(int32_t b);
    int handler(Event*event);
    int fd();
};

#endif // TCP_H

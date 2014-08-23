#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "MqttIn.h"
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
    MqttIn msg;
    struct pt t;
public:

    static int CONNECTED,DISCONNECTED,RXD,MESSAGE,FREE,ERROR;
    Tcp( const char *host,uint16_t port);
    ~Tcp();

    Erc connect();
    Erc disconnect();
    bool isConnected();

    MqttIn* recv();
    int32_t read();
    Erc send(Bytes& pb);


    void mqttRead(int32_t b);
    int handler(Event*event);
    int fd();
};

#endif // TCP_H

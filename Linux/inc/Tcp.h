#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "MqttIn.h"
#include "Thread.h"
#include "EventSource.h"
#include "Handler.h"
#include "Link.h"


class Tcp : public Link
{
private:
    int _sockfd;
    bool _connected;
    const char* _host;
    uint16_t _port;
    MqttIn* _mqttIn;
public:

    static int CONNECTED,DISCONNECTED,RXD,MESSAGE,FREE,ERROR;
    Tcp( const char *host,uint16_t port);
    void setHost(const char* host);
    void setPort(uint16_t port);
    ~Tcp();

    Erc connect();
    Erc disconnect();
    bool isConnected();

    MqttIn* recv();
    uint8_t read();
    uint32_t hasData();
    Erc send(Bytes& pb);


    void mqttRead(int32_t b);
    bool dispatch(Msg& msg);
    int ptRun(Msg& msg);
    int fd();
};

#endif // TCP_H

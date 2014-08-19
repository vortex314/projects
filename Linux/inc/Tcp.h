#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "Thread.h"
#include "EventSource.h"
#include "Sequence.h"


class Tcp : public Sequence
{
private:
    int _sockfd;
    bool _connected;
    const char* _host;
    uint16_t _port;
public:

    static int CONNECTED,DISCONNECTED,RXD,TXD;
    Tcp( const char *host,uint16_t port);
    ~Tcp();

    Erc connect();
    Erc disconnect();
    bool isConnected();

    Erc recv(Bytes* pb);
    int32_t read();

    Erc send(Bytes* pb);
    void run();
    void mqttRead(int32_t b);
    int handler(Event*event){return 0;};
};

#endif // TCP_H

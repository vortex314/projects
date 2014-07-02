#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "Thread.h"
#include "Publisher.h"

class Tcp;

class TcpListener
{
public:
    virtual void onTcpConnect(Tcp* src)=0;
    virtual void onTcpDisconnect(Tcp* src)=0;
    virtual void onTcpMessage(Tcp* src,Bytes* data)=0;
};

class Tcp : public Thread,public Publisher<TcpListener>
{
private:
    int _sockfd;
    TcpListener *_listener;
public:
    Tcp( const char *name, unsigned short stackDepth, char priority);
    Erc connect(char *ip,int portno);

    Erc disconnect();
    Erc recv(Bytes* pb);
    int32_t read();

    Erc send(Bytes* pb);
    void run();
    void mqttRead(int32_t b);
};

#endif // TCP_H

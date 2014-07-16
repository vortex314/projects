#ifndef TCP_H
#define TCP_H

#include "Bytes.h"
#include "Thread.h"
#include "EventSource.h"
#include "Sequence.h"


class Tcp : public Thread,public Sequence
{
private:
    int _sockfd;
public:

    static EventId TCP_CONNECTED,TCP_DISCONNECTED,MQTT_MESSAGE;
    Tcp( const char *name, unsigned short stackDepth, char priority);
    ~Tcp();
    Erc connect(char *ip,int portno);

    Erc disconnect();
    Erc recv(Bytes* pb);
    int32_t read();

    Erc send(Bytes* pb);
    void run();
    void mqttRead(int32_t b);
    int handler(Event*event){return 0;};
};

#endif // TCP_H

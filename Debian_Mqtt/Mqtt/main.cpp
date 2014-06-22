#include <iostream>
#include "Bytes.h"
#include "Str.h"
#include "MqttIn.h"
#include "MqttOut.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

class Tcp
{
public :
    Tcp();
    Erc connect(char *ip,int port);
    Erc disconnect();
    Erc send(Bytes* pb);
    Erc recv(Bytes* pb);
private :
    int _sockfd;

};

Tcp::Tcp()
{
}

Erc Tcp::connect(char *ip,int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    /* Create a socket point */
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        return E_INVAL;
    }
    server = gethostbyname(ip);
    if (server == NULL)
    {
        return E_NOT_FOUND;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (::connect(_sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        return E_CONN_LOSS;
    }
    return E_OK;
}

Erc Tcp::disconnect()
{
    close(_sockfd);
    return E_OK;
}


Erc Tcp::recv(Bytes* pb)
{
    int n;

    n=::read(_sockfd,pb->data(),pb->length()) ;
    if (n < 0)
    {
        return E_CONN_LOSS;
    }
    return E_OK;
}

Erc Tcp::send(Bytes* pb)
{
    int n;
    n=write(_sockfd,pb->data(),pb->length()) ;
    if (n < 0)
    {
        return E_CONN_LOSS;
    }
    return E_OK;
}


#include "Queue.h"
#include "Event.h"

int main(int argc, char *argv[])
{
    Queue q(sizeof(Event),10);
    Event* event=new Event();
    q.put(event);
    Tcp tcp;
    MqttOut mqttOut(256);
    MqttIn mqttIn(256);
    Str str(100);
    uint16_t messageId=1000;

    tcp.connect("test.mosquitto.org",1883);
    mqttOut.Connect(0, "clientId", MQTT_CLEAN_SESSION,
                    "ikke/alive", "false", "userName", "password", 1000);
    tcp.send(&mqttOut);
    tcp.recv(&mqttIn);

    Str topic(100),msg(100);
    topic.append("ikke/topic");
    msg.append("value");
    mqttOut.Publish(MQTT_QOS2_FLAG, &str, &msg, messageId);

    tcp.send(&mqttOut);
    mqttOut.PubRel(messageId);
    tcp.send(&mqttOut);
    mqttOut.Disconnect();
    tcp.send(&mqttOut);
    tcp.disconnect();

}


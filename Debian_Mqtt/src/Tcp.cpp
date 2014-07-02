#include "Tcp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include "MqttIn.h"

Tcp::Tcp( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
{
};


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
        std::cerr << " socket connect() : " << strerror(errno) ;
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

int32_t Tcp::read()
{
    int n;
    uint8_t b;

    n=::read(_sockfd,&b,1) ;
    if (n <= 0)
    {
        return -1;
    }
    return b;
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

void Tcp::run()
{
//       ::sleep(3);
    while(true)
    {
        while ( connect((char*)"localhost",1883) != E_OK )
            sleep(5000);
        _listener->onTcpConnect(this);

        while(true)
        {
            int32_t b=read();
            if ( b < 0 ) break;
            mqttRead(b);
        }
        _listener->onTcpDisconnect(this);
        disconnect();
        sleep(5000);
    }
}
void Tcp::mqttRead(int32_t b)
{
    static MqttIn* mqttIn=new MqttIn(256);

    mqttIn->add(b);
    if (  mqttIn->complete() )
    {
        mqttIn->parse();
        _listener->onTcpMessage(this,mqttIn);
        mqttIn=new MqttIn(256);
        mqttIn->reset();
    }

}

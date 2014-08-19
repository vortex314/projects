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
#include <signal.h>
#include "MqttIn.h"

//  uint64_t TCP_CONNECTED=(uint64_t)"TcpConnected";
//',TCP_DISCONNECTED,TCP_RXD;
EventId Tcp::RXD=Event::nextEventId(( char* const )"TCP::RXD");
EventId Tcp::CONNECTED=Event::nextEventId(( char* const )"TCP::CONNECTED");
EventId Tcp::DISCONNECTED=Event::nextEventId(( char* const )"TCP::DISCONNECTED");

Tcp::Tcp( const char *host,uint16_t port)
{
    unreg();
 //   signal(SIGPIPE, SIG_IGN);
 _host=host;
 _port=port;
    _connected=false;
};

Tcp::~Tcp()
{
}

bool Tcp::isConnected(){
return _connected;
}

Erc Tcp::connect()
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    /* Create a socket point */
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        return E_INVAL;
    }
    server = gethostbyname(_host);
    if (server == NULL)
    {
        return E_NOT_FOUND;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(_port);

    /* Now connect to the server */
    if (::connect(_sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        std::cerr << " socket connect() : " << strerror(errno) ;
        return E_CONN_LOSS;
    }
    _connected=true;
    return E_OK;
}

Erc Tcp::disconnect()
{
    close(_sockfd);
    _connected=false;
    return E_OK;
}

Erc Tcp::recv(Bytes* pb)
{
    int n;

    n=::read(_sockfd,pb->data(),pb->length()) ;
    if (n < 0) {
        _connected=false;
        return E_CONN_LOSS;
    }
    return E_OK;
}
#include <cstdio>
int32_t Tcp::read()
{
    int n;
    uint8_t b;
    n=::read(_sockfd,&b,1) ;
    if (n <= 0) {
        _connected=false;
        perror("read failure");
        return -1;
    }
    return b;
}

Erc Tcp::send(Bytes* pb)
{
    int n;
 //   signal(SIGPIPE, SIG_IGN);
    n=write(_sockfd,pb->data(),pb->length()) ;
    if (n < 0) {
        perror("write failed");
        _connected=false;
        return E_CONN_LOSS;
    }
    return E_OK;
}
#include "Mutex.h"
void Tcp::run()
{
//       ::sleep(3);
    while(true)
    {
        while ( connect() != E_OK )
            sleep(5000);
        publish(Tcp::CONNECTED);

        while(true)
        {
            int32_t b=read();
            if ( b < 0 ) break;
            mqttRead(b);
        }
        publish(Tcp::DISCONNECTED);
        disconnect();
        sleep(5000);
    }
}
#include <iostream>
void Tcp::mqttRead(int32_t b)
{
    static MqttIn* mqttIn=new MqttIn(256);

    mqttIn->add(b);
    if (  mqttIn->complete() )
    {
        mqttIn->parse();
        publish(Tcp::RXD);
        mqttIn=new MqttIn(256);
        mqttIn->reset();
    }

}

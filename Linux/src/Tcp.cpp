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
#include "Logger.h"
static Logger logger(256);

Tcp::Tcp( const char *host,uint16_t port)
{
    _mqttIn.remap(new Bytes(256));
    logger.module("Tcp");
    _host=host;
    _port=port;
    _connected=false;
};

void Tcp::setHost(const char* host)
{
    _host=host;
}

void Tcp::setPort(uint16_t port)
{
    _port=port;
}

Tcp::~Tcp()
{
}

bool Tcp::isConnected()
{
    return _connected;
}

Erc Tcp::connect()
{
    logger.info() << " connect() to " << _host << " : " << _port << " ...";
    logger.flush();
    struct sockaddr_in serv_addr;
    struct hostent *server;
    /* Create a socket point */
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        logger.perror("socket()").flush();
        return E_INVAL;
    }
    server = gethostbyname(_host);
    if (server == NULL)
    {
        logger.perror("gethostbyname()").flush();
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
        logger.perror(" socket connect()  " ).flush() ;
        return E_CONN_LOSS;
    }
    _connected=true;
    logger.info() <<  "connect() connected to " << _host << " : " << _port;
    logger.flush();
    Msg::publish(SIG_TCP_CONNECTED);
//    signal(SIGPIPE, sigTcpHandler);;
    return E_OK;
}

Erc Tcp::disconnect()
{
    close(_sockfd);
    _connected=false;
    _sockfd = 0;
    Msg::publish(SIG_TCP_DISCONNECTED);
    return E_OK;
}



MqttIn* Tcp::recv()
{
    if ( _mqttIn.complete() ) return &_mqttIn;
    else return 0;
}
#include <cstdio>
uint8_t Tcp::read()
{
    int32_t n;
    uint8_t b;
    n=::read(_sockfd,&b,1) ;
    if (n <= 0)
    {
        disconnect();
        logger.perror("read()");
        return -1;
    }
    return b;
}

Erc Tcp::send(Bytes& bytes)
{
    int n;
//   signal(SIGPIPE, SIG_IGN);
//   Log::log().message("TCP send : " ,bytes);
    n=write(_sockfd,bytes.data(),bytes.length()) ;
    if (n < 0)
    {
        logger.perror("write()");
        _connected=false;
        return E_CONN_LOSS;
    }
    return E_OK;
}

#include <sys/ioctl.h>
uint32_t Tcp::hasData()
{
    int count;
    int rc = ioctl(_sockfd, FIONREAD, (char *) &count);
    if (rc < 0 || count==0 )
    {
        logger.perror("ioctl()");
        disconnect();
        return 0;
    }
    return count;
}


void Tcp::dispatch ( Msg& msg )
{
    ptRun(msg);
}
int Tcp::ptRun(Msg& msg)
{
    int32_t b;
    PT_BEGIN ( &t );
    while(true)
    {
        listen(SIG_TCP_CONNECTED);
        PT_YIELD(&t);
        while( true)
        {
            listen(SIG_TCP_RXD | SIG_TCP_DISCONNECTED );
            PT_YIELD(&t);
            if ( msg.sig()==SIG_TCP_RXD && _mqttIn.complete() == false )
            {
                while( hasData() )
                {
                    b=read();
                    _mqttIn.Feed(b);
                    if (  _mqttIn.complete() )
                    {
                        Str l(256);
                        _mqttIn.toString(l);
                        logger.debug()<<"-> USB : " <<l;
                        logger.flush();
                        _mqttIn.parse();
                        Msg m;
                        m.create(256).sig(SIG_TCP_MESSAGE).add(*_mqttIn.getBytes()).send();
                        _mqttIn.reset();
                        break;
                    }
                }
            }
            else if( msg.sig()==SIG_TCP_DISCONNECTED)
                break;
        }
    }
    PT_END ( &t );
}

int Tcp::fd()
{
    if ( _connected )
        return _sockfd;
    else
        return 0;
}




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
    _mqttIn= new MqttIn(256);
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
    MsgQueue::publish(this,SIG_CONNECTED);

//    signal(SIGPIPE, sigTcpHandler);;
    return E_OK;
}

Erc Tcp::disconnect()
{
    close(_sockfd);
    _connected=false;
    _sockfd = 0;
    MsgQueue::publish(this,SIG_DISCONNECTED);
    return E_OK;
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
    if (rc < 0  )
    {
        logger.perror("ioctl()");
        disconnect();
        return 0;
    }
    return count;
}


bool Tcp::dispatch ( Msg& msg )
{
    int32_t b;
    PT_BEGIN ( );
    while(true)
    {
        PT_YIELD_UNTIL(msg.is(0,SIG_CONNECTED,fd(),0));
        while( true)
        {
            PT_YIELD_UNTIL(msg.is(0,SIG_RXD,fd(),0)|| msg.is(0,SIG_ERC,fd(),0) );
            if ( msg.is(0,SIG_RXD,fd(),0) && _mqttIn->complete() == false )
            {
                while( hasData() )
                {
                    b=read();
                    _mqttIn->Feed(b);
                    if (  _mqttIn->complete() )
                    {
                        Str l(256);
                        if ( _mqttIn->parse())
                        {
                            _mqttIn->toString(l);
                            logger.debug()<<"RXD " <<l;
                            logger.flush();
                            MsgQueue::publish(this,SIG_RXD,_mqttIn->type(),_mqttIn);
                            _mqttIn =  new MqttIn(256);
                        }
                        else
                        {
                            _mqttIn->reset();
                        }
                        break;
                    }
                }
            }
            else if( msg.is(0,SIG_ERC,fd(),0))
                break;
        }
    }
    PT_END ( );
}

int Tcp::fd()
{
    if ( _connected )
        return _sockfd;
    else
        return 0;
}




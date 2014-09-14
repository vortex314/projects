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
#include "Log.h"

EventId Tcp::RXD=Event::nextEventId(( char* const )"TCP::RXD");
EventId Tcp::CONNECTED=Event::nextEventId(( char* const )"TCP::CONNECTED");
EventId Tcp::DISCONNECTED=Event::nextEventId(( char* const )"TCP::DISCONNECTED");
EventId Tcp::MESSAGE=Event::nextEventId(( char* const )"TCP::MESSAGE");
EventId Tcp::FREE=Event::nextEventId(( char* const )"TCP::FREE");
EventId Tcp::ERROR=Event::nextEventId(( char* const )"TCP::ERROR");

Tcp::Tcp( const char *host,uint16_t port) : msg(100) {
//   signal(SIGPIPE, SIG_IGN);
    _host=host;
    _port=port;
    _connected=false;
    };

Tcp::~Tcp() {
    }

bool Tcp::isConnected() {
    return _connected;
    }

Erc Tcp::connect() {
    Log::log() << " TCP Connecting to " << _host << " : " << _port << " ...";
    Log::log().flush();
    struct sockaddr_in serv_addr;
    struct hostent *server;
    /* Create a socket point */
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0) {
        return E_INVAL;
        }
    server = gethostbyname(_host);
    if (server == NULL) {
        return E_NOT_FOUND;
        }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(_port);

    /* Now connect to the server */
    if (::connect(_sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) {
        Log::log() << " socket connect() : " << strerror(errno) ;
        Log::log().flush();
        return E_CONN_LOSS;
        }
    _connected=true;
    Log::log() <<  "Tcp connect: connected to " << _host << " : " << _port;
    Log::log().flush();
    return E_OK;
    }

Erc Tcp::disconnect() {
    close(_sockfd);
    _connected=false;
    _sockfd = 0;
    return E_OK;
    }

MqttIn* Tcp::recv() {
    if ( msg.complete() ) return &msg;
    else return 0;
    }
#include <cstdio>
uint8_t Tcp::read() {
    int32_t n;
    uint8_t b;
    n=::read(_sockfd,&b,1) ;
    if (n <= 0) {
        _connected=false;
        perror("read failure");
        return -1;
        }
    return b;
    }

Erc Tcp::send(Bytes& bytes) {
    int n;
//   signal(SIGPIPE, SIG_IGN);
 //   Log::log().message("TCP send : " ,bytes);
    n=write(_sockfd,bytes.data(),bytes.length()) ;
    if (n < 0) {
        perror("write failed");
        _connected=false;
        return E_CONN_LOSS;
        }
    return E_OK;
    }

#include <sys/ioctl.h>
uint32_t Tcp::hasData(){
    int count;
    int rc = ioctl(_sockfd, FIONREAD, (char *) &count);
    if (rc < 0) {
        perror("ioctl failed");
        _connected=false;
        return E_CONN_LOSS;
        }
        return count;
}


int Tcp::handler ( Event* event ) {
    int32_t b;
    PT_BEGIN ( &t );
    while(true) {
        while( isConnected()) {
            PT_YIELD_UNTIL(&t,event->is(RXD) || event->is(FREE));
            if ( event->is(RXD) && msg.complete() == false ) {
                while( hasData() ) {
                    b=read();
                    msg.add(b);
                    if (  msg.complete() ) {
                        Log::log().message("TCP -> USB : " ,msg);
                        msg.parse();
                        publish(MESSAGE);
                        publish(FREE);
                        publish(RXD);  // maybe pending data
                        break;
                        }
                    PT_YIELD ( &t );
                    }
                }
            else if ( event->is(FREE)) {
                msg.reset();
                }
            PT_YIELD ( &t );
            }
        PT_YIELD_UNTIL ( &t,isConnected() );
        }
    PT_END ( &t );
    }

int Tcp::fd() {
    if ( _connected )
        return _sockfd;
    else
        return 0;
    }




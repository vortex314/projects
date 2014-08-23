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
    unreg();
//   signal(SIGPIPE, SIG_IGN);
    _host=host;
    _port=port;
    _connected=false;
    _isComplete=false;
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
    server = gethostbyname("test.mosquitto.org");
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
    return E_OK;
    }

Erc Tcp::disconnect() {
    close(_sockfd);
    _connected=false;
    _sockfd = 0;
    return E_OK;
    }

Bytes* Tcp::recv() {
if ( _isComplete ) return &msg;
else return 0;
    }
#include <cstdio>
int32_t Tcp::read() {
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

Erc Tcp::send(Bytes& pb) {
    int n;
//   signal(SIGPIPE, SIG_IGN);
    n=write(_sockfd,pb.data(),pb.length()) ;
    if (n < 0) {
        perror("write failed");
        _connected=false;
        return E_CONN_LOSS;
        }
    return E_OK;
    }
#include "Mutex.h"
void Tcp::run() {
//       ::sleep(3);
    while(true) {
        while ( connect() != E_OK )
            sleep(5000);
        publish(Tcp::CONNECTED);

        while(true) {
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
void Tcp::mqttRead(int32_t b) {
    static MqttIn* mqttIn=new MqttIn(256);

    mqttIn->add(b);
    if (  mqttIn->complete() ) {
        mqttIn->parse();
        publish(Tcp::RXD);
        mqttIn=new MqttIn(256);
        mqttIn->reset();
        }

    }

int Tcp::handler ( Event* event ) {
    int32_t b;
    PT_BEGIN ( &t );
    while(true) {
        while( isConnected()) {
            PT_YIELD_UNTIL(&t,event->is(RXD) || event->is(FREE));
            if ( event->is(RXD) && _isComplete == false ){
            while( (b=read()) >=0 ) {
                if ( msg.Feed(b)) {
                    _isComplete = true;
                    publish(MESSAGE);
                    publish(FREE);
                    publish(RXD);  // maybe pending data
                    PT_YIELD ( &t );
                    }
                }
            } else if ( event->is(FREE)) {
                msg.clear();
                _isComplete = false;
            }
            PT_YIELD ( &t );
            }
        PT_YIELD_UNTIL ( &t,isConnected() );
        }
    PT_END ( &t );
    }

int Tcp::fd() {
    return _sockfd;
    }

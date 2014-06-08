/*
 * File:   Tcp.h
 * Author: lieven
 *
 * Created on August 24, 2013, 9:39 PM
 */

#ifndef TCP_H
#define	TCP_H
#include <stdint.h>
#include "Erc.h"
#include "Bytes.h"
#include "CircBuf.h"
#include "Timer.h"
#include "Wiz5100.h"
#include "pt.h"
#include "Stream.h"
#include "Event.h"

class Tcp : public Stream {
public:

    enum TcpEvents {
        CONNECTED = EVENT('t', 'C'),
        DISCONNECTED = EVENT('t', 'D'),
        RXD = EVENT('t', 'R'),
        TXD = EVENT('t', 'T'),
        CMD_SEND = EVENT('t', 'S')
    };

    enum State {
        ST_CONNECTED = 1,
        ST_DISCONNECTED = 2,
        ST_CONNECTING
    };
    Tcp(Wiz5100* wiz,uint32_t socket);
    ~Tcp();

    int init();
    void setDstIp(uint8_t* dstIp);
    void setDstPort(uint16_t port);
    void incSrcPort();
    Erc connect();
    void disconnect();
    Erc write(Bytes* outMsg);
    int recvSize(uint16_t* size);
    int recv(Bytes& inMsg);
    Erc recvLoop();
    Erc flush();
    bool hasData();
    uint8_t read();
    char run(struct pt *pt);
    void poll();
    void state(State state);
    bool isConnected();
    bool isConnecting();
    Erc event(Event* pEvent);
private:
    Erc inConnected(Event& event);
    uint8_t _dstIp[4];
    uint16_t _dstPort;
    uint16_t _srcPort;
    uint8_t _socket;
    State _state;
    Wiz5100* _wiz;
    CircBuf _rxd;
    CircBuf _txd;
    uint32_t _bytesTxd;
    uint32_t _bytesRxd;
    uint32_t _connects;
    uint32_t _disconnects;
};


#endif	/* TCP_H */


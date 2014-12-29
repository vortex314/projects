/*
 * Handler.h
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#ifndef HANDLER_H_
#define HANDLER_H_
#include "MqttIn.h"
#include "Msg.h"


class Handler
{
    Handler* _next;
    static Handler* _first;
    uint64_t _timeout;
    uint32_t _sigMask;
public:

    Handler();
    virtual ~Handler();

    void timeout(uint32_t msec);
    bool timeout() ;
    uint64_t getTimeout();

    void listen(uint32_t signalMap);
    bool accept(Signal signal);

    virtual void dispatch(Msg& msg) {};



    static Handler* first() ;
    Handler* next();
    void reg();
};

#endif /* HANDLER_H_ */

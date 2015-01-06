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
#include "pt.h"

class Handler
{
    Handler* _next;
    static Handler* _first;
    uint64_t _timeout;
    uint32_t _sigMask;
    const char* _name;
protected:
    struct pt pt;
public:

    Handler();
    Handler(const char* name);

    virtual ~Handler() ;

    void timeout(uint32_t msec);
    bool timeout();
    uint64_t getTimeout();

    void listen(uint32_t signalMap);
    void listen(uint32_t signalMap, uint32_t time);
    bool accept(Signal signal);


    virtual void dispatch(Msg& msg);
    virtual int ptRun(Msg& msg)
    {
        return 0;
    } ;
    void restart();

    static Handler* first();
    static void dispatchAll(Msg& msg);
    static uint64_t nextTimeout();
    Handler* next();
    void reg();
};

#endif /* HANDLER_H_ */

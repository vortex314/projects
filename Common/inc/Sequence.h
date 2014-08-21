#ifndef SEQUENCE_H
#define SEQUENCE_H
#include "Event.h"
#include "Sys.h"
#include "Queue.h"
class Sequence;

#define MAX_SEQ 10

class Sequence
{
public:
    static Sequence* activeSequence[MAX_SEQ];
    static uint32_t activeSequenceCount;
private:
    uint64_t _timeout;
public:
    Sequence();
    virtual ~Sequence();
    void reg();
    void unreg();
    static void publish(int id,uint16_t detail=0);
    static Erc get(Event& event);
    static void loop();
    void timeout(uint32_t msec);
    bool timeout();
    typedef enum  {NA,OK,END} Result;
    virtual  int handler(Event* event)=0;
};

#endif // SEQUENCE_H

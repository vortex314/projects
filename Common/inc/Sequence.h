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
    Sequence()
    {
        reg();
    };
    ~Sequence()
    {
        unreg();
    };
    void reg()
    {
        int i;
        for (i=0; i<MAX_SEQ; i++)
                if(activeSequence[i]==0) {
                    activeSequence[i]=this;
                    break;
                };
    };
    void unreg()
    {
        int i;
        for (i=0; i<MAX_SEQ; i++) if(activeSequence[i]==this) activeSequence[i]=0;
    };
    void publish(Event* ev)
    {
        Queue::getDefaultQueue()->put(ev);
    }
    void timeout(uint32_t msec)
    {
        _timeout=Sys::upTime()+msec;
    }
    bool timeout()
    {
        return _timeout < Sys::upTime();
    }
    typedef enum  {NA,OK,END} Result;
    virtual  int handler(Event* event)=0;
};

#endif // SEQUENCE_H

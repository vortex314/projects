#include "Sequence.h"

uint32_t Sequence::activeSequenceCount=0;
Sequence* Sequence::activeSequence[MAX_SEQ];
Sequence::Sequence()
{
    reg();
};

Sequence::~Sequence(){
    unreg();
}

void Sequence::reg()
{
    int i;
    for (i=0; i<MAX_SEQ; i++)
        if(activeSequence[i]==0)
        {
            activeSequence[i]=this;
            break;
        };
};
void Sequence::unreg()
{
    int i;
    for (i=0; i<MAX_SEQ; i++)
        if(activeSequence[i]==this)
            activeSequence[i]=0;
};


void Sequence::publish(int id)
{
    Event ev(id);
    Queue::getDefaultQueue()->put(&ev);
}

void Sequence::timeout(uint32_t msec)
{
    _timeout=Sys::upTime()+msec;
}
bool Sequence::timeout()
{
    return _timeout < Sys::upTime();
}

#include "Sequence.h"
#include "QueueTemplate.h"

QueueTemplate<Event> defaultQueue(10);

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


void Sequence::publish(int id,uint16_t detail)
{
    Event ev(id);
    defaultQueue.put(ev);
}

void Sequence::timeout(uint32_t msec)
{
    _timeout=Sys::upTime()+msec;
}
bool Sequence::timeout()
{
    return _timeout < Sys::upTime();
}

#include "Timer.h"
#include "pt.h"
#include "Log.h"
extern Log log;
<<<<<<< HEAD
/*
=======

Erc Sequence::get(Event& event){
	return defaultQueue.get ( event );
}
>>>>>>> 292d18033dbc6214534388472cfb0488ae399486

void  Sequence::loop() {
    Event event;
    defaultQueue.get ( event ); // dispatch eventually IDLE message
    if ( event.id() != Timer::TICK ) {
        log << " EVENT : " ;
        event.toString(log);
        log.flush();
        }

    int i;
    for ( i = 0; i < MAX_SEQ; i++ )
        if ( Sequence::activeSequence[i] ) {
            if ( Sequence::activeSequence[i]->handler ( &event ) == PT_ENDED ) {
                Sequence* seq = Sequence::activeSequence[i];
                seq->unreg();
                delete seq;
                };
            }

    };*/

/*
 * File:   Streams.cpp
 * Author: lieven2
 *
 * Created on 15 september 2013, 17:58
 */

#include "Stream.h"

#define MAX_LISTENS 20


uint32_t listeners=0;

Listener* Stream::_listeners = (Listener*)NULL;
void Stream::eventHandler(Event* pEvent)
{

}

Stream::Stream()
{}


void Stream::addListener(Stream *dst)
{
    addListener(dst,-1);
}

void Stream::addListener(Stream *dst,int32_t newId)
{
    Listener* cursor ;
    if ( _listeners == NULL )
    {
        _listeners = new Listener();
        cursor=_listeners;
    }
    else
    {
        cursor=_listeners;
        while(cursor->next!=NULL) cursor=cursor->next;
        cursor->next=new Listener();
        cursor=cursor->next;
    };


    cursor->next = (Listener*)NULL;
    cursor->dst = dst;
    cursor->src=this;
    cursor->id=newId;
}

Listener* Stream::getListeners()
{
    return _listeners;
}

void Stream::publish(int32_t id)
{
    publish(id,NULL);
}

void Stream::publish(int32_t id,EventData *data)
{
    Queue::getDefaultQueue()->put(new Event(this,id,data));
}
 /*
uint32_t Stream::getEvents()
{
    uint32_t temp=_events;
    _events=0;
    return temp;
}

void Stream::setEvents(uint32_t ev)
{
    _events |= ev;
    Listener* listener = Stream::getListeners();
    while(listener!=NULL)
    {
        if ( event.src() == listener->src )
            if (( listener->id== -1) || ( listener->id == event.id()))
                listener->dst->eventHandler(&event);
        listener = listener->next;
    }
}*/

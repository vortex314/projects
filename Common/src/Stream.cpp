/*
 * File:   Streams.cpp
 * Author: lieven2
 *
 * Created on 15 september 2013, 17:58
 */

#include "Stream.h"

#define MAX_LISTENS 20


uint32_t listeners=0;

Queue* Stream::_defaultQueue=(Queue*)NULL;
Listener* Stream::_listeners = (Listener*)NULL;
void Stream::eventHandler(Event* pEvent)
{

}

Stream::Stream()
{}

Queue* Stream::getDefaultQueue()
{
    if (_defaultQueue == NULL)
        _defaultQueue = new Queue(sizeof(Event),QUEUE_DEPTH );
    return _defaultQueue;
}

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

void Stream::publish(int32_t id,void *data)
{
    getDefaultQueue()->put(new Event(this,id,data));
}


/*
 * File:   Streams.h
 * Author: lieven2
 *
 * Created on 15 september 2013, 17:58
 */

#ifndef STREAMS_H
#define	STREAMS_H
#include <stdint.h>
#include "Erc.h"
#include "Bytes.h"
#include "QueueTemplate.h"
#include "Event.h"
#include "Queue.h"
#define QUEUE_DEPTH 10
#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct ListenStruct
{
    struct ListenStruct* next;
    Stream *dst;
    int32_t newId;
}    Listener;

class Stream
{
public:
    /*   virtual Erc post(Event *pEvent)
       {
           getDefaultQueue()->put(pEvent);
           return E_OK;
       }
       virtual Erc postFromIsr(Event *pEvent)
       {
           getDefaultQueue()->put(pEvent);
           return E_OK;
       }*/
    virtual void eventHandler(Event* pEvent);
    Stream()
    {
        _listener=(Listener*)NULL;
    }

    /*   Stream(Stream* str)
       {
           upStream(str);
           _listener=(Listener*)NULL;
       }
       inline Stream* upStream()
       {
           return _upStream;
       }
       void upStream(Stream* up)
       {
           _upStream = up;
       }
       Erc upQueue(uint32_t id)
       {

           Event ev(_upStream, this, id, NULL);
           return upStream()->post(&ev);
       }
    */
    static Queue* getDefaultQueue()
    {
        if (_defaultQueue == NULL)
            _defaultQueue = new Queue(QUEUE_DEPTH, sizeof(Event));
        return _defaultQueue;
    }
    /*   virtual void wait(int timeout)
       {
           _upStream->wait(timeout);
       }
       virtual void notify()
       {
           _upStream->notify();
       }*/

    void addListener(Stream *ps)
    {
        Listener* cursor =  _listener;
        while(cursor!=NULL) cursor=cursor->next;
        cursor=new Listener();
        cursor->next = (Listener*)NULL;
        cursor->dst = ps;
        cursor->newId=-1;
    }

    void addListener(Stream *ps,int32_t newId)
    {
        Listener* cursor =  _listener;
        while(cursor!=NULL) cursor=cursor->next;
        cursor=new Listener();
        cursor->next = (Listener*)NULL;
        cursor->dst = ps;
        cursor->newId=newId;
    }

    void publish(int32_t id)
    {
        Listener* cursor =  _listener;
        while(cursor!=NULL)
        {
            getDefaultQueue()->put(new Event(cursor->dst,this,id,NULL));
            cursor=cursor->next;
        }
    }

    void publish(int32_t id,void *data)
    {
        Listener* cursor =  _listener;
        while(cursor!=NULL)
        {
            getDefaultQueue()->put(new Event(cursor->dst,this,id,data));
            cursor=cursor->next;
        }
    }



private:
    Stream* _upStream;
    Listener* _listener;
    static Queue* _defaultQueue;
};
/*
 class Stream {
 public:
 virtual Erc push(Event* pEvent)=0;
 Stream();
 Stream(Stream* upStream);
 ~Stream();
 virtual Erc event(Event* event)=0;
 Erc enqueue(Stream* dst, Stream *src,uint32_t event);
 Erc enqueue(Event *event);
 Erc upQueue(uint32_t event);
 Erc dequeue(Event* event);
 Stream* getUpStream();
 void setUpStream(Stream* stream);
 Queue* getQueue();
 void setQueue(Queue* q);
 static Queue* getDefaultQueue();

 private:
 Stream* _upStream;
 //    static  QueueTemplate<class Event> _queue;
 Queue* _queue;
 static Queue* _defaultQueue;
 static Stream* _nullStream;

 };

 */

#endif	/* STREAMS_H */



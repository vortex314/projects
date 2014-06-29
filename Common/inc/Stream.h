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
    Stream *src;
    int32_t id;
}    Listener;

class Stream
{
private:
    Stream* _upStream;
    static Listener* _listeners;
    static Queue* _defaultQueue;
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
        _listeners=(Listener*)NULL;
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
            _defaultQueue = new Queue(sizeof(Event),QUEUE_DEPTH );
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

    void addListener(Stream *dst)
    {
        addListener(dst,-1);
    }

    void addListener(Stream *dst,int32_t newId)
    {
        Listener* cursor ;
        if ( _listeners == NULL )
        {
            _listeners = new Listener();
            cursor=_listeners;
        }
        else
        {
            while(cursor->next!=NULL) cursor=cursor->next;
            cursor->next=new Listener();
        };


        cursor->next = (Listener*)NULL;
        cursor->dst = dst;
        cursor->src=this;
        cursor->id=newId;
    }

    static Listener* getListeners()
    {
        return _listeners;
    }

    void publish(int32_t id)
    {
        publish(id,NULL);
    }

    void publish(int32_t id,void *data)
    {
        getDefaultQueue()->put(new Event(this,id,data));
    }




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



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
//    uint32_t _events;
public:
    virtual void eventHandler(Event* pEvent);
    Stream();
    static Queue* getDefaultQueue();
    void addListener(Stream *dst);
    void addListener(Stream *dst,int32_t newId);
    static Listener* getListeners();
    void publish(int32_t id);
    void publish(int32_t id,EventData *data);
 //   uint32_t getEvents();
 //   void setEvents(uint32_t map);
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



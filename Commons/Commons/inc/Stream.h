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
#include "QueueWrapper.h"

class Stream {
public:
	virtual Erc post(Event *pEvent){
		return E_OK; // define how to store or queue events
	}
	inline Stream* upStream() {
		return _upStream;
	}
	void upStream(Stream* up) {
		_upStream = up;
	}

private:
	Stream* _upStream;
};/*
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


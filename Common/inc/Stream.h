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
		getDefaultQueue()->send(&pEvent);
		return E_OK;
	}
	virtual Erc postFromIsr(Event *pEvent){
		getDefaultQueue()->sendFromIsr(&pEvent);
		return E_OK;
	}
	virtual Erc event(Event* pEvent);
	Stream() {
	}
	;
	Stream(Stream* str) {
		upStream(str);
	}
	inline Stream* upStream() {
		return _upStream;
	}
	void upStream(Stream* up) {
		_upStream = up;
	}
	Erc upQueue(uint32_t id) {
		return upStream()->post(new Event(this, _upStream, id));
	}

	Erc queue(uint32_t id) {
		return post(new Event(this, this, id));
	}
	static Queue*  getDefaultQueue(){
			if ( _defaultQueue== NULL ) _defaultQueue=new Queue(10,4);
			return _defaultQueue;
		}

private:
	Stream* _upStream;
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


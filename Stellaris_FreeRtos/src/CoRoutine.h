/*
 * CoRoutine.h
 *
 *  Created on: 4-jun.-2014
 *      Author: lieven2
 */

#ifndef COROUTINE_H_
#define COROUTINE_H_
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "pt.h"
/*
 * Stream : post, postFromISr(Event*)
 * Thread : run(),
 * ThreadStream : has own queue and thread
 * CoRoutine : common thread
 * CoRoutineStream :
 *
 */
#include "QueueWrapper.h"
#include "Event.h"

class CoRoutine : public Stream {
public:
	struct pt _pt;
	static Queue* _queue;
public:
	CoRoutine(){
		getDefaultQueue();
	}
	CoRoutine(int priority);
	virtual ~CoRoutine();
	Queue*  getDefaultQueue(){
		if ( _queue== NULL ) _queue=new Queue(10,4);
		return _queue;
	}
	virtual char run(Event* pEvent){
		PT_BEGIN(&_pt);
		PT_WAIT_WHILE(&_pt,false);
		PT_END(&_pt);
	}
	Erc post(Event* pEvent){
		return _queue->send(&pEvent);
	}
};

#endif /* COROUTINE_H_ */

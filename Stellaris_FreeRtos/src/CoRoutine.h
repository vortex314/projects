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


class CoRoutine {
public:
	xCoRoutineHandle _coRoutineHandle;
public:
	CoRoutine(int priority);
	virtual ~CoRoutine();
	virtual void run(xCoRoutineHandle handle)=0;
};

#endif /* COROUTINE_H_ */

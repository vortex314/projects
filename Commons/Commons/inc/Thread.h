/*
 * Thread.h
 *
 *  Created on: 2-jun.-2014
 *      Author: lieven2
 */

#ifndef THREAD_H_
#define THREAD_H_
#include "FreeRTOS.h"
#include "task.h"

class Thread {
private:
	xTaskHandle _taskHandle;
public:
	Thread(const char *name, unsigned short stackDepth, char priority);
	~Thread(){};
	virtual void run() =0;
};

#endif /* THREAD_H_ */

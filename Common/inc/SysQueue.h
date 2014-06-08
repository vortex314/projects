/*
 * SysQueue.h
 *
 *  Created on: 25-mei-2014
 *      Author: lieven2
 */

#ifndef SYSQUEUE_H_
#define SYSQUEUE_H_
#include "Erc.h"
#include "Sys.h"
#include "FreeRTOS.h"
#include "queue.h"
class SysQueue {
public:
	SysQueue(uint32_t count,uint32_t size);
	virtual ~SysQueue();
	Erc put(void *ptr);
	Erc putFromISR(void *ptr);
	Erc get(void * ptr,uint32_t timeout);
	uint32_t size();
	static SysQueue* createMainQueue(uint32_t count,uint32_t size);
	static SysQueue* getMainQueue();
private :
	uint32_t _size;
	xQueueHandle _qh;
};

#endif /* SYSQUEUE_H_ */

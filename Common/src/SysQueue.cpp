/*
 * SysQueue.cpp
 *
 *  Created on: 25-mei-2014
 *      Author: lieven2
 */

#include "SysQueue.h"
#ifdef FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
SysQueue* mainQueue = NULL;


SysQueue::SysQueue(uint32_t count, uint32_t size) {
	_qh = xQueueCreate(count,size);
	_size = size;
}

SysQueue::~SysQueue() {
	vQueueDelete(_qh);
	_qh = NULL;
}

SysQueue* SysQueue::createMainQueue(uint32_t count, uint32_t size) {
	mainQueue = (SysQueue*) Sys::malloc(sizeof(SysQueue));
	mainQueue->_qh = xQueueCreate(count,size);
	mainQueue->_size = size;
	return mainQueue;
}

SysQueue* SysQueue::getMainQueue() {
	if (mainQueue == NULL)
		createMainQueue(4, 20);
	return mainQueue;
}

uint32_t SysQueue::size() {
	return uxQueueMessagesWaiting(_qh);
}

Erc SysQueue::put(void* item) {
	void *it=item;
	uint32_t er = xQueueSend(_qh,&it,0);
	if (er == pdTRUE)
		return E_OK;
	return E_LACK_RESOURCE;
}

Erc SysQueue::putFromISR(void* item) {
	void *it=item;
	uint32_t er = xQueueSendFromISR(_qh,&it,0);
	if (er == pdTRUE)
		return E_OK;
	return E_LACK_RESOURCE;
}

Erc SysQueue::get(void *item, uint32_t timeout) {

	if (timeout == 0)
		timeout = portMAX_DELAY;
	uint32_t er = xQueueReceive(_qh,item,timeout);
	if (er == pdTRUE)
		return E_OK;
	return E_LACK_RESOURCE;
}
#else
#include "QueueTemplate.h"
//QueueTemplate<class Event> Stream::_queue(20);
#endif

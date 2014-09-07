/*
 * Thread.cpp
 *
 *  Created on: 2-jun.-2014
 *      Author: lieven2
 */

#include "Thread.h"

extern "C" void pvTaskCode(void *pvParameters) {
	(static_cast<Thread*>(pvParameters))->run();
}

Thread::Thread(const char *name, unsigned short stackDepth, char priority) {
	xTaskCreate((pdTASK_CODE)pvTaskCode, (const signed char *) name, stackDepth,
			(void*) this, priority, &_taskHandle);
	_semaphore = xSemaphoreCreateCounting(10,0);
}

Erc Thread::sleep(uint32_t time) {
	vTaskDelay(time);
	return E_OK;
}

Erc Thread::yield() {
	taskYIELD();
	return E_OK;
}

Erc Thread::wakeup() {
	vTaskResume(_taskHandle);
	return E_OK;
}

void Thread::wait(int timeout) {
	xSemaphoreTake( _semaphore, timeout);
}

void Thread::notify() {
	xSemaphoreGive( _semaphore);
}

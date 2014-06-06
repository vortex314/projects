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
    xTaskCreate((pdTASK_CODE)pvTaskCode, (const signed char *) name, stackDepth, (void*) this, priority, &_taskHandle);
}

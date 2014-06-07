/*
 * CoRoutine.cpp
 *
 *  Created on: 4-jun.-2014
 *      Author: lieven2
 */

#include "CoRoutine.h"
Queue* CoRoutine::_queue=NULL;
extern "C" void pvCoRoutineCode(xCoRoutineHandle handle,void* me) { // re-use index parameter as pointer to class instance
	(static_cast<CoRoutine*>(me))->runCR(handle);
}

CoRoutine::CoRoutine(int priority) {
	xCoRoutineCreate((crCOROUTINE_CODE) pvCoRoutineCode, priority, (unsigned long)this);
}

CoRoutine::~CoRoutine() {

}

void CoRoutine::runCR(xCoRoutineHandle handle) {

}

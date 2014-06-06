/*
 * Stream.h
 *
 *  Created on: 6-jun.-2014
 *      Author: lieven2
 */

#ifndef STREAM_H_
#define STREAM_H_
#include <stdint.h>
#include "Erc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
namespace Micro {

class Stream;

typedef struct {
	Stream* dst;
	Stream* src;
	uint32_t type;
} EventHdr;

typedef struct {
	EventHdr hdr;
	union {
		uint8_t bData[1];
		uint32_t lData[1];
		void* pData;
	};
} Event;

//_____________________________________________________________________________________

class Stream {
public:
	virtual Erc post(Event *pEvent)=0; // define how to store or queue events
	inline Stream* upStream() {
		return _upStream;
	}
	void upStream(Stream* up) {
		_upStream = up;
	}

private:
	Stream* _upStream;
};

//_____________________________________________________________________________________

class Thread {
public:
	Thread(const char *name, unsigned short stackDepth, char priority);
	virtual void run()=0; // free running thread
private:
	xTaskHandle _taskHandle;

};

extern "C" void pvTaskCode(void *pvParameters) {
	(static_cast<Thread*>(pvParameters))->run();
}

Thread::Thread(const char *name, unsigned short stackDepth, char priority) {
	xTaskCreate((pdTASK_CODE) pvTaskCode, (const signed char *) name,
			stackDepth, (void*) this, priority, &_taskHandle);
}

//_____________________________________________________________________________________

class ThreadStream: public Thread, public Stream {
public:
	ThreadStream(const char *name, unsigned short stackDepth, char priority) :
			Thread(name, stackDepth, priority) {
		_queue = xQueueCreate(10, 4);
	}
	inline Erc post(Event* pEvent) {
		if (xQueueSend(_queue, &pEvent, 0))
			return E_LACK_RESOURCE;
		return E_OK;
	}
private:
	xQueueHandle _queue;
};

//_____________________________________________________________________________________

class CoRoutine {
public:
	xCoRoutineHandle _coRoutineHandle;
public:
	CoRoutine(int priority);
	virtual ~CoRoutine();
	virtual void run(xCoRoutineHandle handle)=0;
};

//_____________________________________________________________________________________

extern "C" void pvCoRoutineCode(xCoRoutineHandle handle, void* me) { // re-use index parameter as pointer to class instance
	(static_cast<CoRoutine*>(me))->run(handle);
}

CoRoutine::CoRoutine(int priority) {
	xCoRoutineCreate((crCOROUTINE_CODE) pvCoRoutineCode, priority,
			(unsigned long) this);
}

CoRoutine::~CoRoutine() {

}

//_____________________________________________________________________________________

class CoRoutineStream: public CoRoutine, public Stream {
public:
	Erc post(Event *pEvent) {

	}
};

class RunToCompletionStream {
};

} /* namespace Micro */
#endif /* STREAM_H_ */

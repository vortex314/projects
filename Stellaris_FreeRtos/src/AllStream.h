/*
 * Stream.h
 *
 *  Created on: 6-jun.-2014
 *      Author: lieven2
 */

#ifndef STREAM_H_
#define STREAM_H_

namespace Micro {

typedef struct {
	Stream* dst, src;
	uint32_t type;
} EventHdr;

typedef struct {
	EventHdr hdr;
	union {
		uint8_t bData[];
		uint32_t lData[];
		void* pData;
	};
} Event;

typedef struct {
} MqttConnect;

typedef struct {
	EventHdr hdr;
	MqttConnect mqttConnect;
} EventMQttConnect;

MqttConnect* qc = (EventMqttConnect*) pEvent;

//_____________________________________________________________________________________

class Stream {
public:
	virtual void post(Event *pEvent)=0; // define how to store or queue events
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
};

class RunToCompletionStream {
};

} /* namespace Micro */
#endif /* STREAM_H_ */

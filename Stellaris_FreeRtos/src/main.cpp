#include <stdio.h>
#include <inc/hw_can.h>
#include <inc/hw_sysctl.h>
#include "Sys.h"
#include <driverlib/systick.h>

#include "Led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "Spi.h"
#include <stdint.h>
#include "Erc.h"
#include "Event.h"
#include "Spi.h"

extern void SysTickHandler(void);

extern "C" void vApplicationTickHook(void) {
	SysTickHandler();
}

extern "C" void vApplicationStackOverflowHook(xTaskHandle *pxTask,
		signed char *pcTaskName) {
//bad_task_handle = pxTask;     // this seems to give me the crashed task handle
//bad_task_name = pcTaskName;     // this seems to give me a pointer to the name of the crashed task
//mLED_3_On();   // a LED is lit when the task has crashed
	while (1)
		;
}

//_____________________________________________________________________________________

#include "Thread.h"

//_____________________________________________________________________________________

class ThreadStream: public Thread, public Stream {
public:
	ThreadStream(const char *name, unsigned short stackDepth, char priority) :
			Thread(name, stackDepth, priority) {
		_queue = new Queue(10, 4);
	}
	inline Erc post(Event* pEvent) {
		if (_queue->send(&pEvent))
			return E_LACK_RESOURCE;
		wakeup();
		return E_OK;
	}
	Erc postFromIsr(Event* pEvent) {
		if (_queue->sendFromIsr(&pEvent))
			return E_LACK_RESOURCE;
		wakeup();
		return E_OK;
	}
	Event* wait(Stream* str, uint32_t event, uint32_t timeout) {
		Event* pEvent;
		while (true) {
			if (_queue->receive(&pEvent, timeout) == pdFALSE)
				return NULL;
			if ((str == pEvent->src()) && (pEvent->is(event)))
				return pEvent;
		}
	}
	Queue* queue() {
		return _queue;
	}

private:
	Queue* _queue;
};

#include "CoRoutine.h"
//_____________________________________________________________________________________

class MainThread: public ThreadStream {
public:
	MainThread() :
			ThreadStream("Main", configMINIMAL_STACK_SIZE, 4) {
	}

	void run() {
		Event* pEvent;
		while (true) { // EVENTPUMP
			if (queue()->receive(&pEvent)) {
				//TODO handle event
				Sys::free(pEvent); // free up event
			}
		}
	}
};

#include "Wiz5100.h"

class Wiz810Thread: public ThreadStream {
public:
	Wiz810Thread() :
			ThreadStream("wiz810", configMINIMAL_STACK_SIZE, 5) {
		_spi = new Spi(0);
		_spi->upStream(this);
	}

	Erc spiSend(uint16_t addr, uint8_t data) {
		Event *pEvent;
		Bytes response(10);
		_spi->write(W5100_WRITE_OPCODE);
		_spi->write(addr >> 8);
		_spi->write(addr & 0xFF);
		_spi->write(data);
		_spi->flush();
		if (pEvent = wait(_spi, Spi::RXD, 10)) {
			while (_spi->hasData())
				response.write(_spi->read());
			if (response.length() != 4)
				return E_CONN_LOSS;
			return E_OK;
		}
		return E_TIMEOUT;

	}

	int exchange(uint32_t out, uint32_t& in) {
		Bytes outB(4);
		Bytes inB(4);

	}

	void run() {

		for (;;) {
			spiSend(0x0500, 5);
			vTaskDelay(1000);
		}
	}

	Spi* _spi;
};

Wiz810Thread* wiz;
MainThread* mainT;
#include "CoRoutine.h"
Led ledRed(Led::LED_RED);
Led ledBlue(Led::LED_BLUE);
class CoR: public CoRoutine {

public:
	CoR() :
			CoRoutine(4) {
	}

	void runCR(xCoRoutineHandle handle) {
		crSTART(handle)
		;
		for (;;) {
			crDELAY(handle, 100);
			ledBlue.toggle();
		}
	crEND();
}

};

CoR* cor;

extern "C" void vApplicationIdleHook(void) {
	Event* pEvent;
	vCoRoutineSchedule();
	Queue *q = CoRoutine::getDefaultQueue();
	while (q->receive(&pEvent, 0)) {
		((CoRoutine*) (pEvent->dest()))->run(pEvent);
		Sys::free(pEvent);
	}
}

int main(void) {
	Sys::init();

	Led ledGreen(Led::LED_GREEN);

	ledGreen.blink(1);
//	ledRed.blink(2);
//	ledBlue.blink(3);
//	ledRed.light(false);
//	ledBlue.light(false);
	wiz = new Wiz810Thread();
	mainT = new MainThread();
	cor = new CoR();

	vTaskStartScheduler();

}

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
		_queue = new Queue(10,4);
	}
	inline Erc post(Event* pEvent) {
		if (xQueueSend(_queue, &pEvent, 0))
			return E_LACK_RESOURCE;
		return E_OK;
	}
	Queue* queue() { return _queue;};
private:
	Queue* _queue;
};



//_____________________________________________________________________________________

class CoRoutineStream: public CoRoutine, public Stream {
public:
	Erc post(Event *pEvent) {
return E_OK;
	}
};

class RunToCompletionStream {
};



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
		_spi->setUpStream(this);
	}
	Erc event(Event* pEvent) {
		return E_OK;
	}

	Erc write(uint16_t addr,uint8_t data) {
		Bytes response(10);
		_spi->write(W5100_WRITE_OPCODE);
		_spi->write(addr>>8);
		_spi->write(addr&0xFF);
		_spi->write(data);
		_spi->flush();
//		wait(_spi,_spi->RXD,10);
		while ( _spi->hasData() )
			response.write(_spi->read());
		if ( response.length() != 4  ) return E_CONN_LOSS;
		if (rxd != 0x00010203)
					return E_AGAIN;
/*		Event* pEvent;
		_spi->send(out);
		in.clear();
		if (getQueue()->receive(&pEvent, 500) == false) // wait 500msec
			return E_TIMEOUT;
		if (pEvent->src() != _spi) // unexpected event
			while (1)
				;
		Sys::free(pEvent);*/
		return E_OK;
	}

	int exchange(uint32_t out, uint32_t& in) {
		Bytes outB(4);
		Bytes inB(4);

	}


private:
	void run() {
		Bytes out(10);
		Bytes in(10);
		for (;;) {
			out.clear();
			out.write((uint8_t*) "Hello", 0, 5);
			spiExchange(out, in);
			vTaskDelay(10);
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

	void run(xCoRoutineHandle handle) {
		crSTART(handle);
		for (;;) {
			crDELAY(handle, 100);
			ledBlue.toggle();
		}
		crEND();
	}

};

CoR* cor;

extern "C" void vApplicationIdleHook( void )
   {
       vCoRoutineSchedule(  );
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

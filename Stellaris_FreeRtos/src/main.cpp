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
				pEvent->dest()->event(pEvent);
				Sys::free(pEvent); // free up event
			}
		}
	}
};

#include "Wiz5100.h"
#include "Tcp.h"
#include "MqttIn.h"
#include "MqttOut.h"

uint8_t mac[] = { 0x9B, 0xA9, 0xE9, 0xF2, 0xF3, 0x46 };
uint8_t ip[] = { 192, 168, 0, 255 };
uint8_t gtw[] = { 192, 168, 0, 1 };
uint8_t mask[] = { 255, 255, 255, 0 };
uint8_t test_mosquitto_org[] = { 85, 119, 83, 194 };

class ConnectTask: public Thread, public Stream {
public:
	ConnectTask(const char *name, unsigned short stackDepth, char priority) :
			Thread(name, stackDepth, priority) {
		_queue = new Queue(10, 4);
		_spi = new Spi(0);
		_wiz = new Wiz5100(_spi, 0);
		_tcp = new Tcp(_wiz, 0); // use socket 0
		_tcp->upStream(this);
		_mqttIn = new MqttIn(256);
		_mqttOut = new MqttOut(256);
	}
	void run() {
		_wiz->init();
		_wiz->reset();
		_wiz->loadCommon(mac, ip, gtw, mask);

		while (true) {
			_tcp->init();
			_tcp->setDstIp(test_mosquitto_org);
			_tcp->setDstPort(1883);
			while (_tcp->connect() == E_CONN_LOSS)
				vTaskDelay(3000);
			_mqttOut->Connect(0, "clientId", MQTT_CLEAN_SESSION, "willTopic",
					"willMsg", "userName", "password", 1000);
			_tcp->write(_mqttOut);
			_tcp->flush();
			while (_tcp->hasData()) {
				_mqttIn->add(_tcp->read());
			}
			_tcp->disconnect();
			vTaskDelay(1000);
		}
	}
private:
	Queue* _queue;
	Spi* _spi;
	Wiz5100* _wiz;
	Tcp* _tcp;
	MqttIn* _mqttIn;
	MqttOut* _mqttOut;
};

MainThread* mainT;

#include "CoRoutine.h"

Led ledRed(Led::LED_RED);
Led ledBlue(Led::LED_BLUE);
Led ledGreen(Led::LED_GREEN);

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
/*	Event* pEvent;
	vCoRoutineSchedule();
	Queue *q = CoRoutine::getDefaultQueue();
	while (q->receive(&pEvent, 0)) {
		((CoRoutine*) (pEvent->dest()))->run(pEvent);
		Sys::free(pEvent);
	}*/
}

int main(void) {
	Sys::init();

	Led ledGreen(Led::LED_GREEN);

	ledGreen.blink(1);
//	ledRed.blink(2);
//	ledBlue.blink(3);
//	ledRed.light(false);
//	ledBlue.light(false);

	mainT = new MainThread();
	ConnectTask* connect = new ConnectTask("Conn",
			configMINIMAL_STACK_SIZE + 100, 4);
//	cor = new CoR();

	vTaskStartScheduler();

}

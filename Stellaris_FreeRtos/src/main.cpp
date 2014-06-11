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


extern "C" void vApplicationIdleHook(void) {
	/*
	 vCoRoutineSchedule();
	 */
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

class MainThread: public Thread {
public:
	MainThread() :
			Thread("Main", configMINIMAL_STACK_SIZE, 4) {
	}

	void run() {
		Event event;
		while (true) {
			if (Stream::getDefaultQueue()->receive(&event)) {
				event.dst()->event(&event);
			}
		}
	}
};

#include "Wiz5100.h"
#include "Tcp.h"
#include "MqttIn.h"
#include "MqttOut.h"
#include "Str.h"

uint8_t mac[] = { 0x9B, 0xA9, 0xE9, 0xF2, 0xF3, 0x46 };
uint8_t ip[] = { 192, 168, 0, 111 };
uint8_t gtw[] = { 192, 168, 0, 1 };
uint8_t mask[] = { 255, 255, 255, 0 };
uint8_t test_mosquitto_org[] = { 85, 119, 83, 194 };
uint8_t pcpav2[] = { 192, 168, 0, 206 };
uint8_t pi[] = { 192, 168, 0, 128 };

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
		_topic = new Str(32);
		_msg = new Str(100);
		_messageId = 1234;
		_clientId = new Str(20);
	}
	void run() {
		int i;
		_wiz->init();
		_wiz->reset();
		_wiz->loadCommon(mac, ip, gtw, mask);
//		_mqttOut->prefix("ikke/");

		while (true) {
			_messageId++;
			_clientId->append("ikke");
			_clientId->append("messageId");
			_tcp->setDstIp(pi);
			_tcp->setDstPort(1883);
			_tcp->init();
			while (_tcp->connect() == E_CONN_LOSS)
				vTaskDelay(3000);
			_mqttOut->Connect(0, _clientId->data(), MQTT_CLEAN_SESSION,
					"ikke/alive", "false", "userName", "password", 1000);
			_tcp->write(_mqttOut);
			_tcp->flush();
			for (i = 0; i < 100; i++) {
				_topic->clear();
				_msg->clear();
				_topic->append("ikke/topic");
				_msg->append("topicValue_");
				_msg->append(i);
				_mqttOut->Publish(MQTT_QOS2_FLAG, _topic, _msg, _messageId);
				_tcp->write(_mqttOut);
				_tcp->flush();
				_mqttOut->PubRel(_messageId);
				_tcp->write(_mqttOut);
				_tcp->flush();
				vTaskDelay(100);
			};
			_mqttOut->Disconnect();
			_tcp->write(_mqttOut);
			_tcp->flush();
			vTaskDelay(10);
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
	uint16_t _messageId;
	Str* _topic;
	Str* _msg;
	Str* _clientId;
};

MainThread* mainT;

#include "CoRoutine.h"

Led ledRed(Led::LED_RED);
Led ledBlue(Led::LED_BLUE);
Led ledGreen(Led::LED_GREEN);



int main(void) {
	Sys::init();

	Led ledGreen(Led::LED_GREEN);
	Event event(&ledGreen, (void*) NULL, 1234, 0);

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

#include "CoRoutine.h"

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

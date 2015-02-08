/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Board.h"
#include "Mqtt.h"
#include "Timer.h"
#include "Prop.h"
#include "Cbor.h"
#include "Msg.h"

#include <malloc.h>
#include "Handler.h"

class TempTopic: public Prop {
public:
	TempTopic() :
			Prop("system/temp", (Flags )
					{ T_FLOAT, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
	}
	void toBytes(Bytes& bytes) {
		Json json(bytes);
		json.add(Board::getTemp());
	}
};

class HardwareTopic: public Prop {
public:
	HardwareTopic() :
			Prop("system/hardware", (Flags )
					{ T_OBJECT, M_READ, T_10SEC, QOS_1, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json msg(message);
		msg.addMap(4);
		msg.addKey("cpuRevision");
		Bytes b(8);
		Board::processorRevision(b);
		msg.add(b);
		msg.addKey("cpu");
		msg.add("LM4F120H5QR");
		msg.addKey("board");
		msg.add("EK-LM4F120XL");
		msg.addKey("bool");
		msg.add(false);
		msg.addBreak();
	}
};

class UptimeTopic: public Prop {
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags )
					{ T_UINT64, M_READ, T_1SEC, QOS_0, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(Sys::upTime());
	}
};

static uint64_t bootTime;
class RealTimeTopic: public Prop {
public:

	RealTimeTopic() :
			Prop("system/now", (Flags )
					{ T_UINT64, M_RW, T_1SEC, QOS_0, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(bootTime + Sys::upTime());
	}
	void fromBytes(Bytes& message) {
		Json json(message);
		double d;
		uint64_t now;
		if (json.get(d)) {
			now = d;
			bootTime = now - Sys::upTime();
		}
	}
};

class SystemOnlineTopic: public Prop {
public:
	SystemOnlineTopic() :
			Prop("system/online", (Flags )
					{ T_BOOL, M_READ, T_10SEC, QOS_1, NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.add(true);
	}
};
SystemOnlineTopic systemOnline;
RealTimeTopic now;
UptimeTopic uptime;

HardwareTopic hardware;
TempTopic tt;

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/lm4f120h5qr.h"
#include "inc/hw_gpio.h"

#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"

class GpioOutTopic: public Prop {
	uint32_t _gpio_port;
	uint8_t _gpio_pin;
	bool _value;
public:
	GpioOutTopic(const char *name, uint32_t base, uint8_t pin) :
			Prop(name, (Flags )
					{ T_BOOL, M_RW, T_10SEC, QOS_2, NO_RETAIN }) {
		_gpio_port = base;
		_gpio_pin = pin;
		_value = false;
//		GPIOPinTypeGPIOOutput(_gpio_port, _gpio_pin); // Enable the GPIO pins
//		GPIOPinWrite(_gpio_port, _gpio_pin, 0); // DON'T INITIALIZE PIN in STATIC , PORT IS NOT INIT YET
	}

	void fromBytes(Bytes& message) {
		Json json(message);
		bool bl;
		if (json.get(bl)) {
			_value = bl;
			if (bl)
				GPIOPinWrite(_gpio_port, _gpio_pin, _gpio_pin);
			else
				GPIOPinWrite(_gpio_port, _gpio_pin, 0);
		}
	}
	void toBytes(Bytes& message) {
		Json json(message);
		json.add(_value);
	}
};

GpioOutTopic gpioF2("GPIO/F2", GPIO_PORTF_BASE, GPIO_PIN_2);
GpioOutTopic gpioF1("GPIO/F1", GPIO_PORTF_BASE, GPIO_PIN_1);
GpioOutTopic gpioF3("GPIO/F3", GPIO_PORTF_BASE, GPIO_PIN_3);

// #include "Fsm.h"

class LedBlink: public Handler {
	bool _isOn;
	uint32_t _msecInterval;
	Mqtt* _mqtt;
public:
	LedBlink(Mqtt* mqtt) :
			Handler("LedBlink") {
		_isOn = false;
		_msecInterval = 500;
		_mqtt = mqtt;
	}

	virtual ~LedBlink() {
	}

	bool dispatch(Msg& msg) {
		PT_BEGIN()
		while (true) {
			timeout(_msecInterval);
			PT_YIELD_UNTIL(
					msg.is(_mqtt, SIG_CONNECTED)
							|| msg.is(_mqtt, SIG_DISCONNECTED) || timeout());
			switch (msg.signal) {
			case SIG_TICK: {
				Board::setLedOn(Board::LED_GREEN, _isOn);
				_isOn = !_isOn;
				break;
			}
			case SIG_CONNECTED: {
				_msecInterval = 100;
				break;
			}
			case SIG_DISCONNECTED: {
				_msecInterval = 500;
				break;
			}
			default: {
			}
			}

		}
	PT_END()
	;
}

};

#include "Persistent.h"

class PropMqttPrefix: public Prop {
public:
	Str mqttPrefix;
	PropMqttPrefix(const char* name) :
			 Prop(name, (Flags )
					{ T_STR, M_RW, T_10SEC, QOS_2, RETAIN }),mqttPrefix(20) {
	}

	void toBytes(Bytes& message) {
		Json json(message);
		json.get(mqttPrefix);
		json.add(mqttPrefix);
	}

	void fromBytes(Bytes& message) {
		Json json(message);
		if (json.get(mqttPrefix)) {
			Persistent pers;
			pers.put(PERS_MQTT_PREFIX, mqttPrefix.data(), mqttPrefix.length());
		}
	}
	Str& get() {
		uint8_t length = mqttPrefix.capacity();
		Persistent pers;
		mqttPrefix.clear();
		if (!pers.get(PERS_MQTT_PREFIX, mqttPrefix.data(), length))
			mqttPrefix << "Stellaris-1/";
		else
			mqttPrefix.length(length);
		return mqttPrefix;
	}
};

PropMqttPrefix mqttPrefix("mqtt/prefix");

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
#include "BipBuffer.h"
#include "Msg.h"
#include "Uart.h"
#include "Cbor.h"

extern Uart *gUart0;

PropMgr propMgr;

int main(void) {
	Board::init();	// initialize usb
	Uart::init();

	Mqtt mqtt(*gUart0);	// mqtt active object
	LedBlink ledBlink(&mqtt); // led blinks when mqtt is connected

	propMgr.setMqtt(&mqtt);
	/*	char prefix[100];

	 #define PREFIX "pcacer_1/"
	 Persistent clear;
	 // clear.erasePage((uint8_t*)0x30000);
	 // clear.put(PERS_MQTT_PREFIX,(uint8_t*)PREFIX,sizeof(PREFIX));

	 Persistent flash;
	 uint8_t realLength = sizeof(prefix);
	 if (flash.get(PERS_MQTT_PREFIX, (uint8_t*) prefix, realLength))
	 strcpy(prefix, "Stellaris_1/");*/

	propMgr.setPrefix(mqttPrefix.get().c_str()); // should be after setMqtt link, otherwise prefix doesn't get propagated

	uint64_t clock = Sys::upTime() + 100;
	MsgQueue::publish(0, SIG_INIT, 0, 0);				// kickoff all engines
	Msg msg;

	while (1) {
		if (Sys::upTime() > clock) {
			clock += 10;		// 10 msec timer tick
			MsgQueue::publish(0, SIG_TICK, 0, 0); // check timeouts every 10 msec
		}
		if (gUart0->hasData())	// if UART has received data alert uart receiver
			MsgQueue::publish(gUart0, SIG_START);
		// _________________________________________________________________handle all queued messages
		while (MsgQueue::get(msg)) {
			Handler::dispatchToChilds(msg);

		}

	}
}

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

#include <malloc.h>
#include "Handler.h"

class TempTopic: public Prop {
public:
	TempTopic() :
			Prop("system/temp", (Flags ) { T_FLOAT, M_READ, T_1SEC, QOS_0,
							NO_RETAIN }) {
	}
	void toBytes(Bytes& bytes) {
		Cbor msg(bytes);
		msg.add(Board::getTemp());
	}
};

TempTopic tt;

class HardwareTopic: public Prop {
public:
	HardwareTopic() :
			Prop("system/hardware", (Flags ) { T_OBJECT, M_READ, T_1KSEC, QOS_1,
							NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Cbor msg(message);
		msg.addMap(4);
		msg.add("cpuRevision");
		Bytes b(8);
		Board::processorRevision(b);
		msg.add(b);
		msg.add("cpu");
		msg.add("LM4F120H5QR");
		msg.add("board");
		msg.add("EK-LM4F120XL");
		msg.add("bool");
		msg.add(false);
	}
};

HardwareTopic hardware;

class UptimeTopic: public Prop {
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags ) { T_UINT64, M_READ, T_1SEC, QOS_0,
							NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Cbor msg(message);
		msg.add(Sys::upTime());
	}
};

UptimeTopic uptime;

class SystemOnlineTopic : public Prop {
public:
	SystemOnlineTopic() :
			Prop("system/online", (Flags ) { T_BOOL, M_READ, T_10SEC, QOS_1,
							NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Cbor msg(message);
		msg.add(true);
	}
};
SystemOnlineTopic systemOnline;


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
			Prop(name, (Flags ) { T_BOOL, M_RW, T_10SEC, QOS_2, NO_RETAIN }) {
		_gpio_port = base;
		_gpio_pin = pin;
		_value = false;
//		GPIOPinTypeGPIOOutput(_gpio_port, _gpio_pin); // Enable the GPIO pins
//		GPIOPinWrite(_gpio_port, _gpio_pin, 0); // DON'T INITIALIZE PIN in STATIC , PORT IS NOT INIT YET
	}

	void fromBytes(Bytes& message) {
		Cbor msg(message);
		bool bl;
		if (msg.get(bl)) {
			_value = bl;
			if (bl)
				GPIOPinWrite(_gpio_port, _gpio_pin, _gpio_pin);
			else
				GPIOPinWrite(_gpio_port, _gpio_pin, 0);
		}
	}
	void toBytes(Bytes& message) {
		Cbor msg(message);
		if (GPIOPinRead(_gpio_port, _gpio_pin))
			msg.add((bool) true);
		else
			msg.add((bool) false);
	}
};

GpioOutTopic gpio("GPIO/F2", GPIO_PORTF_BASE, GPIO_PIN_2);

#include "Fsm.h"

class LedBlink: public Handler {
	bool _isOn;
	uint32_t _msecInterval;
public:
	LedBlink() {
		_isOn = false;
		_msecInterval = 500;
	}

	virtual ~LedBlink() {
	}

	void onEntry(){
		timeout(_msecInterval);
	}

	void onTimeout() {
		Board::setLedOn(Board::LED_GREEN, _isOn);
		_isOn = !_isOn;
		timeout(_msecInterval);
	}

	void onOther(Msg& event) {
		switch (event.sig()) {
		case SIG_MQTT_CONNECTED: {
			_msecInterval = 100;
			break;
		}
		case SIG_MQTT_DISCONNECTED: {
			_msecInterval = 500;
			break;
		}
		default: {
			break;
		}
		}
	}
};
LedBlink ledBlink;
Msg msg;
Handler* hdlr;
static Msg timeout(SIG_TIMEOUT);

void eventPump() {
	while (true) {
		msg.open();	// get message from queue
		if (msg.sig() == SIG_IDLE) // ignore idles, no interest in handlers
			break;
		for (hdlr = Handler::first(); hdlr != 0; hdlr = hdlr->next()) { // dispatch message event to all handlers
			hdlr->dispatch(msg);
			msg.rewind();
		}
		msg.recv();	// confirm reception, remove from queue
	}
}

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

int main(void) {
	Board::init();	// initialize usb
	Uart::init();
//	Usb usb;	// usb active object
//	Uart uart0;
	Mqtt mqtt(*gUart0);	// mqtt active object

	PropMgr propertyListener(mqtt);
	uint64_t clock = Sys::upTime() + 100;
	Msg::publish(SIG_INIT);
	Msg::publish(SIG_ENTRY);
	while (1) {
		eventPump();
		if (Sys::upTime() > clock) {
			clock += 10;		// 10 msec timer tick
			Msg::publish(SIG_TIMER_TICK);
		}
		if (gUart0->hasData())
			Msg::publish(SIG_LINK_RXD);
	}
}

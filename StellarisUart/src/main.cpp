/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Usb.h"
#include "Board.h"
#include "Mqtt.h"
#include "Timer.h"
#include "Prop.h"
#include "Cbor.h"

#include <malloc.h>
/*


 class LedTopic {

 LedTopic(const char* name,int led) {
 Str s(20);
 s << name;
 s << "/on";
 Prop p(s, (void* )led, LedInterval, (Flags ) { T_BOOL, M_WRITE, QOS_1, I_OBJECT, false,
 true, true }) ;
 }
 static bool LedInterval(Cmd cmd, void *instance, Cbor msg) {
 int ledNr;
 if (cmd == CMD_GET) {
 msg.add(Board::getLed(ledNr));
 } else if (cmd == CMD_PUT) {
 bool on;
 PackType pt;
 Variant v;
 msg.readToken(pt,v);
 if (msg.getBool(on))
 Board::setLedOn(ledNr, on);
 } else if (cmd == CMD_DESC) {
 msg.add((int) (Flags ) { T_BOOL, M_WRITE, QOS_1, I_OBJECT, false,
 true, true });
 }
 return true;
 }
 };

 LedTopic ledTopic("ledBlue",Board::LED_BLUE);
 */
//typedef void (*Xdr)(void*, Cmd, Bytes&);
void ftoa(float n, char *res, int afterpoint);

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

//TempTopic tt;

class HardwareTopic: public Prop {
public:
	HardwareTopic() :
			Prop("system/hardware", (Flags ) { T_OBJECT, M_READ, T_10SEC, QOS_1,
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

//HardwareTopic hardware;

class UptimeTopic: public Prop {
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags ) { T_UINT64, M_READ, T_10SEC, QOS_0,
							NO_RETAIN }) {
	}

	void toBytes(Bytes& message) {
		Cbor msg(message);
		msg.add(Sys::upTime());
	}
};

//UptimeTopic uptime;

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
//#include "inc/lm4f112h5qc.h"
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
			Prop(name,
					(Flags ) { T_BOOL, M_RW, T_10SEC, QOS_2,
									NO_RETAIN }) {
		_gpio_port = base;
		_gpio_pin = pin;
		_value = false;
//		GPIOPinTypeGPIOOutput(_gpio_port, _gpio_pin); // Enable the GPIO pins
//		GPIOPinWrite(_gpio_port, _gpio_pin, 0); // DON'T INITIALIZE PIN in STATIC , PORT IS NOT INIT YET
	}

	void fromBytes(Bytes& message) {
		Cbor msg(message);
		bool bl;
		if (msg.getBool(bl)) {
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
			msg.add((bool)true);
		else
			msg.add((bool)false);
	}
};

GpioOutTopic gpio("GPIO/F2", GPIO_PORTF_BASE, GPIO_PIN_2);

#include "Fsm.h"

class LedBlink: public Fsm {
	bool _isOn;
	uint32_t _msecInterval;
public:
	LedBlink() {
		init(static_cast<SF>(&LedBlink::blink));
		timeout(1000);
		_isOn = false;
		_msecInterval = 500;
	}

	virtual ~LedBlink() {
	}

	void blink(Msg& event) {
		switch (event.sig()) {
		case SIG_TIMEOUT: {
			Board::setLedOn(Board::LED_GREEN, _isOn);
			_isOn = !_isOn;
			timeout(_msecInterval);
//			Prop::publishAll();
			break;
		}
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
Fsm* fsm;
static Msg timeout(SIG_TIMEOUT);

void eventPump() {
	while (true) {
		msg.open();	// get message from queue
		if (msg.sig() == SIG_IDLE)
			break;
		for (fsm = Fsm::first(); fsm != 0; fsm = Fsm::next(fsm)) {
			if ((msg.sig() == SIG_TIMER_TICK)) {
				if (fsm->timeout())
					fsm->dispatch(timeout);
			} else
				fsm->dispatch(msg);
		}
//		Fsm::dispatchToAll(msg);
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

extern void uartTest();

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
	Msg msg;
	while (1) {
		eventPump();
		if (Sys::upTime() > clock) {
			clock += 10;
			Msg::publish(SIG_TIMER_TICK);
		}
		if ( gUart0->hasData())
			Msg::publish(SIG_LINK_RXD);
	}
}

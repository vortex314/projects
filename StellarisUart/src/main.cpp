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

//typedef void (*Xdr)(void*, Cmd, Bytes&);

void ftoa(float n, char *res, int afterpoint);

void getTemp(void* addr, Cmd cmd, Bytes& message) {

	if (cmd == ( CMD_GET)) {
		Cbor msg(message);
		msg.add(Board::getTemp());
	}
}

void getHardware(void* addr, Cmd cmd, Bytes& message) {

	if (cmd == (CMD_GET)) {
		Cbor msg(message);
		msg.addMap(3);
		msg.add("cpuRevision");
		msg.add(Board::processorRevision());
		msg.add("cpu");
		msg.add("LM4F120H5QR");
		msg.add("board");
		msg.add("EK-LM4F120XL");
	}
}

uint64_t cpuRev = Board::processorRevision();

Prop uptime("system/uptime", Sys::_upTime);
Prop cpu("hardware/cpu", "LM4F120H5QR");
Prop board("hardware/board", "EK-LM4F120XL");
Prop revision("hardware/cpuRevision", cpuRev);

Prop temp("system/temperature", (void*) 0, getTemp, (Flags ) { T_FLOAT, M_READ,
				QOS_0, I_OBJECT, false, true, true });
Prop rev("system/hardware", (void*) 0, getHardware, (Flags ) { T_OBJECT, M_READ,
				QOS_0, I_OBJECT, false, true, true });

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
			Prop::publishAll();
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
	}
}

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

#include <malloc.h>

typedef void (*Xdr)(void*, Cmd, Bytes&);

void ftoa(float n, char *res, int afterpoint);

void getTemp(void* addr, Cmd cmd, Bytes& buffer) {
	Msgpack msg(buffer);
	if (cmd == CMD_GET)
		msg.pack(Board::getTemp());
}

void getRev(void* addr, Cmd cmd, Bytes& buffer) {
	Msgpack msg(buffer);
	if (cmd == CMD_GET) {
			msg.pack(Board::processorRevision());
	}
}

Prop cpu("system/CPU", "lm4f120h5qr");
Prop board("system/board", "Stellaris LaunchPad");
Prop uptime("system/uptime", Sys::_upTime);
Prop temp("system/temperature", (void*) 0, getTemp, (Flags ) { T_FLOAT, M_READ,
				QOS_0, I_OBJECT, false, true, true });
Prop rev("system/cpu.revision", (void*) 0, getRev, (Flags ) { T_BYTES, M_READ,
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
			temp.updated();
			uptime.updated();
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

int main(void) {

	Str s(10);
	s << "eee";

	Board::init();	// initialize usb
	Usb::init();
	Usb usb;	// usb active object
	Mqtt mqtt(usb);	// mqtt active object

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

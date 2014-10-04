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

//typedef void (*Xdr)(void*, Cmd, Bytes&);

void ftoa(float n, char *res, int afterpoint);

void getTemp(void* addr, Cmd cmd, Packer& msg) {
	if (cmd == CMD_GET)
		msg.pack(Board::getTemp());
}

void getHardware(void* addr, Cmd cmd, Packer& msg) {
	if (cmd == CMD_GET) {
		msg.packMapHeader(3);
		msg.pack("cpuRevision");
		msg.pack(Board::processorRevision());
		msg.pack("cpu");
		msg.pack("LM4F120H5QR");
		msg.pack("board");
		msg.pack("EK-LM4F120XL");
	}
}

Prop uptime("system/uptime", Sys::_upTime);
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

#include "Cbor.h"

class Listener: public CborListener {
public:
	void OnInteger(int value) {
	}

	void OnBytes(unsigned char *data, int size) {
	}

	void OnString(std::string &str) {
	}

	void OnArray(int size) {
	}

	void OnMap(int size) {
	}

	void OnTag(unsigned int tag) {
	}

	void OnSpecial(int code) {
	}

	void OnError(const char *error) {
	}

};

int main(void) {

	CborOutput output(100);
	CborWriter writer(output);

	writer.writeTag(123);
	writer.writeArray(3);
	writer.writeString("hello");
	writer.writeString("world");
	writer.writeInt(321);

	CborInput input(output.getData(), output.getSize());
	CborReader reader(input);
	Listener listener;

	reader.SetListener(listener);
	reader.Run();

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

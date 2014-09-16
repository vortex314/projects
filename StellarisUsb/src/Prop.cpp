/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Prop.h"

Prop* Prop::_first;

Prop::Prop(const char* name, void* instance, Xdr xdr, Flags flags) {
	_name = name;
	_instance = instance;
	_xdr = xdr;
	_flags = flags;
	_flags.publishValue = true;
	_flags.publishMeta = true;
	if (_first == 0)
		_first = this;
	else {
		Prop* cursor = _first;
		while (cursor->_next != 0) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

Prop*
Prop::findProp(Str& name) {
	Prop* cursor = _first;
	while (cursor != 0) {
		if (name.equals(cursor->_name))
			return cursor;
		cursor = cursor->_next;
	}
	return 0;
}

void Prop::set(Str& topic, Strpack& message, uint8_t header) {
	Str str(30);
//	str.substr(topic, getPrefix.length());
	Prop* p = findProp(str);
	if (p) {
		if (p->_xdr)
			p->_xdr(p->_instance, CMD_PUT, message);
		p->_flags.publishValue = true;
	}

	Fsm::publish(SIG_PROP_CHANGED);
}

PropMgr::PropMgr(Mqtt& mqtt) :
		_mqtt(mqtt), _topic(30), _message(100) {
	_cursor = Prop::_first;
	PT_INIT(&t);
	init(static_cast<SF>(&PropMgr::sleep));
}

void PropMgr::sleep(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_CONNECTED: {
		TRAN(PropMgr::publishing);
		break;
	}
	default: {
	}
	}
}

typedef void (*Xdr)(void*, Cmd, Strpack&);

void strXdr(void* addr, Cmd cmd, Strpack& strp) {
	if (cmd == CMD_GET)
		strp << (const char*) addr;
}

Prop cpu("system/cpu", (void*) "lm4f120h5qr", strXdr, (Flags ) { T_STR, M_READ,
				QOS_0, I_ADDRESS, false, true, true });
Prop board("system/board", (void*) "Stellaris LaunchPad", strXdr, (Flags ) {
				T_STR, M_READ, QOS_0, I_ADDRESS, false, true, true });

void PropMgr::nextProp() {
	_cursor = _cursor->_next;
	if (_cursor == 0)
		_cursor = Prop::_first;
}

void PropMgr::publishing(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_DO_PUBLISH: { //TODO
		break;
	}
	case SIG_ENTRY: {
		_cursor = Prop::_first;
		timeout(100);
		break;
	}
	case SIG_MQTT_PUBLISH_FAILED: {
		nextProp();
		timeout(100);
		break;
	}
	case SIG_MQTT_PUBLISH_OK: {
//		_cursor->_flags.publishValue = false;
		nextProp();
		timeout(100);
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout() && _cursor->_flags.publishValue) {
			_topic = _cursor->_name;
			_message.clear();
			_cursor->_xdr(_cursor->_instance, CMD_GET, _message);
			_mqtt.Publish(_cursor->_flags, 0xBEAF, _topic, _message);
			timeout(UINT32_MAX);
		} else {
			nextProp();
			timeout(1000);
		}
		break;
	}
	case SIG_EXIT: {
		timeout(UINT32_MAX);
		break;
	}
	case SIG_MQTT_DISCONNECTED: {
		TRAN(PropMgr::sleep);
		break;
	}
	default: {
	}
	}

}


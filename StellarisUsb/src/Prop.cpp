/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Prop.h"
#include "Fsm.h"

Prop* Prop::_first;

const char* sType[] = { "UINT8", "UINT16", "UINT32", "UINT64", "INT8", "INT16",
		"INT32", "INT64", "BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY", "MAP",
		"STR", "OBJECT" };

const char* sMode[] = { "READ", "WRITE" };

const char* sQos[] = { "0", "1", "2" };

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

extern Str putPrefix;
extern Str getPrefix;

void Prop::set(Str& topic, Strpack& message) {
	Str str(30);

	if (topic.startsWith(putPrefix)) {
		str.substr(topic, putPrefix.length());
		Prop* p = findProp(str);
		if (p) {
			if (p->_xdr)
				p->_xdr(p->_instance, CMD_PUT, message);
			p->_flags.publishValue = true;
		}
		Msg::publish(SIG_PROP_CHANGED);
	} else if (topic.startsWith(getPrefix)) {
		str.substr(topic, getPrefix.length());
		Prop* p = findProp(str);
		if (p) {
			p->_flags.publishValue = true;
			Msg::publish(SIG_PROP_CHANGED);
		}

	}

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

void uint64Xdr(void* addr, Cmd cmd, Strpack& strp) {
	if (cmd == CMD_GET)
		strp << *((uint64_t*) addr);
}

void ftoa(float n, char *res, int afterpoint);

void getTemp(void* addr, Cmd cmd, Strpack& strp) {
	char buffer[20];
	ftoa(Board::getTemp(), buffer, 2);
	if (cmd == CMD_GET)
		strp << buffer;
}

Prop cpu("system/cpu", (void*) "lm4f120h5qr", strXdr, (Flags ) { T_STR, M_READ,
				QOS_0, I_ADDRESS, false, true, true });
Prop board("system/board", (void*) "Stellaris LaunchPad", strXdr, (Flags ) {
				T_STR, M_READ, QOS_0, I_ADDRESS, false, true, true });
Prop uptime("system/uptime", (void*) &Sys::_upTime, uint64Xdr, (Flags ) {
				T_UINT64, M_READ, QOS_0, I_ADDRESS, false, true, true });
Prop temp("system/temperature", (void*) 0, getTemp, (Flags ) { T_FLOAT, M_READ,
				QOS_0, I_OBJECT, false, true, true });

void PropMgr::nextProp() {
	_cursor = _cursor->_next;
	if (_cursor == 0)
		_cursor = Prop::_first;
}

void PropMgr::publishing(Msg& event) {
	switch (event.sig()) {
	case SIG_PROP_CHANGED: { //TODO
		timeout(10);
		break;
	}
	case SIG_ENTRY: {
		Prop* cursor = _cursor = Prop::_first;
		while (cursor->_next != 0) {
			cursor->_flags.publishValue = true;
			cursor->_flags.publishMeta = true;
			cursor = cursor->_next;
		}

		timeout(10);
		break;
	}
	case SIG_MQTT_PUBLISH_FAILED: {
		nextProp();
		timeout(100);
		break;
	}
	case SIG_MQTT_PUBLISH_OK: {
		if (_publishMeta)
			_cursor->_flags.publishMeta = false;
		else
			_cursor->_flags.publishValue = false;
		nextProp();
		timeout(100);
		break;
	}
	case SIG_TIMEOUT: {
		if (_cursor->_flags.publishValue) {
			_publishMeta = false;
			_topic = _cursor->_name;
			_message.clear();
			_cursor->_xdr(_cursor->_instance, CMD_GET, _message);
			_mqtt.Publish(_cursor->_flags, 0xBEAF, _topic, _message);
			timeout(UINT32_MAX);
		} else if (_cursor->_flags.publishMeta) {
			_publishMeta = true;
			_topic = _cursor->_name;
			_topic << ".META";
			_message.clear();
			_message << "{ 'type' : '" << sType[_cursor->_flags.type] << "'";
			_message << ",'mode' : '" << sMode[_cursor->_flags.mode] << "'";
			_message << ",'qos' : " << sQos[_cursor->_flags.qos] << "";
			_message << "}";
			_mqtt.Publish(_cursor->_flags, 0xBEAF, _topic, _message);
			timeout(UINT32_MAX);

		} else {
			nextProp();
			timeout(1000);
			temp._flags.publishValue = true;
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


/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"
#include "Prop.h"
#include "Fsm.h"


Prop* Prop::_first;

const char* sType[] = { "UINT8", "UINT16", "UINT32", "UINT64", "INT8", "INT16",
		"INT32", "INT64", "BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY", "MAP",
		"STR", "OBJECT" };

const char* sMode[] = { "READ", "WRITE" };

const char* sQos[] = { "0", "1", "2" };

Prop::Prop(const char* name, void* instance, Xdr xdr, Flags flags) {
	init(name, instance, xdr, flags);
}

void Prop::init(const char* name, void* instance, Xdr xdr, Flags flags) {
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

Prop::Prop(const char* name, const char* value) {
	init(name, (void*) value, xdrString, (Flags ) { T_FLOAT, M_READ, QOS_0,
					I_ADDRESS, false, true, true });
}
Prop::Prop(const char* name, uint64_t& value) {
	init(name, (void*) &value, xdrUint64, (Flags ) { T_UINT64, M_READ, QOS_1,
					I_ADDRESS, false, true, true });
}

#include <cstdlib>

void Prop::xdrUint64(void* addr, Cmd cmd, Packer& msg) {
	if (cmd == CMD_GET)
		msg.pack(*((uint64_t*) addr));
	else if (cmd == CMD_PUT) {

		char value[20];
		int i = 0;
		while (msg.hasData())
			value[i++] = msg.read();
		value[i++] = '\0';
		char *ptr;
		*((uint64_t*) addr) = strtoull(value, &ptr, 10);
	}
}

void Prop::xdrString(void* addr, Cmd cmd, Packer& msg) {
	if (cmd == CMD_GET)
		msg.pack( (const char*) addr);
	else if (cmd == CMD_PUT) {
		int i = 0;
		msg.offset(0);
		while (msg.hasData())
			((char*) addr)[i++] = msg.read();
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

void Prop::set(Str& topic, Packer& message) {
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
			Msgpack msg(_message);
			msg.packMapHeader(3);
			msg.pack("type");msg.pack(sType[_cursor->_flags.type]);
			msg.pack("mode");msg.pack(sMode[_cursor->_flags.mode]);
			msg.pack("qos"),msg.pack(sQos[_cursor->_flags.qos]);
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

void Prop::updated() {
	_flags.publishValue = true;
}


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

void Prop::xdrUint64(void* addr, Cmd cmd, Bytes& message) {
	Cbor msg(message);
	if (cmd == (CMD_GET)) {
		msg.add(*((uint64_t*) addr));
	} else if (cmd == CMD_PUT) {
		Variant v;
		PackType t;
		msg.readToken(t, v);
		if (t == P_PINT)
			*((uint64_t*) addr) = v._uint64;
	}
}

void Prop::xdrString(void* addr, Cmd cmd, Bytes& message) {
	Cbor msg(message);
	if (cmd == (CMD_GET)){
		msg.add((const char*) addr);
			}
	else if (cmd == CMD_PUT) {
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

void Prop::set(Str& topic, Bytes& message) {
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
		timeout(10);
		break;
	}
	case SIG_MQTT_PUBLISH_FAILED: {
		nextProp();
		timeout(1000);
		break;
	}
	case SIG_MQTT_PUBLISH_OK: {
		if (_publishMeta)
			_cursor->_flags.publishMeta = false;
		else
			_cursor->_flags.publishValue = false;
		nextProp();
		timeout(1);
		break;
	}
	case SIG_TIMEOUT: {
		if (_cursor->_flags.publishValue) {
			_publishMeta = false;
			_topic = _cursor->_name;
			_message.clear();
			_cursor->_xdr(_cursor->_instance, CMD_GET, _message);
			_mqtt.Publish(_cursor->_flags, Mqtt::nextMessageId(), _topic, _message);
			timeout(UINT32_MAX);
		} else if (_cursor->_flags.publishMeta) {
			_publishMeta = true;
			_topic = _cursor->_name;
			_topic << ".META";
			_message.clear();
			Cbor msg(_message);
			msg.addMap(-1);
			msg.add("type").add(sType[_cursor->_flags.type]);
			msg.add("mode").add(sMode[_cursor->_flags.mode]);
			msg.add("qos").add(sQos[_cursor->_flags.qos]);
			msg.addBreak();
			_mqtt.Publish(_cursor->_flags, Mqtt::nextMessageId(), _topic, _message);
			timeout(UINT32_MAX);
		} else {
			nextProp();
			timeout(1);
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

void Prop::publishAll() {
	Prop* cursor = Prop::_first;
	while (cursor->_next != 0) {
		cursor->_flags.publishValue = true;
		cursor->_flags.publishMeta = true;
		cursor = cursor->_next;
	}
}

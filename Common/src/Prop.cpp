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


Prop::Prop(const char* name,  Flags flags) {
	init(name,  flags);
}

void Prop::init(const char* name, Flags flags) {
	_name = name;
	_flags = flags;
	_flags.doPublish=true;
	_lastPublished=0;
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
static int pow10[10] = {
        1, 10, 100, 1000, 10000,
        100000, 1000000, 10000000, 100000000, 1000000000
    };
bool Prop::hasToBePublished() {
	if ( _flags.mode == M_WRITE) return false;
	uint32_t x= pow10[_flags.interval];
	if ( Sys::upTime() > (_lastPublished + pow10[_flags.interval])) return true;
	return _flags.doPublish;
}

void Prop::doPublish(){
	_flags.doPublish=true;
}

void Prop::isPublished(){
	_flags.doPublish=false;
	_lastPublished = Sys::upTime();
}

void Prop::metaToBytes(Bytes& message){
	Cbor msg(message);
	msg.addMap(-1);
	msg.add("type");
	msg.add(sType[_flags.type]);
	msg.add("mode");
	msg.add(sMode[_flags.mode]);
	msg.add("qos");
	msg.add(_flags.qos);
	msg.add("interval");
	msg.add(1000/(10^_flags.interval));
	msg.addBreak();
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
	if (cmd == (CMD_GET)) {
		msg.add((const char*) addr);
	} else if (cmd == CMD_PUT) {
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
extern Str  headPrefix;

void Prop::set(Str& topic, Bytes& message) {
	Str str(30);

	if (topic.startsWith(putPrefix)) { // "PUT/<device>/<topic>
		str.substr(topic, putPrefix.length());
		Prop* p = findProp(str);
		if (p) {
				message.offset(0);
				p->fromBytes(message);
			p->doPublish();
		}
		Msg::publish(SIG_PROP_CHANGED);
	} else if (topic.startsWith(getPrefix)) { // "GET/<device>/<topic>
		str.substr(topic, getPrefix.length());
		Prop* p = findProp(str);
		if (p) {
			p->doPublish();
			Msg::publish(SIG_PROP_CHANGED);
		}

	} else if (topic.startsWith(headPrefix)) { // "HEAD/<device>/<topic>

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
		_cursor->isPublished();
		nextProp();
		timeout(1);
		break;
	}
	case SIG_TIMEOUT: {
		if (_cursor->hasToBePublished()) {
			_topic = _cursor->_name;
			_message.clear();
			_cursor->toBytes( _message);
			_mqtt.Publish(_cursor->_flags, Mqtt::nextMessageId(), _topic,
					_message);
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

void Prop::publishAll() {
	Prop* cursor = Prop::_first;
	while (cursor->_next != 0) {
		cursor->doPublish();
		cursor = cursor->_next;
	}
}

/*
 * Handler.cpp
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#include "Handler.h"
#include "Sys.h"

uint64_t _timeout;
Handler::Handler() {
	_timeout = UINT64_MAX;
	_next=0;
	reg();
}

void Handler::timeout(uint32_t msec) {
	_timeout = Sys::upTime() + msec;
}

bool Handler::timeout() {
	return _timeout < Sys::upTime();
}

void Handler::dispatch(Msg& msg) {
	Signal signal = msg.sig();
	if (signal == SIG_TIMER_TICK) {
		if (timeout())
			onTimeout();
	} else if (signal == SIG_MQTT_MESSAGE) {
		Bytes bytes(0);
		msg.get(bytes);
		MqttIn mqttIn(&bytes);
		bytes.offset(0);
		if (mqttIn.parse())
			onMqttMessage(mqttIn);
		else
			Sys::warn(EINVAL, "MQTT");
	} else if ( signal == SIG_INIT ) {
		onEntry();
	} else
		onOther(msg);
}

Handler* Handler::_first = 0;

Handler* Handler::first() {
	return _first;
}

Handler* Handler::next() {
	return _next;
}

void Handler::reg() {
	if (_first == 0)
		_first = this;
	else {
		Handler* cursor = _first;
		while (cursor->_next != 0) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

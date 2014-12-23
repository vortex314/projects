#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "BipBuffer.h"
#include "Msg.h"
#include "Event.h"

typedef struct {
	uint16_t length :16;
	Signal signal :8;
} Envelope;

BipBuffer Msg::bb;

#define ENVELOPE_SIZE	sizeof(Envelope)

Msg::Msg() {
	_bufferStart = 0;
	_signal = SIG_IDLE;
}

Msg::Msg(int size) {
	create(size);
	_signal = SIG_IDLE;
}

Msg::Msg(Signal sig) {
	_bufferStart = 0;
	_signal = sig;
	map(0, 0);
//	_bufferStart = (uint)&env.signal;
}

Msg& Msg::create(int size) {
	if (!bb.IsInitialized())
		bb.AllocateBuffer(1024);
	int reserved;
	_bufferStart = bb.Reserve(size + ENVELOPE_SIZE, reserved);
	if (_bufferStart)
		map(_bufferStart + ENVELOPE_SIZE, reserved - ENVELOPE_SIZE);
	else
		Sys::warn(EINVAL, "MSG");
	return *this;
}

Msg& Msg::open() {
	Envelope env;
	map(0, 0);

	// read length
	int size = 2;
	_bufferStart = bb.GetContiguousBlock(size);

	if (size >= 3) { 		// map to these bytes
		memcpy(&env, _bufferStart, ENVELOPE_SIZE);
		_signal = env.signal;
		map(_bufferStart + ENVELOPE_SIZE, env.length - ENVELOPE_SIZE);
	} else {
		_signal = SIG_IDLE;
	}
	// read signal
	return *this;
}

void Msg::recv() {
	bb.DecommitBlock(length() + ENVELOPE_SIZE);
}

void Msg::send() {
	Envelope env = { length() + ENVELOPE_SIZE, _signal };
	::memcpy(_bufferStart, &env, ENVELOPE_SIZE);
	bb.Commit(length() + ENVELOPE_SIZE);
}

Msg& Msg::rewind() {
	offset(0);
	return *this;
}

bool Msg::isEmpty() {
	return _signal == SIG_IDLE;
}

void Msg::publish(Signal sig) {
	Msg msg(0);
	msg.sig(sig);
	msg.send();
}

void Msg::publish(Signal sig, uint16_t detail) {
	Msg msg(2);
	msg.sig(sig);
	Cbor cbor(msg);
	cbor.add((uint64_t) detail);
	msg.send();
}

void Msg::get(Bytes& bytes) {
	Cbor cbor(*this);
	cbor.get(bytes);
}

Msg& Msg::add(Bytes& bytes) {
	Cbor cbor(*this);
	cbor.add(bytes);
	return *this;
}

Msg& Msg::sig(Signal sig) {
	_signal = sig;
	return *this;
}

Signal Msg::sig() {
	return _signal;
}


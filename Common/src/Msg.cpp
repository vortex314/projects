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

Msg::Msg() :
		Bytes(0) {
	_start = 0;
	_signal = SIG_IDLE;
}

#define ENVELOPE_SIZE	sizeof(Envelope)

Msg::Msg(Signal sig) :
		Bytes(1) {
	_start = 0;
	_signal = sig;
	map(0, 0);
//	_start = (uint)&env.signal;
}

Msg& Msg::create(int size) {
	if (!bb.IsInitialized())
		bb.AllocateBuffer(1024);
	int reserved;
	_start = bb.Reserve(size + ENVELOPE_SIZE, reserved);
	map(_start + ENVELOPE_SIZE, reserved - ENVELOPE_SIZE);
	return *this;
}

Msg& Msg::open() {
	Envelope env;
	map(0, 0);

	// read length
	int size = 2;
	_start = bb.GetContiguousBlock(size);

	if (size >= 3) { 		// map to these bytes
		memcpy(&env, _start, ENVELOPE_SIZE);
		_signal = env.signal;
		map(_start + ENVELOPE_SIZE, env.length - ENVELOPE_SIZE);
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
	::memcpy(_start, &env, ENVELOPE_SIZE);
	bb.Commit(length() + +ENVELOPE_SIZE);
}

Msg& Msg::add(uint8_t v) {
	uint8_t* pb = (uint8_t*) &v;
	write(pb, 0, sizeof(v));
	return *this;
}

Msg& Msg::add(uint16_t v) {
	uint8_t* pb = (uint8_t*) &v;
	write(pb, 0, sizeof(v));
	return *this;
}

Msg& Msg::add(int v) {
	uint8_t* pb = (uint8_t*) &v;
	write(pb, 0, sizeof(v));
	return *this;
}

Msg& Msg::add(Bytes& v) {
	uint16_t length = v.length();
	uint8_t* pb = (uint8_t*) &length;
	write(pb[0]);
	write(pb[1]);
	v.offset(0);
	while (v.hasData())
		write(v.read());
	return *this;
}

Msg& Msg::get(Bytes& v) {
	uint16_t length = v.length();
	uint8_t* pb = (uint8_t*) &length;
	pb[0] = read();
	pb[1] = read();
	v.offset(0);
	for (int i = 0; i < length; i++)
		v.write(read());
	return *this;
}

Msg& Msg::get(uint8_t& v) {
	uint8_t* pb = (uint8_t*) &v;
	for (uint32_t i = 0; i < sizeof(v); i++)
		*pb++ = read();
	return *this;
}

Msg& Msg::get(uint16_t& v) {
	uint8_t* pb = (uint8_t*) &v;
	for (uint32_t i = 0; i < sizeof(v); i++)
		*pb++ = read();
	return *this;
}

Msg& Msg::get(int& v) {
	uint8_t* pb = (uint8_t*) &v;
	for (uint32_t i = 0; i < sizeof(v); i++)
		*pb++ = read();
	return *this;
}

Msg& Msg::rewind() {
	offset(0);
	return *this;
}

bool Msg::isEmpty() {
	return _signal == SIG_IDLE;
}

void Msg::publish(Signal sig) {
	Msg msg;
	msg.create(0).sig(sig).send();
}

void Msg::publish(Signal sig, uint16_t detail) {
	Msg msg;
	msg.create(2).sig(sig).add(detail).send();
}

Msg& Msg::sig(Signal sig) {
	_signal = sig;
	return *this;
}

Signal Msg::sig() {
	return _signal;
}

BipBuffer Msg::bb;

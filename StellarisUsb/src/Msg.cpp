#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "BipBuffer.h"
#include "Msg.h"

typedef union {
	struct {
		uint16_t length;
		uint8_t id;
	};
	uint8_t b[3];
} Envelope;

Msg::Msg() :
		Bytes(0) {
	_start = 0;

}

#define ENVELOPE_SIZE	2

Msg::Msg(Signal sig) :
		Bytes(1) {
	Envelope env;
	env.id = sig;
	env.length = 1;
	map(&env.id, 1);
	_start = &env.id;
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
	map(0, 0);
	uint16_t l;
	// read length
	int size = 2;
	_start = bb.GetContiguousBlock(size);

	if (size > 2) { 		// map to these bytes
		memcpy(&l, _start, 2);
		map(_start + 2, size - 2);
	}
	// read signal
	return *this;
}

void Msg::recv() {
	bb.DecommitBlock(length() + 2);
}

void Msg::send() {
	uint16_t l = length();
	::memcpy(_start, &l, 2);
	bb.Commit(length() + 2);
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
	return length() == 0;
}

void Msg::publish(Signal sig){
	Msg msg;
	msg.create(1).add((uint8_t)sig).send();
}

void Msg::publish(Signal sig,uint16_t detail){
	Msg msg;
	msg.create(1).add((uint8_t)sig).add(detail).send();
}

BipBuffer Msg::bb;

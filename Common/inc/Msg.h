/*
 * Msg.h
 *
 *  Created on: 12-sep.-2014
 *      Author: lieven2
 */

#ifndef SIG_H_
#define SIG_H_

#include "Bytes.h"
#include "BipBuffer.h"
//#include "Event.h"
#include "Cbor.h"

typedef enum Signal
{
	SIG_INIT = 1 << 0,
	SIG_IDLE = 1 << 1,
	SIG_ERC = 1 << 2,
	SIG_TIMEOUT = 1 << 3,
	SIG_TICK = 1 << 4,
	SIG_CONNECTED = 1 << 5,
	SIG_DISCONNECTED = 1 << 6,
	SIG_RXD = 1 << 7,
	SIG_TXD = 1 << 8,
	SIG_START = 1 << 9,
	SIG_STOP = 1 << 10,
	SIG_SUCCESS = 1 << 11,
	SIG_FAIL = 1 << 12

} Signal;

class Msg: public Bytes
{
private:
	static BipBuffer bb;
	Signal _signal;
	void * _src;
	uint8_t* _bufferStart;
public:
	Msg();
	Msg(int size);
	Msg(Signal sig);

	Msg& create(int size);

	Msg& open();
	void recv();
	void send();

	void* src();
	Msg& src(void* ptr);

	Signal sig();
	Msg& sig(Signal sig);

	void get(Bytes& bytes);
	void get(uint32_t& i);

	Msg& add(Bytes& bytes);
	Msg& rewind();
//	Msg& send(void* str);
	Msg& recv(void* str);
	bool isEmpty();
	bool is(Signal signal);
	bool is(Signal, void* src);
	static void publish(Signal sig);
	static void publish(Signal signal, uint16_t detail);
	static void publish(Signal signal, void* src);

	static void push(uint8_t* pb, uint32_t length);

};

#endif /* SIG_H_ */

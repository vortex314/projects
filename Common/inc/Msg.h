/*
 * Msg.h
 *
 *  Created on: 12-sep.-2014
 *      Author: lieven2
 */

#ifndef MSG_H_
#define MSG_H_

#include "Bytes.h"
#include "BipBuffer.h"
#include "Event.h"
#include "Cbor.h"

class Msg : public Bytes {
private:
	static BipBuffer bb;
	Signal _signal;
	uint8_t* _bufferStart;
public:
	Msg();
	Msg(int size);
	Msg(Signal sig);
	Msg& create(int size);
	Msg& open();
	void recv();
	void send();
	Signal sig();
	void get(Bytes& bytes);
	void get(uint32_t& i);
	Msg& sig(Signal sig);
	Msg& add(Bytes& bytes);
	Msg& rewind();
	Msg& send(void* str);
	Msg& recv(void* str);
	bool isEmpty();
	static void publish(Signal sig);
	static void publish(Signal signal,uint16_t detail);

	static void push(uint8_t* pb,uint32_t length);

};

#endif /* MSG_H_ */

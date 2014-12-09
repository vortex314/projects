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

class Msg: public Bytes {
private:
	static BipBuffer bb;
	Signal _signal:8;
	uint8_t* _start;
public:
	Msg();
	Msg(Signal sig);
	Msg& create(int size);
	Msg& open();
	void recv();
	void send();
	Signal sig();
	Msg& sig(Signal sig);
	Msg& add(uint8_t v);
	Msg& add(uint16_t v);
	Msg& get(uint16_t& v);
	Msg& add(Bytes& bytes);
	Msg& get(Bytes& bytes);
	Msg& add(int v);
	Msg& get(int& v);
	Msg& get(uint8_t& v);
	Msg& rewind();
	Msg& send(void* str);
	Msg& recv(void* str);
	bool isEmpty();
	static void publish(Signal sig);
	static void publish(Signal signal,uint16_t detail);
	static void push(uint8_t* pb,uint32_t length);

};

#endif /* MSG_H_ */

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

typedef enum Signal {
	SIG_INIT = 1 << 0,
	SIG_IDLE = 1 << 1,
	SIG_ERC = 1 << 2,
//	SIG_TIMEOUT = 1 << 3,
	SIG_TICK = 1 << 4,
	SIG_CONNECTED = 1 << 5,
	SIG_DISCONNECTED = 1 << 6,
	SIG_RXD = 1 << 7,
	SIG_TXD = 1 << 8,
	SIG_START = 1 << 9,
	SIG_STOP = 1 << 10,
	SIG_DO = 1 << 13

} Signal;

class Handler;
class Msg {
public:
	Handler* src;
	Signal signal;
	int param;
	void* data;
//	Msg();
	bool is(Handler * src, int sigMask, int param, void* data);
	bool is(Handler * src, int sigMask);
	bool is(Handler * src, Signal signal);
};

class MsgQueue {
private:
	static BipBuffer bb;
public:

public:
	static void publish(Msg& msg);
	static void publish(Handler* src, Signal signal);
	static void publish(Handler* src, Signal signal, int param, void* data);
	static bool get(Msg& msg);
};

#endif /* SIG_H_ */

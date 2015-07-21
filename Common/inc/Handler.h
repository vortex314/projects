/*
 * Handler.h
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#ifndef HANDLER_H_
#define HANDLER_H_

#include "Msg.h"
#include "ProtoThread.h"

class Handler: public ProtoThread {
	static Handler* _firstChild;
	Handler* _next;
	uint64_t _timeout;
	const char* _name;
public:
	Handler();
	Handler(const char* name);

	virtual ~Handler() {
	}

	void timeout(uint32_t msec);
	bool timeout();
	uint64_t getTimeout();
	const char* getName();

	virtual void free(void* ptr) {
		::free(ptr);
	}

	static Handler* first();
	Handler* next();
	static void dispatchToChilds(Msg& msg);
	void reg(Handler* hdlr);
};

#endif /* HANDLER_H_ */

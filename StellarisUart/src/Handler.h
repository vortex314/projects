/*
 * Handler.h
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#ifndef HANDLER_H_
#define HANDLER_H_

#include "Msg.h"
#include "pt.h"

class Handler
{
	Handler* _firstChild;
	Handler* _next;
	uint64_t _timeout;
//	uint32_t _sigMask;
//	void* _srcMask;
	const char* _name;
protected:
	struct pt pt;
public:
	Handler();
	Handler(const char* name);

	virtual ~Handler()
	{
	}

	void timeout(uint32_t msec);
	bool timeout();
	uint64_t getTimeout();

	virtual int dispatch(Msg& msg)
	{
		return 0;
	}

	void restart();

	Handler* first();
	Handler* next();
	void dispatchToChilds(Msg& msg);
	void reg(Handler* hdlr);
};

#endif /* HANDLER_H_ */

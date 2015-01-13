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



class Handler {
	static Handler* _first;
	Handler* _next;
	uint64_t _timeout;
	uint32_t _sigMask;
	void* _srcMask;
	const char* _name;
protected:
	struct pt pt;
public:
	Handler();
	Handler(const char* name);

	virtual ~Handler() {
	}

	void timeout(uint32_t msec);
	bool timeout();
	uint64_t getTimeout();

	void listen(uint32_t signalMap);
	void listen(uint32_t signalMap, uint32_t time);
	void listen(void* src);
	void listen(uint32_t signalMap,void* src);
	bool accept(Signal signal);
	bool accept(Signal signal,void* src);

	virtual void dispatch(Msg& msg);
	virtual int ptRun(Msg& msg){ return 0;} ;
	void restart();

	static Handler* first();
	Handler* next();
	void reg();
};


#endif /* HANDLER_H_ */

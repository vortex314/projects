/*
 * Handler.h
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#ifndef HANDLER_H_
#define HANDLER_H_
#include "MqttIn.h"
#include "Msg.h"
#include "pt.h"

class Handler {
	static Handler* _first;
	Handler* _next;
	uint64_t _timeout;
	uint32_t _sigMask;
	const char* _name;
protected:
	struct pt pt;
public:
/*	virtual void onInit() {
	}

	virtual void onEntry() {
	}

	virtual void onExit() {
	}

	virtual void onTimeout() {
	}

	virtual void onMqttMessage(MqttIn& msg) {
	}

	virtual void onOther(Msg& msg) {
	}*/

	Handler();
	Handler(const char* name);

	virtual ~Handler() {
	}

	void timeout(uint32_t msec);
	bool timeout();
	uint64_t getTimeout();

	void listen(uint32_t signalMap);
	void listen(uint32_t signalMap, uint32_t time);
	bool accept(Signal signal);

	virtual void dispatch(Msg& msg);
	virtual int ptRun(Msg& msg){ return 0;} ;
	void restart();

	static Handler* first();
	Handler* next();
	void reg();
};

#endif /* HANDLER_H_ */

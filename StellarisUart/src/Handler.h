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


class Handler {
	Handler* _next;
	static Handler* _first;
public:
	virtual void onInit(){};
	virtual void onEntry(){};
	virtual void onExit(){};
	virtual void onTimeout(){};
	virtual void onMqttMessage(MqttIn& msg){};
	virtual void onOther(Msg& msg){};
	uint64_t _timeout;
	Handler();

	virtual ~Handler(){};

	void timeout(uint32_t msec);
	bool timeout() ;

	virtual void dispatch(Msg& msg) ;

	static Handler* first() ;
	Handler* next();
	void reg();
};

#endif /* HANDLER_H_ */

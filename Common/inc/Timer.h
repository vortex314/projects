/* 
 * File:   Timer.h
 * Author: lieven
 *
 * Created on September 7, 2013, 12:25 AM
 */

#ifndef TIMER_H
#define	TIMER_H
#include <stdint.h>
#include "Erc.h"
#include "Bits.h"
#include "Stream.h"

class Timer: public Stream {
public:
	enum BitEvents {
		ALARM = 1
	};
	enum TimerEvents {
		EXPIRED = EVENT_CHAR('T','i','m','X')
	};
	Timer(Stream* stream);
	~Timer();
	Timer(const Timer& ref);
	Timer(uint32_t value, bool reload);
	void interval(uint32_t interval);
	void start();
	void start(uint32_t interval);
	void stop();
	void reload(bool automatic);
	void dec();
	Erc event(Event* event);
	static void decAll();
	bool expired();
private:
	Bits _events;
	uint32_t _counterValue;
	uint32_t _reloadValue;
	bool _isAutoReload;
	bool _isActive;
	bool _isExpired;
};

#endif	/* TIMER_H */


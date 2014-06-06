/* 
 * File:   Timer.cpp
 * Author: lieven
 * 
 * Created on September 7, 2013, 12:25 AM
 */

#include "Timer.h"

#define MAXTIMERS 20
volatile uint32_t timerCount = 0;
Timer* timers[MAXTIMERS];

Timer::Timer(Stream* upStream) : Stream(upStream) {
    //register timer
    ASSERT(timerCount < MAXTIMERS);
    timers[timerCount++] = this;
    _isActive = false;
    _isAutoReload = false;
    _reloadValue = 1000;
    _counterValue = 1000;
    _isExpired=false;
}

Timer::~Timer() {
    ASSERT(false);
}

void Timer::decAll() {
    uint32_t i;
    for (i = 0; i < timerCount; i++)
    	timers[i]->dec();
}

Timer::Timer(uint32_t value, bool reload) {
    _isAutoReload = reload;
    _isActive = false;
    _reloadValue = value;
    _counterValue = value;
    timers[timerCount++] = this;
    _isExpired = false;
}

void Timer::interval(uint32_t interval) {
    _reloadValue = interval;
    _counterValue = interval;
}

void Timer::start() {
    _isActive = true;
    _counterValue = _reloadValue;
    _isExpired = false;
}

bool Timer::expired() {
    return _isExpired;
}

void Timer::start(uint32_t value) {
    interval(value);
    start();
}

void Timer::stop() {
    _isActive = false;
}

void Timer::reload(bool automatic) {
    _isAutoReload = automatic;
}

void Timer::dec() {
    if (_isActive)
        if (--_counterValue == 0) {
        	Event* pEvent=new Event(getUpStream(),this,Timer::EXPIRED);
        	getUpStream()->getQueue()->sendFromIsr(&pEvent);
            _isExpired = true;
            if (_isAutoReload) _counterValue = _reloadValue;
            else _isActive = false;
        }
}

Erc Timer::event(Event* ev) {
    ASSERT(false);
    return E_NO_ACCESS;
}

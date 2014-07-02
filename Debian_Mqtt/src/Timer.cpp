#include "Timer.h"
#include "Timer.h"

int Timer::timerCount=0;
Timer* Timer::timers[MAX_TIMERS];

void Timer::decAll()
{
    uint32_t i;
    for (i = 0; i < timerCount; i++)
        timers[i]->dec();
}
/*
void Timer::addListener(TimerListener* tl){
    _listener = tl;
}
*/

Timer::Timer()
{
    //register timer
    timers[timerCount++] = this;
    _isActive = false;
    _isAutoReload = false;
    _reloadValue = 1000;
    _counterValue = 1000;
    _isExpired = false;
}

Timer::Timer(uint32_t value, bool reload)
{
    _isAutoReload = reload;
    _isActive = false;
    _reloadValue = value;
    _counterValue = value;
    timers[timerCount++] = this;
    _isExpired = false;
}

void Timer::interval(uint32_t interval)
{
    _reloadValue = interval;
    _counterValue = interval;
}

void Timer::start()
{
    _isActive = true;
    _counterValue = _reloadValue;
    _isExpired = false;
}

bool Timer::expired()
{
    return _isExpired;
}

void Timer::start(uint32_t value)
{
    interval(value);
    start();
}

void Timer::stop()
{
    _isActive = false;
}

void Timer::reload(bool automatic)
{
    _isAutoReload = automatic;
}

void Timer::dec()
{
    if (_isActive)
        if (--_counterValue == 0)
        {
            _listener->onTimerExpired(this);
            _isExpired = true;
            if (_isAutoReload)
                _counterValue = _reloadValue;
            else
                _isActive = false;
        }
}

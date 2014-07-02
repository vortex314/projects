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
#include "EventSource.h"


class Timer;
class TimerListener
{
public:
    virtual void onTimerExpired(Timer* src)=0;
};

class Timer: public EventSource
{
public:
    Timer();

    void startRepeat(uint64_t interval);
    void startOnce(uint64_t interval);
    void stop();

    bool isExpired();
    void setExpired(bool value);
    bool isRunning();

private:
    struct TimerStruct;
    TimerStruct* _this;
};

#endif	/* TIMER_H */


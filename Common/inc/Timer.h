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

class Timer: public EventSource<TimerListener>
{
public:
    Timer();

    void startRepeat(uint64_t interval);
    void startOnce(uint64_t interval);
    void stop();

    bool isExpired();
    void setExpired(bool value);
    bool isRunning();

    static void checkAll();

private:
    struct TimerStruct;
    TimerStruct* _this;
};
#include "Thread.h"
class TimerThread : public Thread
{

public:

    TimerThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
public :
    void run() ;
};

#endif	/* TIMER_H */


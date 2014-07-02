#ifndef TIMER_H
#define TIMER_H
#include "Publisher.h"
#include "Erc.h"
#include <stdint.h>
#define MAX_TIMERS 20
class Timer;
class TimerListener
{
public:
    virtual void onTimerExpired(Timer* src)=0;
};
class Timer: public Publisher<TimerListener>
{
public:


    Timer();
    ~Timer();
    Timer(const Timer& ref);
    Timer(uint32_t value, bool reload);
    void interval(uint32_t interval);
    void start();
    void start(uint32_t interval);
    void stop();
    void reload(bool automatic);
    void dec();
    static void decAll();
    bool expired();

private:
    static Timer* timers[MAX_TIMERS];
    static int timerCount;
    uint32_t _counterValue;
    uint32_t _reloadValue;
    bool _isAutoReload;
    bool _isActive;
    bool _isExpired;
    TimerListener* _listener;
};

#endif // TIMER_H

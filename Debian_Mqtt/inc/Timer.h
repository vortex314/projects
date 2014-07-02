#ifndef TIMER_H
#define TIMER_H
#include "Publisher.h"
#include "Erc.h"
#include <stdint.h>
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
    ~Timer();

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

#endif // TIMER_H

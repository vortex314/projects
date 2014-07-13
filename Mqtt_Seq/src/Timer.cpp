#include "Timer.h"
#include "Sys.h"

#define MAX_TIMERS  20

struct Timer::TimerStruct
{
    uint64_t expireTime;
    uint64_t intervalTime;
    bool isRunning;
    bool isAutoReload;
} _this ;

static uint16_t timerCount=0;
static Timer* timers[MAX_TIMERS];

void Timer::checkAll()
{
    uint32_t i;
    for (i = 0; i < timerCount; i++)
        if ( timers[i]->isExpired() && timers[i]->isRunning() )
            timers[i]->setExpired(true);
}


Timer::Timer()  //: TimerStruct(_this)
{
    _this = new TimerStruct();
    _this->isRunning = false;
    _this->isAutoReload = false;
    _this->intervalTime=0;
    _this->expireTime = Sys::upTime();
    timers[timerCount++]=this;
}

void Timer::startOnce(uint64_t interval)
{
    _this->isAutoReload=false;
    _this->expireTime = Sys::upTime()+interval;
    _this->intervalTime=interval;
    _this->isRunning=true;
}

void Timer::startRepeat(uint64_t interval)
{
    _this->isAutoReload=true;
    _this->expireTime = Sys::upTime()+interval;
    _this->intervalTime=interval;
    _this->isRunning=true;
}

bool Timer::isExpired()
{
    return (_this->expireTime <= Sys::upTime());
}

bool Timer::isRunning()
{
    return _this->isRunning;
}



void Timer::stop()
{
    _this->isRunning = false;
}
#include "Mutex.h"
void Timer::setExpired(bool v)
{

    if ( _this->isAutoReload )
    {
        _this->expireTime = Sys::upTime()+_this->intervalTime;
    }
    else
    {
        _this->isRunning =  false;
    }

    TimerListener* l;
    for(l=firstListener(); l!=0; l=nextListener(l))
    {
        Mutex::lock();
        l->onTimerExpired(this);
        Mutex::unlock();
    }
}

#include "Thread.h"
#include <time.h>



void TimerThread::run()
{
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    Sys::_upTime= deadline.tv_sec*1000 + deadline.tv_nsec/1000000;
    while(true)
    {
// Add the time you want to sleep
        deadline.tv_nsec += 1000000000;

// Normalize the time to account for the second boundary
        if(deadline.tv_nsec >= 1000000000)
        {
            deadline.tv_nsec -= 1000000000;
            deadline.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
        Sys::_upTime= deadline.tv_sec*1000 + deadline.tv_nsec/1000000;
//        Timer::checkAll();
        publish(new Event(this,TIMER_TICK,0));
    }

}


//_________________________________________________________
// Queue q(sizeof(Event),10);




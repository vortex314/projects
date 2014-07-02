
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

/*void Timer::decAll()
{
    uint32_t i;
    for (i = 0; i < timerCount; i++)
        timers[i]->dec();
}*/


Timer::Timer() : EventSource() //: TimerStruct(_this)
{
    _this = new TimerStruct();
    _this->isRunning = false;
    _this->isAutoReload = false;
    _this->intervalTime=0;
    _this->expireTime = Sys::upTime();
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


void Timer::stop()
{
    _this->isRunning = false;
}

void Timer::setExpired(bool v){
    ((TimerListener*)firstListener())->onTimerExpired(this);
    if ( _this->isAutoReload ) {
        _this->expireTime = Sys::upTime()+_this->intervalTime;
    } else {
        _this->isRunning =  false;
    }
}

#include "Thread.h"
#include <time.h>
class TimerThread : public Thread
{

public:

    TimerThread( const char *name, unsigned short stackDepth, char priority):Thread(name, stackDepth, priority)
    {
    };
public :
    void run()
    {
        while(true)
        {

            struct timespec deadline;
            clock_gettime(CLOCK_MONOTONIC, &deadline);

// Add the time you want to sleep
            deadline.tv_nsec += 1000;

// Normalize the time to account for the second boundary
            if(deadline.tv_nsec >= 1000000000)
            {
                deadline.tv_nsec -= 1000000000;
                deadline.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
 //           Timer::decAll();
        }
    }
};

//_________________________________________________________
// Queue q(sizeof(Event),10);


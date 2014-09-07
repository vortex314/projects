#include <pthread.h>
#include <unistd.h> // sleep
#include <malloc.h>
#include "Thread.h"

extern "C" void* pvTaskCode(void *pvParameters)
{
    (static_cast<Thread*>(pvParameters))->run();
    return NULL;
}

Thread::Thread(const char *name, unsigned short stackDepth, char priority)
{
    _ref = ::malloc(sizeof(pthread_t));


}

void Thread::start(){
    /* create threads */
    pthread_t* pThread = (pthread_t*)_ref;
    pthread_create (pThread, NULL,  pvTaskCode, (void *) this);
}

void Thread::run()
{
    while(true)
    {
        ::sleep(10);
    }
}

void Thread::sleep(uint32_t time)
{
    ::sleep(time/1000);
}

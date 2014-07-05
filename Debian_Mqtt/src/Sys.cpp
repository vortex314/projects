#include "Sys.h"

#include <malloc.h>

uint64_t Sys::_upTime=0;

uint64_t Sys::upTime()  // time in msec since boot, only increasing
{
    return _upTime;
}

void * Sys::malloc(uint32_t size)
{
    return ::malloc(size);
#ifdef FREERTOS
    return pvPortMalloc(size);
#endif
#ifdef NO_RTOS
    int i;
    for (i = 0; i < blockCount; i++)   // find free of same size
    {
        if (block[i].free)
            if (block[i].size == size)
            {
                block[i].free = false;
                return (void*) block[i].start;
            }
    }
    uint32_t i32Size = ( size+3) / 4;
    void* ptr;
    ASSERT((myHeapOffset + i32Size) < HEAP_SIZE);
    ptr = (void*) &myHeap[myHeapOffset];
    myHeapOffset += i32Size;
    if (blockCount < BLOCK_MAX)
    {
        int i = blockCount++;
        block[i].size = size;
        block[i].free = false;
        block[i].start = ptr;
    }
    else while(1);
    return ptr;
#endif
}

void Sys::free(void *pv)
{
    ::free(pv);
#ifdef FREERTOS
    vPortFree(pv);
#endif
#ifdef NO_RTOS
    int i;
    for (i = 0; i < blockCount; i++)
    {
        if (block[i].free==false)
            if (block[i].start == pv)
            {
                block[i].free = true;
                return;
            }
    }
    while(1);
    ASSERT(false); // trying to free something never allocated
#endif
}
#include "Logger.h"
Logger* Sys::_logger =  new Logger(100);
Logger&  Sys::getLogger()
{
 //   if ( Sys::_logger == 0) Sys::_logger=new Logger();
    return *Sys::_logger;
};


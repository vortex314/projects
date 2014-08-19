#ifndef QUEUE_H
#define QUEUE_H
#include <stdint.h>
#include "Erc.h"

#include "Event.h"

#define DEFAULT_QUEUE_DEPTH 100
class Queue
{
public:
    Queue(uint32_t elementSize,uint32_t depth);
    virtual ~Queue();

    Erc put(void* data);
    Erc get(void* data);
    Erc clear();

    static Queue* getDefaultQueue();

protected:
private:
    uint32_t _msgSize;
    void* _ref;

    static Queue* _defaultQueue;

};

#endif // QUEUE_H

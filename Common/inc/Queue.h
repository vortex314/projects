#ifndef QUEUE_H
#define QUEUE_H
#include <stdint.h>
#include "Erc.h"
#include <mqueue.h>
#include "Event.h"

#define DEFAULT_QUEUE_DEPTH 10
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
    mqd_t mq;

    static Queue* _defaultQueue;

};

#endif // QUEUE_H

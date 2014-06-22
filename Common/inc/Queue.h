#ifndef QUEUE_H
#define QUEUE_H
#include <stdint.h>
#include "Erc.h"
#include <mqueue.h>
class Queue
{
public:
    Queue(uint32_t elementSize,uint32_t depth);
    virtual ~Queue();

    Erc put(void* data);
    Erc get(void* data);

protected:
private:
    uint32_t _msgSize;
    void* _ref;
    mqd_t mq;

};

#endif // QUEUE_H

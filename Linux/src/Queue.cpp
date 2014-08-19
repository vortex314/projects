#include "Queue.h"
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>    /* For mode constants */
#include <mqueue.h>
#include "Sys.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define PQ ((mqd_t*)_ref)
#define QUEUE_NAME "/MQ2"
//    mqd_t mq;
Queue::Queue(uint32_t elementSize,uint32_t depth)
{
    _ref = new mqd_t; //
    // Sys::malloc(sizeof(mqd_t));
    mqd_t* mq = (mqd_t*)_ref;

    struct mq_attr attr;

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = depth;
    attr.mq_msgsize = elementSize;
    attr.mq_curmsgs = 0;

    *mq = mq_open(QUEUE_NAME, O_RDWR);
    if ( (*mq < 0) && (errno==ENOENT) )
    {
        /* create the message queue if it doesn't exist */
        *mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0777, &attr);
    }
    if ( *mq < 0 )
    {
        perror("mq_open");
        ASSERT(false);
    }

    _msgSize = elementSize;
}

Queue::~Queue()
{
    //dtor
}



Erc Queue::put(void* data)
{
    struct   timespec tm;
    mqd_t* mq = (mqd_t*)_ref;

    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += 1;
    if( mq_timedsend( *mq, (const char *)data ,_msgSize, NULL,  &tm )<0 )
    {
        perror("mq_send : QUEUE FULL ?");
        return E_AGAIN;
    }
    else
    {
//       perror("mq_send : OK");
    }
    /*    if ( mq_send(mq, (const char *)data ,_msgSize,1) < 0 )
        {
            perror("mq_send");
            return E_AGAIN;
        };*/
    return E_OK;
}

Erc Queue::get(void* data)
{
    size_t msgSize = _msgSize;
    unsigned int prio;
    mqd_t* mq = (mqd_t*)_ref;
    if ( mq_receive(*mq, (char *)data ,msgSize,&prio) < 0 )
    {
        perror("mq_receive");
        return E_AGAIN;
    };
    return E_OK;
}

Erc Queue::clear()
{
    size_t msgSize = _msgSize;
    char data[20];
    mqd_t* mq = (mqd_t*)_ref;
    struct   timespec tm;

    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_nsec += 1000000;
    while ( mq_timedreceive( *mq, (char*)data, msgSize, NULL,  &tm ) >0 )
    {
    }
    return E_OK;
}


Queue* Queue::_defaultQueue=(Queue*)NULL;
Queue* Queue::getDefaultQueue()
{
    if (_defaultQueue == NULL)
        _defaultQueue = new Queue(sizeof(Event),DEFAULT_QUEUE_DEPTH );
    return _defaultQueue;
}

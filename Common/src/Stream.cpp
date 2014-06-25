/*
 * File:   Streams.cpp
 * Author: lieven2
 *
 * Created on 15 september 2013, 17:58
 */

#include "Stream.h"

#define MAX_LISTENS 20


uint32_t listeners=0;

Queue* Stream::_defaultQueue=(Queue*)NULL;
void Stream::eventHandler(Event* pEvent)
{

}
/*
Queue* Stream::_defaultQueue=NULL;

Queue* Stream::getDefaultQueue() {
	if (_defaultQueue == NULL)
		_defaultQueue = new Queue(20, 4);
	return _defaultQueue;
}

Stream::Stream() {
	_queue = getDefaultQueue();
	_upStream = this;
}

Stream::Stream(Stream* upStream = NULL) {
	_queue = getDefaultQueue();
	_upStream = upStream;
}

Stream::~Stream() {
	ASSERT(false);
}

Erc Stream::enqueue(Event* pev) {
	return _queue->send(&pev);
}

Erc Stream::enqueue(Stream* dst, Stream *src, uint32_t event) {
	Event* pev = new Event(dst, src, event);
	return _queue->send(&pev);
}

Erc Stream::upQueue(uint32_t event) {
	return _upStream->enqueue(_upStream, this, event);
}

Stream* Stream::getUpStream() {
	return _upStream;
}
void Stream::setUpStream(Stream* stream) {
	_upStream = stream;
}

void Stream::setQueue(Queue* q) {
	_queue = q;
}

Queue* Stream::getQueue() {
	return _queue;
}
*/

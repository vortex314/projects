/*
 * File:   Event.h
 * Author: lieven
 *
 * Created on September 2, 2013, 10:50 PM
 */

#ifndef EVENT_H
#define	EVENT_H
#include <stdint.h>

#define EVENT(cls,type) ((cls<<24)+(type<<16))
#define EVENT_CHAR(d,c,b,a) ((a<<24)+(b<<16)+(c<<8)+(d))
#define SE(__status,__event)  ( (__event & 0xFFFF0000) + __status)
#include "QueueTemplate.h"

class Stream;
class Sys;
/*
 typedef struct {
 Stream* dst;
 void *src;
 uint32_t type;
 void *data;
 } Event;
 */
class Event {
public:
	Event();
	Event(Stream *dst, void* src, uint32_t id);
	bool is(uint32_t value);
	bool is(uint32_t clsType, uint16_t detail);
	uint32_t is();
	uint32_t id();
	Stream *dst();
	void *src();
	uint16_t detail();
public:
	Stream* _dst;
	void* _src;
	uint32_t _id;
};

#endif	/* EVENT_H */


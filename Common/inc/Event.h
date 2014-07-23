/*
 * File:   Event.h
 * Author: lieven
 *
 * Created on September 2, 2013, 10:50 PM
 */

#ifndef EVENT_H
#define	EVENT_H
#include <stdint.h>
#include "Str.h"

#define EVENT(cls,type) ((cls<<24)+(type<<16))
#define EV(d,c,b,a) ((a<<24)+(b<<16)+(c<<8)+(d))
#define SE(__status,__event)  ( (__event & 0xFFFF0000) + __status)
#define GEN_EVENT_ID(x) EventId x=Event::nextEventId( #x )

#include "QueueTemplate.h"

class Stream;
class Sys;
#define MAX_EVENT_ID    20
typedef uint8_t EventId;
class EventData {
    virtual void toString(Str& str)=0;
};

class Event {
public:
	Event();
	Event(void* src, EventId id,EventData *data);
	bool is(EventId value);
	bool is(void* src,EventId id);
	EventId id();
	EventData* data();
	void* src();
	static EventId nextEventId(char *s);
	char *getEventIdName();
	void toString(Str& line);
private:
	void* _src;
	EventId _id;
	EventData* _data;
	static uint32_t eventIdCount;
	static char* eventNames[MAX_EVENT_ID];
};

#endif	/* EVENT_H */


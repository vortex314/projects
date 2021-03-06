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
#define MAX_EVENT_ID    50
typedef int EventId;
/*class EventData {
 virtual void toString(Str& str)=0;
 };*/
/*
enum Signal {
	SIG_IDLE,
	SIG_ENTRY,
	SIG_EXIT,
	SIG_INIT,
	SIG_TIMEOUT,
	SIG_TIMER_TICK,
	SIG_USER,
	SIG_LINK_CONNECTED = SIG_USER + 1,
	SIG_LINK_DISCONNECTED,
	SIG_LINK_RXD,
	SIG_MQTT_CONNECTED,
	SIG_MQTT_DISCONNECTED,
	SIG_MQTT_MESSAGE,
	SIG_MQTT_DO_PUBLISH,
	SIG_MQTT_PUBLISH_FAILED,
	SIG_MQTT_PUBLISH_OK,
	SIG_PROP_CHANGED
};
*/
#include "Signal.h"

class Event {
public:
	Event();
	Event(int id);
	Event(int id, uint16_t w);
	bool is(int id);
	bool is(int id, uint16_t w);
	int id();
	uint16_t detail();
	uint8_t byte(int i);
	static EventId nextEventId(const char *s);
	char *getEventIdName();
	void toString(Str& line);
public:

	Signal _id ;
	uint16_t _w :16;

	static uint32_t eventIdCount;
	static const char* eventNames[MAX_EVENT_ID];
	static Erc nextEvent(Event& event);
	static Erc publish(Event& event);
};

#endif	/* EVENT_H */


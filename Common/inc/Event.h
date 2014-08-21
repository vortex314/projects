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
typedef int EventId;
class EventData {
        virtual void toString(Str& str)=0;
    };

class Event {
    public:
        Event();
        Event(int id);
        Event(int id,uint16_t w);
        bool is(int id);
        bool is(int id,uint16_t w);
        int id();
        uint16_t detail();
        uint8_t byte(int i);
        static EventId nextEventId(const char *s);
        char *getEventIdName();
        void toString(Str& line);
    private:
        struct {
            uint8_t _id:8;
            union {
                uint8_t _b[3];
                uint16_t _w;
                };
            };
        static uint32_t eventIdCount;
        static const char* eventNames[MAX_EVENT_ID];
    };

#endif	/* EVENT_H */


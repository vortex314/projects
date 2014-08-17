#include "Event.h"
#include "Sys.h"
#include "Erc.h"

uint32_t Event::eventIdCount=0;
char* Event::eventNames[MAX_EVENT_ID];

EventId Event::nextEventId(char *name)
{
    eventNames[eventIdCount]=name;
    return eventIdCount++;
}

char* Event::getEventIdName()
{
    return eventNames[_id];
}

Event::Event()
{
}

Event::Event(void* src, EventId id,EventData *data)
{
    _src = src;
    _id = id;
    _data = data;
}

bool Event::is(EventId id)
{
    if ((_id) == (id))
        return true;
    return false;
}
bool Event::is(void* src,EventId id)
{
    if ((_id) == (id))
        if (src == _src)
            return true;
    return false;
}

EventId Event::id()
{
    return _id;
}

void *Event::src()
{
    return _src;
}

EventData* Event::data()
{
    return _data;
}

#include "MqttIn.h"
//#include "Tcp.h"
void Event::toString(Str& out)
{
    out.append("{ id : ");
    if (eventNames[_id]) out.append(eventNames[_id]);

/*    if ( data() )
    {
            out.append(", data : ");
        if ( id() == Tcp::MQTT_MESSAGE)
            ((MqttIn*)data())->toString(out);
    }*/
    out.append(" }");
}


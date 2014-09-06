#include "Event.h"
#include "Sys.h"
#include "Erc.h"

QueueTemplate<Event> defaultQueue(10);

Erc Event::nextEvent(Event& event) {
	return defaultQueue.get(event);
}

Erc Event::publish(Event& event){
	return defaultQueue.put(event);
}

char* Event::getEventIdName()
{
    return (char*)eventNames[_id];
}

Event::Event()
{
}

	Event::Event(int id){
	    _id=(Signals)id;
	    _w=0;
	}

Event::Event(int id,uint16_t w)
{
    _id=(Signals)id;
    _w=w;
}

bool Event::is(int id)
{
    if ((_id) == (id))
        return true;
    return false;
}
bool Event::is(int id,uint16_t w)
{
    if ((_id) == (id))
        if (w == _w)
            return true;
    return false;
}

EventId Event::id()
{
    return _id;
}

uint16_t Event::detail()
{
    return _w;
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
    out.append(" } = ") << _id ;
}


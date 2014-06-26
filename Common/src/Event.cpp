#include "Event.h"
#include "Sys.h"
#include "Erc.h"

Event::Event()
{
    _src = (Stream*) 0;
    _id = EVENT('U', 'U');
}

Event::Event(Stream* src, int32_t id,void *data)
{
    _src = src;
    _id = id;
    _data = data;
}

bool Event::is(int32_t type)
{
    if ((_id) == (type))
        return true;
    return false;
}
bool Event::is(Stream* src,int32_t type)
{
    if ((_id) == (type))
        if (src == _src)
            return true;
    return false;
}

int32_t Event::id()
{
    return _id;
}

Stream *Event::src()
{
    return _src;
}

void* Event::data()
{
    return _data;
}


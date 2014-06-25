#include "Event.h"
#include "Sys.h"
#include "Erc.h"

Event::Event()
{
    _src = (Stream*) 0;
    _dst = (Stream*) 0;
    _id = EVENT('U', 'U');
}

Event::Event(Stream* dst, Stream* src, uint32_t id,void *data)
{
    _src = src;
    _dst = dst;
    _id = id;
    _data = data;
}

bool Event::is(uint32_t type)
{
    if ((_id) == (type))
        return true;
    return false;
}
bool Event::is(Stream* src,uint32_t type)
{
    if ((_id) == (type))
        if (src == _src)
            return true;
    return false;
}

uint32_t Event::id()
{
    return _id;
}

Stream *Event::src()
{
    return _src;
}

Stream *Event::dst()
{
    return _dst;
}

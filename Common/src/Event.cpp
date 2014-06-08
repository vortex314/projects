#include "Event.h"
#include "Sys.h"
#include "Erc.h"

Event::Event() {
    _src = (Stream*)0;
    _dst = (Stream*)0;
    _id = EVENT('U', 'U');
}

Event::Event(Stream* dst, Stream* src, uint32_t id) {
    _src = src;
    _dst = dst;
    _id = id;
}

bool Event::is(uint32_t clsType) {
    if ((_id & 0xFFFF0000) == (clsType))
        return true;
    return false;
}

bool Event::is(uint32_t clsType, uint16_t detail) {
    if ((_id & 0xFFFF0000) == clsType && (_id & 0x0FFFF) == detail)
        return true;
    return false;
}

uint32_t Event::is() {
    return _id & 0xFFFF0000;
}

uint32_t Event::id() {
    return _id;
}

uint32_t Event::clsType() {
    return _id & 0xFFFF0000;
}

uint16_t Event::detail() {
    return _id & 0x0FFFF;
}

Stream *Event::src() {
    return _src;
}

Stream *Event::dest() {
    return _dst;
}

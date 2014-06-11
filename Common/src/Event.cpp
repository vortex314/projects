#include "Event.h"
#include "Sys.h"
#include "Erc.h"

Event::Event() {
	_src = (Stream*) 0;
	_dst = (Stream*) 0;
	_id = EVENT('U', 'U');
}

Event::Event(Stream* dst, void* src, uint32_t id, void *p) {
	_src = src;
	_dst = dst;
	_id = id;
	_data = p;
}

bool Event::is(uint32_t type) {
	if ((_id) == (type))
		return true;
	return false;
}

uint32_t Event::id() {
	return _id;
}

uint16_t Event::detail() {
	return _id & 0x0FFFF;
}

void *Event::src() {
	return _src;
}

Stream *Event::dst() {
	return _dst;
}

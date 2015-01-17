/*
 * Prop.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"
#include "Prop.h"
// #include "Fsm.h"

Prop* Prop::_first;

const char* sType[] =
{ "UINT8", "UINT16", "UINT32", "UINT64", "INT8", "INT16", "INT32", "INT64",
		"BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY", "MAP", "STR", "OBJECT" };

const char* sMode[] =
{ "READ", "WRITE", "RW" };

Prop::Prop(const char* name, Flags flags)
{
	init(name, flags);
}

void Prop::init(const char* name, Flags flags)
{
	_name = name;
	_flags = flags;
	_flags.doPublish = true;
	_lastPublished = 0;
	if (_first == 0)
		_first = this;
	else
	{
		Prop* cursor = _first;
		while (cursor->_next != 0)
		{
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}
static int pow10[10] =
{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
bool Prop::hasToBePublished()
{
	if (_flags.mode == M_WRITE)
		return false;
	if (Sys::upTime() > (_lastPublished + pow10[_flags.interval]))
		return true;
	return _flags.doPublish;
}

void Prop::doPublish()
{
	_flags.doPublish = true;
}

void Prop::isPublished()
{
	_flags.doPublish = false;
	_lastPublished = Sys::upTime();
}

void Prop::metaToBytes(Bytes& message)
{
	Cbor msg(message);
	int r;
	msg.addMap(-1);
	msg.add("type");
	msg.add(sType[_flags.type]);
	msg.add("mode");
	r = _flags.mode;
	msg.add(sMode[r]);
	msg.add("qos");
	msg.add(_flags.qos);
	msg.add("interval");
	r = _flags.interval;
	msg.add(pow10[r]);
	msg.addBreak();
}

#include <cstdlib>

Prop* Prop::findProp(Str& name)
{
	Prop* cursor = _first;
	while (cursor != 0)
	{
		if (name.equals(cursor->_name))
			return cursor;
		cursor = cursor->_next;
	}
	return 0;
}

extern PropMgr propMgr;

void PropMgr::set(Str& topic, Bytes& message)
{
	Str str(TOPIC_MAX_SIZE);

	if (topic.startsWith(_mqtt->_putPrefix))   // "PUT/<device>/<topic>
	{
		str.substr(topic, _mqtt->_putPrefix.length());
		Prop* p = Prop::findProp(str);
		if (p)
		{
			message.offset(0);
			p->fromBytes(message);
			p->doPublish();
			nextProp(p);
		}
	}
	else if (topic.startsWith(_mqtt->_getPrefix))     // "GET/<device>/<topic>
	{
		str.substr(topic, _mqtt->_getPrefix.length());
		Prop* p = Prop::findProp(str);
		if (p)
		{
			if (message.length() == 0)
			{
				p->doPublish();
				nextProp(p);
			}
			else
			{
				Str t(TOPIC_MAX_SIZE);
				t << p->_name;
				t << ".META";
				Bytes msg(MSG_MAX_SIZE);
				p->metaToBytes(msg);
				_mqtt->publish(t, msg, (Flags
						)
						{ T_MAP, M_READ, T_100SEC, QOS_0, NO_RETAIN });
			}
		}

	}
	else if (topic.startsWith(_mqtt->_headPrefix))     // "HEAD/<device>/<topic>
	{

	}

}

PropMgr::PropMgr() :
		_topic(TOPIC_MAX_SIZE), _message(MSG_MAX_SIZE)
{
	_cursor = Prop::_first;
	_state = ST_DISCONNECTED;
	_next = 0;
	PT_INIT(&t);
}

void PropMgr::mqtt(Mqtt& mq)
{
	_mqtt = &mq;
}

void PropMgr::nextProp()
{
	if (_next)
	{
		_cursor = _next;
		_next = 0;
	}
	else
	{
		_cursor = _cursor->_next;
		if (_cursor == 0)
			_cursor = Prop::_first;
	}
}

void PropMgr::nextProp(Prop* next)
{
	_next = next;
}

int PropMgr::ptRun(Msg& msg)
{
	if (msg.signal == SIG_DISCONNECTED)
	{
		restart();
		return 0;
	}
	PT_BEGIN(&pt)
		while (true)
		{
			PT_YIELD_UNTIL(&pt, msg.is(_mqtt, SIG_CONNECTED));
			_cursor = Prop::_first;
			while (true)
			{
				timeout(10);
				PT_YIELD(&pt);
				if (msg.is(_mqtt, SIG_DISCONNECTED))
					break;
				if (_cursor->hasToBePublished())
				{
					_topic = _mqtt->_prefix;
					_topic << _cursor->_name;
					_message.clear();
					_cursor->toBytes(_message);
					if (_mqtt->publish(_topic, _message, _cursor->_flags))
					{
						timeout(TIME_WAIT_REPLY);
						PT_YIELD_UNTIL(&pt,msg.is(_mqtt,SIG_SUCCESS | SIG_FAIL | SIG_DISCONNECTED) || timeout());
						if (msg.signal == SIG_SUCCESS)
						{
							_cursor->isPublished();
							nextProp();
						}
					}
				}
				else
				{
					nextProp();
				}

			}
		}
	PT_END(&pt)
}

void Prop::publishAll()
{
Prop* cursor = Prop::_first;
while (cursor->_next != 0)
{
	cursor->doPublish();
	cursor = cursor->_next;
}
}

/*
 * Handler.cpp
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#include "Handler.h"
#include "Sys.h"

Handler::Handler()
{
	_timeout = UINT64_MAX;
	_sigMask = UINT32_MAX; // accept all
	_name = "UNDEFINED";
	_next = 0;
	PT_INIT(&pt);
	reg();
}

Handler::Handler(const char* name)
{
	_timeout = UINT64_MAX;
	_sigMask = UINT32_MAX; // accept all
	_name = name;
	_next = 0;
	PT_INIT(&pt);
	reg();
}

void Handler::restart()
{
	PT_INIT(&pt);
	_sigMask = UINT32_MAX;
	_timeout = UINT64_MAX;
}

void Handler::timeout(uint32_t msec)
{
	_timeout = Sys::upTime() + msec;
}

bool Handler::timeout()
{
	return _timeout < Sys::upTime();
}

uint64_t Handler::getTimeout()
{
	return _timeout;
}

void Handler::listen(uint32_t sigMask)
{
	_sigMask = sigMask;
}

void Handler::listen(void* src)
{
	_srcMask = src;
}

void Handler::listen(uint32_t sigMask, uint32_t time)
{
	_sigMask = sigMask | SIG_TIMEOUT;
	timeout(time);
}

void Handler::listen(uint32_t sig,void* src){
	_sigMask=sig;
	_srcMask=src;
}

bool Handler::accept(Signal signal, void* src)
{
	if (_srcMask == 0 || src == 0)
		return (signal & _sigMask);
	else
		return (_srcMask == src) && (signal & _sigMask);
}

Msg timeoutMsg(SIG_TIMEOUT);

void Handler::dispatch(Msg& msg)
{
	if (accept(msg.sig(),msg.src()))
		ptRun(msg);
	else if (msg.is(SIG_TICK,0) && (_sigMask & SIG_TIMEOUT) && timeout())
		ptRun(timeoutMsg);

}

//_________________________________________________________________________________________________
//
//				HANDLER LIST
//_________________________________________________________________________________________________

Handler* Handler::_first = 0;

Handler* Handler::first()
{
	return _first;
}

Handler* Handler::next()
{
	return _next;
}

void Handler::reg()
{
	if (_first == 0)
		_first = this;
	else
	{
		Handler* cursor = _first;
		while (cursor->_next != 0)
		{
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

//_________________________________________________________________________________________________
//
//				LISTENER LIST
//_________________________________________________________________________________________________


/*
 * Handler.cpp
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */

#include "Handler.h"
#include "Sys.h"
#include "Msg.h"

Handler::Handler()
{
	_timeout = UINT64_MAX;
	_name = "UNDEFINED";
	_next = 0;
	_firstChild = 0;
	PT_INIT(&pt);
}

Handler::Handler(const char* name)
{
	_timeout = UINT64_MAX;
	_name = name;
	_next = 0;
	_firstChild = 0;
	PT_INIT(&pt);
}

void Handler::restart()
{
	PT_INIT(&pt);
	_timeout = UINT64_MAX;
	Msg initMsg =
	{ this, SIG_INIT, 0, 0 };
	dispatch(initMsg);

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
/*
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

 void Handler::listen(uint32_t sig, void* src)
 {
 _sigMask = sig;
 _srcMask = src;
 }

 bool Handler::accept(Signal signal, void* src)
 {
 if (_srcMask == 0 || src == 0)
 return (signal & _sigMask);
 else
 return (_srcMask == src) && (signal & _sigMask);
 }
 */

//_________________________________________________________________________________________________
//
//				HANDLER LIST
//_________________________________________________________________________________________________
Handler* Handler::first()
{
	return _firstChild;
}

Handler* Handler::next()
{
	return _next;
}

void Handler::reg(Handler* hdlr)
{
	if (_firstChild == 0)
		_firstChild = hdlr;
	else
	{
		Handler* cursor = _firstChild;
		while (cursor->_next != 0)
		{
			cursor = cursor->_next;
		}
		cursor->_next = hdlr;
	}
}

//_________________________________________________________________________________________________
//
//				LISTENER LIST
//_________________________________________________________________________________________________
Handler* hdlr;
Msg timeoutMsg =
{ 0, SIG_TIMEOUT, 0, 0 };
void Handler::dispatchToChilds(Msg& msg)
{
	Handler* hdlr;
	for (hdlr = first(); hdlr != 0; hdlr = hdlr->next())
	{
		hdlr->dispatch(msg);
	}
}

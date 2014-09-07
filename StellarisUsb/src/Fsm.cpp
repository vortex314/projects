#include "Event.h"
#include "Board.h"
#include "Fsm.h"



typedef void (Fsm::*SF)(Event& e);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

Event EV_ENTRY(SIG_ENTRY);
Event EV_EXIT(SIG_EXIT);

Fsm* Fsm::_first = 0;

Fsm::Fsm() {
	_f = &Fsm::null;
	_timeout = 0;
	_next = 0;
	reg();
}

void Fsm::dispatch(Event& event) {
	SF f;
	f = _f;
	CALL_MEMBER_FN(*this,_f)(event);
	if (f != _f)	// if state changed
			{
		CALL_MEMBER_FN(*this,f)(EV_EXIT);
		CALL_MEMBER_FN(*this,_f)(EV_ENTRY);
	}
}

void Fsm::tran(SF f) {
	_f = f;
}
void Fsm::init(SF f) {
	_f = f;
	return CALL_MEMBER_FN(*this,_f)(EV_ENTRY);
}

void Fsm::null(Event& ev) {

}

bool Fsm::isInState(SF f) {
	if (f == _f)
		return true;
	return false;
}

void Fsm::timeout(uint32_t msec) {
	_timeout = Sys::upTime() + msec;
}
bool Fsm::timeout() {
	return _timeout < Sys::upTime();
}

void Fsm::reg() {
	if (_first == 0)
		_first = this;
	else {
		Fsm* cursor = _first;
		while (cursor->_next != 0) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

void Fsm::dispatchToAll(Event& event) {
	Fsm* cursor = _first;
	while (cursor != 0) {
		cursor->dispatch(event);
		cursor = cursor->_next;
	}
}

void Fsm::publish(int sig) {
	Event ev(sig);
	ASSERT(Event::publish(ev)==E_OK);
}

void Fsm::publish(int sig, int detail) {
	Event ev(sig, detail);
	ASSERT(Event::publish(ev)==E_OK);
}




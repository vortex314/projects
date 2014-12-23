#include "Event.h"
#include "Fsm.h"
#include "Msg.h"



typedef void (Fsm::*SF)(Msg& e);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

Msg EV_ENTRY(SIG_ENTRY);
Msg EV_EXIT(SIG_EXIT);

Fsm* Fsm::_first = 0;

Fsm::Fsm() {
	_f = &Fsm::null;
	_timeout = 0;
	_next = 0;
	reg();
}

void Fsm::dispatch(Msg& msg) {
	SF f;
	f = _f;
	CALL_MEMBER_FN(*this,_f)(msg);
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
//	return CALL_MEMBER_FN(*this,_f)(EV_ENTRY);
}

void Fsm::null(Msg& ev) {	// do nothing

}

bool Fsm::isInState(SF f) {
	if (f == _f)
		return true;
	return false;
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

void Fsm::dispatchToAll(Msg& msg) {
	Fsm* cursor = _first;
	while (cursor != 0) {
		cursor->dispatch(msg);
		cursor = cursor->_next;
	}
}




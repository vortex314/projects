#include "Event.h"
#include "Queue.h"
#include "Msg.h"
#include "MqttIn.h"

#ifndef FSM_H
#define FSM_H
class Fsm;

typedef void (Fsm::*SF)(Msg& e);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#define TRAN(__x) tran(static_cast<SF>(&__x))

class Fsm  {
private:
	SF _f;
	uint64_t _timeout;
	Fsm* _next;
	static Fsm* _first;
public:
	Fsm();
	static Fsm* first() {
		return _first;
	}

	static Fsm* next(Fsm* fsm) {
		return fsm->_next;
	}


	virtual void dispatch(Msg& msg);

	void tran(SF f);
	void init(SF f);
	bool isInState(SF f);

	void null(Msg& ev);

//	void timeout(uint32_t msec);
//	bool timeout();
//	static void publish(int sig);
//	static void publish(int sig, int detail);
	static void dispatchToAll(Msg& event);
	static Erc nextEvent(Msg& event);
	void reg();

};

#endif


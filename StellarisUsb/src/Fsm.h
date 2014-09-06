#include "Event.h"
#include "Board.h"
#include "Queue.h"

#ifndef FSM_H
#define FSM_H
class Fsm;

typedef void (Fsm::*SF)(Event& e);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#define TRAN(__x) tran(static_cast<SF>(&__x))



class Fsm {
private:
	SF _f;
	uint64_t _timeout;
	Fsm* _next;
	static Fsm* _first;
public:
	Fsm();

	virtual void dispatch(Event& event);

	void tran(SF f);
	void init(SF f);
	bool isInState(SF f);

	void null(Event& ev);

	void timeout(uint32_t msec);
	bool timeout();
	static void publish(int sig);
	static void publish(int sig,int detail);
	static void dispatchToAll(Event& event);
	static Erc nextEvent(Event& event);
	void reg();

};

#endif


#include "Sys.h"
#include "Sequence.h"
#include "Timer.h"

#include <stm32f0xx.h>

void eventPump() {
	Event event;
	while (Sequence::get(event) == E_OK) {
		int i;
		for (i = 0; i < MAX_SEQ; i++)
			if (Sequence::activeSequence[i]) {
				if (Sequence::activeSequence[i]->handler(&event) == PT_ENDED) {
					Sequence* seq = Sequence::activeSequence[i];
					seq->unreg();
					delete seq;
				};
			}
	}
}

#include "Board.h"
class LedSeq: public Sequence {
private:
	struct pt t;

public:
	LedSeq() {
		PT_INIT( &t);
	}
	int handler(Event* event) {
		PT_BEGIN ( &t )
			while (true) {
				timeout(250);
				PT_YIELD_UNTIL( &t, timeout());
				Board::setLedOn(Board::LED_GREEN, true);
				timeout(250);
				PT_YIELD_UNTIL( &t, timeout());
				Board::setLedOn(Board::LED_GREEN, false);
			}
		PT_END ( &t );
}
};

 int main(void) {
	Sys::init();
	Board::init();
	LedSeq ledSeq;
	uint64_t clock = Sys::upTime() + 100;
	while (1) {
			eventPump();
			if (Sys::upTime() > clock) {
				clock += 10;
				Sequence::publish(Timer::TICK);
			}
		}
	return 0;
}
#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line) {
	while(1);
}
#endif

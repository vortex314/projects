/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Usb.h"
#include "Board.h"
#include "Mqtt.h"
#include "Timer.h"
#include "Prop.h"

#define LED_INTERVAL 100
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
;

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int main(void) {
	Board::init();	// initialize usb
	Usb::init();
	Usb usb;		// usb active object
	LedSeq ledSeq; 	// blinking led AO
	Mqtt mqtt(usb);	// mqtt active object
	PropertyListener propertyListener(mqtt);
	uint64_t clock = Sys::upTime() + 100;
	while (1) {
		eventPump();
		if (Sys::upTime() > clock) {
			clock += 10;
			Sequence::publish(Timer::TICK);
		}
	}
}

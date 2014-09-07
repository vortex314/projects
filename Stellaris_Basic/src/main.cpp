#include <stdio.h>
#include <inc/hw_can.h>
#include <inc/hw_sysctl.h>
#include "Sys.h"
#include <driverlib/systick.h>
#include "Stream.h"
#include "Event.h"
#include "Timer.h"
#include "Led.h"

/*
 * hello.c
 */
int main(void) {
	Event event;
	Sys::init();

	Led ledGreen(Led::LED_GREEN);
	Led ledRed(Led::LED_RED);
	Led ledBlue(Led::LED_BLUE);

	ledGreen.blink(1);
	ledRed.light(false);
	ledBlue.light(false);


	while (true) { // EVENTPUMP
		if (Stream::dequeue(&event) == 0) { // event waiting
			event.dest()->event(event);
		}
	}
}

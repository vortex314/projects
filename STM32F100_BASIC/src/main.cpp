/*
 * main.cpp
 *
 *  Created on: 25-jun.-2015
 *      Author: lieven2
 */

#include "Board.h"
#include "Msg.h"

#include <malloc.h>
#include "Handler.h"

int main(void) {
	Board::init();	// initialize usb

	uint64_t clock = Sys::upTime() + 100;
	MsgQueue::publish(0, SIG_INIT, 0, 0);				// kickoff all engines
	Msg msg;
	while (1) {
		if (Sys::upTime() > clock) {
			clock += 10;		// 10 msec timer tick
			MsgQueue::publish(0, SIG_TICK, 0, 0); // check timeouts every 10 msec
		}
	}
}

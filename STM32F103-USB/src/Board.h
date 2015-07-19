/*
 * Board.h
 *
 *  Created on: 14-jul.-2015
 *      Author: lieven2
 */

#ifndef SRC_BOARD_H_
#define SRC_BOARD_H_

#include <core_cm3.h>

class Board {
	static void inline disableInterrupts() {
		__disable_irq();

	}
	static void inline enableInterrupts() {
		__enable_irq();
	}
};



#endif /* SRC_BOARD_H_ */

/*
 * Board.h
 *
 *  Created on: 26-aug.-2014
 *      Author: lieven2
 */

#ifndef BOARD_H_
#define BOARD_H_
#include <stdint.h>

class Board {
public:
	static void init();
	typedef enum {
			BUTTON_LEFT = 1, BUTTON_RIGHT
		} Button;
		typedef enum {
			LED_GREEN = 1, LED_BLUE, LED_RED
		} Led;


		static void setLedOn(int32_t led, bool on);
};

#endif /* BOARD_H_ */

/*
 * Board.h
 *
 *  Created on: 9-dec.-2012
 *      Author: lieven2
 */

#ifndef BOARD_H_
#define BOARD_H_
#include <stdint.h>
#include <Bytes.h>
#include "stm32f1xx_hal.h"
#include "stm32vl_discovery.h"
#ifdef __cplusplus
class Board {
public:
	static void init();
	typedef enum {
		BUTTON_LEFT = 1, BUTTON_RIGHT
	} Button;
	typedef enum {
		LED_BLUE = 1, LED_GREEN, LED_RED
	} Led;


	static bool isButtonPressed(Button button);
	static void setLedOn(int32_t led, bool on);
	static bool getButton(Button button);
	static bool getLed(int32_t led);
	static char* processor();
	static void processorRevision(Bytes& b);
	static float getTemp();
	static void disableInterrupts();
	static void enableInterrupts();
};
#endif
#endif /* BOARD_H_ */

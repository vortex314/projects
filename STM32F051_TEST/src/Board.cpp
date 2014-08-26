/*
 * Board.cpp
 *
 *  Created on: 26-aug.-2014
 *      Author: lieven2
 */

#include "Board.h"
#include "stm32f0xx_conf.h"
#include "stm320518_eval.h"

void Board::init() {
	/* Initialize Leds LD3 and LD4 mounted on STM32VLDISCOVERY board */
	STM_EVAL_LEDInit(LED1);
	STM_EVAL_LEDInit(LED2);
}

void Board::setLedOn(int32_t led, bool on) {
	if (led == LED_GREEN)
	{
		if (on)
			STM_EVAL_LEDOn(LED1);
		else
			STM_EVAL_LEDOff(LED1);
	}
	else if (led == LED_BLUE) {
		if (on)
			STM_EVAL_LEDOn(LED2);
		else
			STM_EVAL_LEDOff(LED2);
	}

}


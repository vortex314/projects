/*
 * Sys.cpp

 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Sys.h"
#include "stm320518_eval.h"
#include "stm32f0xx.h"

Sys::Sys() {
}

void Sys::init() {
	WWDG_DeInit();
	SysTick_Config(SystemCoreClock / 1000);
}
/*
 * Sys.cpp
 *
 *  Created on: 20-aug.-2014
 *      Author: lieven2
 */
#include "Sys.h"
//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************

extern "C" void SysTick_Handler(void) {
	Sys::_upTime++;
}

uint64_t Sys::_upTime=0;

uint64_t Sys::upTime(){
	return Sys::_upTime;
}

/*
 * Sys.cpp
 *
 *  Created on: 20-aug.-2014
 *      Author: lieven2
 */
#include "Sys.h"
#include "Board.h"
//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************

extern "C" void SysTickIntHandler(void) {
	Sys::_upTime++;
}

uint64_t Sys::_upTime=0;

uint64_t Sys::upTime(){
	return Sys::_upTime;
}
uint32_t Sys::_errorCount=0;
void Sys::warn(int err,const char* s){
_errorCount++;
Board::setLedOn(Board::LED_RED, true);
}

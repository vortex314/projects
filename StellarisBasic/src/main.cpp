/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */


#include "Board.h"

uint32_t val=0x100;
volatile double d=0.0;
int main(void) {

	Board::init();	// initialize usb


	while (1) {
		val++;
		d+=1.2;
		}
	}


/*
 * Led.cpp
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Led.h"

#include "Timer.h"

Led::Led(LedColor color)
 {
	_led = color;
	init();
}

Led::~Led() {
	while (1)
		;
}







//____________________________________________ HARDWARE SPECIFIC





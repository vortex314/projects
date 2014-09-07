/*
 * Led.cpp
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Led.h"

#include "Timer.h"

Led::Led(LedColor color) :
		_timer(this) {
	_timer.interval(1000);
	_timer.reload(true);
	_isOn = false;
	_frequency = 5;
	_led = color;
	_state = Led::OFF;
	init();
}

Led::~Led() {
	while (1)
		;
}

void Led::light(bool isOn) {
	_timer.stop();
	if (isOn) {
		_isOn = true;
		on();
	} else {
		_isOn = false;
		off();
	};
}

void Led::blink(int frequency) {
	_frequency = frequency;
	_timer.interval((1000 / frequency) / 2);
	_timer.start();
	_state = Led::BLINK;
	_isOn = true;
	on();
}

void Led::once(int interval) {
	_state = Led::ONCE;
	on();
	_timer.start(interval);
}

Erc Led::event(Event& event) {
	if (event.clsType() == Timer::EXPIRED) {
		if (_state == Led::BLINK) {
			toggle();
		} else if (_state == Led::ONCE) {
			_state = Led::OFF;
			off();
		};
	}
	return E_OK;
}
//____________________________________________ HARDWARE SPECIFIC
#include "stm32f10x_conf.h"
#include "STM32vldiscovery.h"

void Led::init() {
	/* Initialize Leds LD3 and LD4 mounted on STM32VLDISCOVERY board */
	STM32vldiscovery_LEDInit(LED3);
	STM32vldiscovery_LEDInit(LED4);
	_timer.start();
}

void Led::toggle() {
	if (_isOn) {
		_isOn = false;
		off();
	} else {
		_isOn = true;
		on();
	};
}

void Led::on() {
	if (_led == LED_GREEN)
		STM32vldiscovery_LEDOn(LED3);
	if (_led == LED_BLUE)
		STM32vldiscovery_LEDOn(LED4);
}

void Led::off() {
	if (_led == LED_GREEN)
		STM32vldiscovery_LEDOff(LED3);
	if (_led == LED_BLUE)
		STM32vldiscovery_LEDOff(LED4);
}


/*
 * Led.cpp
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Led.h"

#include "Timer.h"

Led::Led(LedColor led) :
		_timer(this) {
	_timer.interval(1000);
	_timer.reload(true);
	_isOn = false;
	_frequency = 5;
	_led = led;
	_state = Led::OFF;
	init();
	off();
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

Erc Led::event(Event* event) {
	if (event->id() == Timer::EXPIRED) {
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

#include <driverlib/rom_map.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
//
// GPIO, Timer, Peripheral, and Pin assignments for the colors
//
#define RED_GPIO_PERIPH         SYSCTL_PERIPH_GPIOF
#define RED_TIMER_PERIPH        SYSCTL_PERIPH_TIMER0
#define BLUE_GPIO_PERIPH        SYSCTL_PERIPH_GPIOF
#define BLUE_TIMER_PERIPH       SYSCTL_PERIPH_TIMER1
#define GREEN_GPIO_PERIPH       SYSCTL_PERIPH_GPIOF
#define GREEN_TIMER_PERIPH      SYSCTL_PERIPH_TIMER1


#define RED_GPIO_BASE           GPIO_PORTF_BASE
#define RED_TIMER_BASE          TIMER0_BASE
#define BLUE_GPIO_BASE          GPIO_PORTF_BASE
#define BLUE_TIMER_BASE         TIMER1_BASE
#define GREEN_GPIO_BASE         GPIO_PORTF_BASE
#define GREEN_TIMER_BASE        TIMER1_BASE

#define RED_GPIO_PIN            GPIO_PIN_1
#define BLUE_GPIO_PIN           GPIO_PIN_2
#define GREEN_GPIO_PIN          GPIO_PIN_3


#define RED_GPIO_PIN_CFG        GPIO_PF1_T0CCP1
#define BLUE_GPIO_PIN_CFG       GPIO_PF2_T1CCP0
#define GREEN_GPIO_PIN_CFG      GPIO_PF3_T1CCP1

void Led::init() {
/*	if (_led == LED_GREEN) {
		SysCtlPeripheralEnable(GREEN_GPIO_PERIPH);
		GPIOPinConfigure(GREEN_GPIO_PIN_CFG);
		GPIOPadConfigSet(GREEN_GPIO_BASE, GREEN_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
				GPIO_PIN_TYPE_STD);
	};

	if (_led == LED_BLUE) {
		SysCtlPeripheralEnable(BLUE_GPIO_PERIPH);
		GPIOPinConfigure(BLUE_GPIO_PIN_CFG);
		GPIOPadConfigSet(BLUE_GPIO_BASE, BLUE_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
				GPIO_PIN_TYPE_STD);
	};
	if (_led == LED_RED) {
		SysCtlPeripheralEnable(RED_GPIO_PERIPH);
		GPIOPinConfigure(RED_GPIO_PIN_CFG);
		GPIOPadConfigSet(RED_GPIO_BASE, RED_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
				GPIO_PIN_TYPE_STD);
	}*/
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
	if (_led == LED_GREEN) {
		GPIOPinWrite(GREEN_GPIO_BASE, GREEN_GPIO_PIN,GREEN_GPIO_PIN );
	};

	if (_led == LED_BLUE) {
		GPIOPinWrite(BLUE_GPIO_BASE, BLUE_GPIO_PIN, BLUE_GPIO_PIN);
	};
	if (_led == LED_RED) {
		GPIOPinWrite(RED_GPIO_BASE, RED_GPIO_PIN, RED_GPIO_PIN);
	}
}

void Led::off() {
	if (_led == LED_GREEN) {
			GPIOPinWrite(GREEN_GPIO_BASE, GREEN_GPIO_PIN,0 );
		};

		if (_led == LED_BLUE) {
			GPIOPinWrite(BLUE_GPIO_BASE, BLUE_GPIO_PIN, 0);
		};
		if (_led == LED_RED) {
			GPIOPinWrite(RED_GPIO_BASE, RED_GPIO_PIN, 0);
		}
}


/*
 * Gpio.cpp
 *
 *  Created on: 17-jul.-2015
 *      Author: lieven2
 */

#include <Gpio.h>

const struct {
	Gpio::Port port;
	uint32_t clock;
	GPIO_TypeDef* base;
} ports[] = { { Gpio::PORT_A, RCC_APB2Periph_GPIOA, GPIOA }, //
		{ Gpio::PORT_B, RCC_APB2Periph_GPIOB, GPIOB }, //
		{ Gpio::PORT_C, RCC_APB2Periph_GPIOC, GPIOC }, //
		{ Gpio::PORT_D, RCC_APB2Periph_GPIOD, GPIOD }, //
		{ Gpio::PORT_E, RCC_APB2Periph_GPIOE, GPIOE } //
};

Gpio::Gpio(Port port, uint32_t pin) {
	_port = port;
	_pin = 1 << pin;
	_mode = INPUT;
}
void Gpio::init(Mode mode) {
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(ports[_port].clock, ENABLE);
	GPIO_InitStructure.GPIO_Pin = _pin;
	if (mode == INPUT) {
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	} else if (mode == OUTPUT_PP) {
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	} else if (mode == OUTPUT_OD) {
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	}
	GPIO_Init(ports[_port].base, &GPIO_InitStructure);
}

void Gpio::write(int i) {
	if (i)
		GPIO_SetBits(ports[_port].base, _pin);
	else
		GPIO_ResetBits(ports[_port].base, _pin);
}

int Gpio::read() {
	return GPIO_ReadInputDataBit(ports[_port].base, _pin);
}

Gpio::~Gpio() {
}


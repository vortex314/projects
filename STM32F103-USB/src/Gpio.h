/*
 * Gpio.h
 *
 *  Created on: 17-jul.-2015
 *      Author: lieven2
 */

#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

class Gpio {

public:
	typedef enum {
		PORT_A, PORT_B, PORT_C, PORT_D, PORT_E
	} Port;
	typedef enum {
		INPUT, OUTPUT_PP, OUTPUT_OD
	} Mode;
	Gpio();
	Gpio(Port port, uint32_t pin);
	virtual ~Gpio();
	void init(Mode mode);
	void write(int v);
	int read();
	void mode(Mode mode);
private:
	Port _port;
	uint32_t _pin;
	Mode _mode;

};

#endif /* SRC_GPIO_H_ */

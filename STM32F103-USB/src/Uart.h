/*
 * Uart.h
 *
 *  Created on: 20-aug.-2015
 *      Author: lieven2
 */

#ifndef UART_H_
#define UART_H_
#include "Link.h"
#include "Msg.h"
#include "Link.h"
#include "Handler.h"
#include "CircBuf.h"
#include <stdint.h>

class Uart {
	uint32_t _idx;
	uint32_t _baudrate;
public:
	Uart(uint32_t idx);
	virtual ~Uart();
	void setBaud(uint32_t baud);
	void init();
//	void free(void* ptr);
	uint8_t read();
	uint32_t hasData();
	Erc send(Bytes&);
	Erc send(const char* s);
//	void toFifo();
	CircBuf _in;
	CircBuf _out;
	uint32_t _crcErrors;
	uint32_t _overrunErrors;

private:
};

#endif /* UART_H_ */

/*
 * Uart.h
 *
 *  Created on: 24-okt.-2014
 *      Author: lieven2
 */

#ifndef UART_H_
#define UART_H_
#include "Link.h"
#include "CircBuf.h"
class Uart: public Link {
public:
	Uart();
	virtual ~Uart();
	static void init();
	uint8_t read();
	uint32_t hasData();
	Erc send(Bytes&);
	Erc connect();
	Erc disconnect();
	CircBuf _in;
	CircBuf _out;
private:

};

#endif /* UART_H_ */

/*
 * Uart.h
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */

#ifndef UART_H_
#define UART_H_
#include "Event.h"
#include "Stream.h"
#include "CircBuf.h"
#include "Timer.h"

class Uart: public Stream {
public:
	enum UartEvents {
		RXD = EVENT('U', 'R'), TXD = EVENT('U', 'T')
	};
	Uart();
	~Uart();
	void init();
	Erc write(Bytes& data);
	Erc event(Event& event);
	uint8_t read();
	bool hasData();
	bool hasSpace();
	CircBuf _rxd;
	CircBuf _txd;
	uint32_t _commErrors;
	uint32_t _bytesRxd;
	uint32_t _bytesTxd;
private:
	void poll();
	Timer _timer;
};

#endif /* UART_H_ */

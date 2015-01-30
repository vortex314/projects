/*
 * Uart.h
 *
 *  Created on: 24-okt.-2014
 *      Author: lieven2
 */

#ifndef UART_H_
#define UART_H_
#include "Msg.h"
#include "Link.h"
#include "Handler.h"
#include "CircBuf.h"
#include "MqttIn.h"
class Uart: public Link, public Handler
{
public:
	Uart();
	virtual ~Uart();
	static void init();
	uint8_t read();
	uint32_t hasData();
	Erc send(Bytes&);
	void toFifo();
	Erc connect();
	Erc disconnect();
	bool isConnected()
	{
		return true;
	}
	void isConnected(bool val)
	{
	}
	bool dispatch(Msg& event);
	CircBuf _in;
	CircBuf _out;
	Bytes _inBuffer;
	uint32_t _crcErrors;
	uint32_t _overrunErrors;

private:

};

#endif /* UART_H_ */

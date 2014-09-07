/*
 * Spi.h
 *
 *  Created on: 26-mei-2014
 *      Author: lieven2
 */

#ifndef SPI_H_
#define SPI_H_

#include "Event.h"
#include "Stream.h"
#include "Bytes.h"

class Spi: public Stream {
public:
	enum {
		RXD, TXD
	} SpiEvent;
	Spi(uint32_t port);
	virtual ~Spi();
	Erc exchange(uint8_t* out, uint8_t* in, uint32_t length);
	Erc write(uint8_t b);
	Erc flush();
	Erc readIn(Bytes& in);
	uint8_t read();
	bool hasData();
	Erc send(Bytes& b);
	Erc setClock(uint32_t hz);
	uint32_t getClock();
	Erc event(Event* event);
	void intHandler(void);
private:
	Bytes _in;
	Bytes _out;
	void* _port;
	uint32_t _clock;
	uint32_t _count;
	uint32_t _readMax;

};

typedef struct {
	Bytes data();
} SpiData;

#endif /* SPI_H_ */

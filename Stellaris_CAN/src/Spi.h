/*
 * Spi.h
 *
 *  Created on: 20-dec.-2013
 *      Author: lieven2
 */

#ifndef SPI_H_
#define SPI_H_
#include <stdint.h>
#include <Stream.h>
#include "Erc.h"
#include "Sys.h"
#include "Bytes.h"

class Spi {
public:
	Spi(int idx);
	virtual ~Spi();
	void init();
	Erc exchange(Bytes& out, Bytes& in );
	Erc exchange(uint32_t out,uint32_t *in);
};

#endif /* SPI_H_ */

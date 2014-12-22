/*
 * I2C.h
 *
 *  Created on: 22-dec.-2014
 *      Author: lieven2
 */

#ifndef I2C_H_
#define I2C_H_
#include "Bytes.h"

class I2C {
private:
	uint32_t _port;
	uint32_t _master_base;

public:
	I2C(uint32_t port);
	virtual ~I2C();
	void init();
	void send(uint8_t addr,Bytes& bytes);
};

#endif /* I2C_H_ */

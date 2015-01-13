/*
 * Tcp.h
 *
 *  Created on: 7-jan.-2015
 *      Author: lieven2
 */

#ifndef TCP_H_
#define TCP_H_
#include "Sys.h"
#include "Bytes.h"
class Tcp
{
public:
	Tcp();
	virtual ~Tcp();
	virtual Erc connect();
	virtual Erc disconnect();
	virtual uint32_t hasData();
	virtual uint8_t read();
	virtual Erc write(Bytes& bytes);
	virtual bool isConnected();
};

#endif /* TCP_H_ */

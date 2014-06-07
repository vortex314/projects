/*
 * Eeprom.h
 *
 *  Created on: 2-nov.-2013
 *      Author: lieven2
 */

#ifndef EEPROM_H_
#define EEPROM_H_
#include <stdint.h>
#include "Erc.h"


class Eeprom {
public:
	Eeprom();
	virtual ~Eeprom();
	static Erc init();
	static Erc erase();
	static Erc setVar(char *name,void* pv,uint32_t size);
	static Erc getVar(char *name,void* pv,uint32_t size);
	static uint8_t *findVar(char *name);
	static Erc write(uint8_t* pDst, uint8_t* pSrc,uint32_t size);
	static Erc read(uint8_t* pDst, uint8_t* pSrc,uint32_t size);
	static uint32_t currentPage;
};




#endif /* EEPROM_H_ */

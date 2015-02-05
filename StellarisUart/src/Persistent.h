/*
 * Persistent.h
 *
 *  Created on: 1-feb.-2015
 *      Author: lieven2
 */

#ifndef PERSISTENT_H_
#define PERSISTENT_H_
#include <stdint.h>



struct {
	char device[32];
	uint8_t ipAddress[4];

} aa;

class Persistent {
public:
	Persistent();
	virtual ~Persistent();
	static bool reset();
	uint32_t readWord(uint16_t offset);
	uint16_t find(const char*s );
	uint16_t next(uint16_t offset);
	bool put(const char* s, void* start, uint16_t length,uint16_t maxLength);
	bool get(const char* s, void* start, uint16_t& length,uint16_t maxLength);
};

#endif /* PERSISTENT_H_ */

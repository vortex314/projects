/*
 * Persistent.h
 *
 *  Created on: 1-feb.-2015
 *      Author: lieven2
 */

#ifndef PERSISTENT_H_
#define PERSISTENT_H_
#include <stdint.h>

class Persistent {
public:
	Persistent();
	virtual ~Persistent();
	bool store(const char* s, void* start, uint32_t length);
	bool restore(const char* s, void* start, uint32_t length);
};

#endif /* PERSISTENT_H_ */

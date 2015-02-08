/*
 * Persistent.h
 *
 *  Created on: 1-feb.-2015
 *      Author: lieven2
 */

#ifndef PERSISTENT_H_
#define PERSISTENT_H_
#include <stdint.h>

	enum PERS_ID {
		PERS_0,PERS_MQTT_PREFIX, PERS_MQTT_HOST, PERS_MQTT_PORT
	} ;

class Persistent {
private:
	uint8_t* _flashStart;
	uint8_t* _flashEnd;
	uint8_t* _pageActive;
public:

	Persistent();
	virtual ~Persistent();
	static bool reset();

	bool put(uint8_t index, uint8_t* data, uint8_t length);
	bool get(uint8_t index, uint8_t* data, uint8_t& length);
	bool isPageActive(uint8_t* ptr);
	bool setPageActive(uint8_t* ptr);
	bool findPageActive();
	bool erasePage(uint8_t* ptr);
	bool findTag(uint8_t tag, uint8_t** offset);
	bool freeSpace(uint8_t** addr) ;
};

#endif /* PERSISTENT_H_ */

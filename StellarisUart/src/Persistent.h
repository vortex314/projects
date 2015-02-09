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
	PERS_0, PERS_MQTT_PREFIX, PERS_MQTT_HOST, PERS_MQTT_PORT
};

class Persistent {
private:
	uint8_t* _flashStart;
	uint8_t* _flashEnd;
	uint8_t* _pageActive;
	uint32_t _sequence;
public:

	Persistent();
	virtual ~Persistent();
	static bool reset();

	bool put(uint8_t index, uint8_t* data, uint8_t length);
	bool get(uint8_t index, uint8_t* data, uint8_t& length);
	bool isPageActive(uint8_t* ptr);
	bool setPageActive(uint8_t* ptr);
	bool findPageActive();
	void findActivePage();
	bool erasePage(uint8_t* ptr);
	bool findTag(uint8_t tag, uint8_t** offset);
	bool freeSpace(uint8_t** addr);

	void pageInit(uint8_t* page, uint32_t sequence);
	void pageErase(uint8_t* page);
	bool pageFindTag(uint8_t* page, uint8_t tag, uint8_t** address);
	bool pageGetTag(uint8_t* page, uint8_t index, uint8_t** data,
			uint8_t* length);
	uint8_t* pageNext(uint8_t* page);
	void pageCopy(uint8_t* dst, uint8_t* src);
	uint8_t* pageFreeBegin(uint8_t* page);
};

#endif /* PERSISTENT_H_ */

/*
 * Persistent.cpp
 *
 *  Created on: 1-feb.-2015
 *      Author: lieven2
 */

#include <Persistent.h>
#define TARGET_IS_DUSTDEVIL_RA0
#include <driverlib/rom_map.h>
#include <driverlib/rom.h>
#include <cstring>
#include <string.h>
#include "inc/hw_types.h"
#include "inc/hw_flash.h"
#include "driverlib/sysctl.h"
#include "driverlib/flash.h"
#include "driverlib/debug.h"

// <max_length(4)><real_length(4)><name><max_length><length><data>

#define ROUNDUP4(xxx) ((xxx + 3 ) & 0xFFFC)

typedef struct {
	uint8_t free;
	uint8_t allocated;
	uint8_t used;
	uint8_t tag;
} Header;

bool Persistent::isPageActive(uint8_t* ptr) {
	return (*ptr == 0 && *(ptr + 1) == 0xFFFFFFFF);
}
bool Persistent::setPageActive(uint8_t* ptr) {
	uint32_t data = 0;
	if (FlashProgram(&data, (uint32_t) ptr, 4))
		return false;
	_pageActive = ptr;
	return true;
}

bool Persistent::findPageActive(){
	uint8_t* ptr;
		for (ptr = _flashStart; ptr < _flashEnd; ptr += 0x400) {
			if (isPageActive(ptr)) {
				_pageActive = ptr;
				break;
			}
		}
		if (ptr == _flashEnd) {
			setPageActive(_flashStart);
		}
		return true;
}



Persistent::Persistent() {
	_flashStart = (uint8_t*) 0x30000; // after 3 x 64KB see linker script
	_flashEnd = (uint8_t*) 0x31000; // 0x1000 = 4 KB , 4 pages of 1K
	_pageActive = _flashStart;
	findPageActive();
}

Persistent::~Persistent() {
}

bool Persistent::put(uint8_t index, uint8_t* data, uint8_t length) {
// find address
	uint8_t* address = 0;
	if ( !freeSpace(&address)) return false;
	Header hdr = { 0xFF, ROUNDUP4(length), length, index };
	if (FlashProgram((uint32_t*) &hdr, (uint32_t)address, 4))
		return false;
	union {
		uint32_t i32;
		uint8_t b[4];
	} v;
	int l;
	for (int i = 0; i < ROUNDUP4(length); i += 4) {
		if (i < length - 4) {
			l = 4;
		} else
			l = length - i;
		memcpy(v.b, data + i, l);
		FlashProgram(&v.i32, (uint32_t)(address + 4 + i), 4);
	}
	return true;
}

bool Persistent::get(uint8_t tag, uint8_t* data, uint8_t& length) {
// find address
	uint8_t* address = 0;
	if (!findTag(tag, &address))
		return false;

	Header* pHdr = (Header*) address;
	if (pHdr->used > length)
		return false;
	memcpy(data, (uint8_t*) address + 4, pHdr->used);
	length = pHdr->used;
	return true;
}




bool Persistent::findTag(uint8_t tag, uint8_t** addr) {
	uint8_t* ptr = _pageActive + 8;
	uint8_t* tagAddress = 0;
	while (ptr < _pageActive + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*)ptr == 0xFFFFFFFF) {
			if (tagAddress == 0) {
				*addr =  ptr;
				return false;;
			} else {
				*addr = tagAddress;
				return true;
			}
		}
		if (pHdr->tag == tag) {
			tagAddress = ptr;
		}
		ptr += pHdr->allocated +4 ;
		if (pHdr->allocated == 0)
			break;
	}
	return false;
}


bool Persistent::freeSpace(uint8_t** addr) {
	uint8_t* ptr = _pageActive + 8;
	while (ptr < _pageActive + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*)ptr == 0xFFFFFFFF) {
			*addr =  ptr;
			return true;
		}
		ptr += pHdr->allocated ;
		if (pHdr->allocated == 0)
			break;
	}
	return false;
}

bool Persistent::erasePage(uint8_t* ptr) {
	return FlashErase((uint32_t) ptr) == 0;
}



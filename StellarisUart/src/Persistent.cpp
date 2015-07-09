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
/*

 PageHeader : <sequence><FFFFFFFF>
 highets sequence is active page
 4 x 1 Kb page
 <00><FF>
 <lengthRound4><realLength><id> <data4>
 < 0xFF>< 8 bit >< 8 bit >< 8 bit >

 */
// <max_length(4)><real_length(4)><name><max_length><length><data>
#define ROUNDUP4(xxx) ((xxx + 3 ) & 0xFFFC)

typedef struct {
	uint8_t free;
	uint8_t allocated;
	uint8_t used;
	uint8_t tag;
} Header;

void Persistent::pageInit(uint8_t* address, uint32_t sequence) {
	FlashErase((uint32_t) address);
	FlashProgram(&sequence, (uint32_t) address, 4);
}

void Persistent::pageErase(uint8_t* address) {
	FlashErase((uint32_t) address);
}

bool Persistent::pageFindTag(uint8_t* address, uint8_t tag, uint8_t** addr) {
	uint8_t* ptr = address + 8;
	uint8_t* tagAddress = 0;
	while (ptr < address + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*) ptr == 0xFFFFFFFF) {
			if (tagAddress == 0) {
				*addr = ptr;
				return false;;
			} else {
				*addr = tagAddress;
				return true;
			}
		}
		if (pHdr->tag == tag) {
			tagAddress = ptr;
		}
		ptr += pHdr->allocated + 4;
		if (pHdr->allocated == 0)
			break;
	}
	return false;
}

bool Persistent::pageGetTag(uint8_t* page, uint8_t tag, uint8_t** start,
		uint8_t* length) {
	uint8_t* addr = 0;

	if (!pageFindTag(page, tag, &addr))
		return false;

	Header* pHdr = (Header*) addr;
	if (pHdr->used > *length)
		return false;

	memcpy(*start, (uint8_t*) addr + 4, pHdr->used);
	*length = pHdr->used;
	return true;
}

uint32_t Persistent::pageSpaceLeft(uint8_t* page) {
	uint8_t* addr = pageFreeBegin(page);
	return page + 0x400 - addr;
}

uint32_t Persistent::pageSequence(uint8_t* page) {
	return *(uint32_t*) page;
}

bool Persistent::pageIsUsed(uint8_t* page) {
	return (*(uint32_t*) page != 0xFFFFFFFF
			&& *(uint32_t*) (page + 4) == 0xFFFFFFFF);
}

uint8_t* Persistent::pageFreeBegin(uint8_t* page) {
	uint8_t* addr;
	uint8_t* ptr = page + 8;
	while (ptr < page + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*) ptr == 0xFFFFFFFF) {
			addr = ptr;
			return addr;
		}
		ptr += pHdr->allocated;
		if (pHdr->allocated == 0)
			break;
	}
	return 0;
}

void Persistent::pageFindActive() {
	uint8_t* page;
	uint32_t* activePage = 0;
	uint32_t sequence=0;
	for (page = _flashStart; page < _flashEnd; page += 0x400) {
		if (pageIsUsed(page))
			if (*(uint32_t*) page != 0xFFFFFFFF
					&& *(uint32_t*) (page + 4) == 0xFFFFFFFF)
				if (*page >= sequence) {
					activePage = (uint32_t*)page;
					sequence = *page;
				}
	}
	if (activePage) {
		_pageActive = (uint8_t*)activePage;
		_sequence = sequence;
	} else {
		_pageActive = _flashStart;
		_sequence = 0;
		pageInit(_pageActive, _sequence);
	}

}

uint8_t* Persistent::pageNext(uint8_t * activePage) {
	uint8_t* page = activePage + 0x400;
	if (page >= _flashEnd)
		page = _flashStart;
	return page;
}

void Persistent::pageCopy(uint8_t* dst, uint8_t* src) {
	uint8_t* tagAddress;
	pageInit(dst, _sequence + 1);
	for (uint8_t tag = 0; tag < 10; tag++) {
		if (pageFindTag(src, tag, &tagAddress))
			pagePutTag(dst, tag, tagAddress + 4, ((Header*)tagAddress)->used);
	}
}

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

Persistent::Persistent() {
	_flashStart = (uint8_t*) 0x30000; // after 3 x 64KB see linker script
	_flashEnd = (uint8_t*) 0x31000; // 0x1000 = 4 KB , 4 pages of 1K
	_pageActive = _flashStart;
	pageFindActive();
}

Persistent::~Persistent() {
}

bool Persistent::put(uint8_t tag, uint8_t* data, uint8_t length) {
// find address
	if (pageSpaceLeft(_pageActive) < (ROUNDUP4(length) + 4)) {
		pageCopy(pageNext(_pageActive), _pageActive);
		_pageActive = pageNext(_pageActive);
	}
	return pagePutTag(_pageActive, tag, data, length);
}

bool Persistent::pagePutTag(uint8_t* page, uint8_t tag, uint8_t* data, uint8_t length) {
	uint8_t* address = pageFreeBegin(page);
	Header hdr = { 0xFF, ROUNDUP4(length), length, tag };
	if (FlashProgram((uint32_t*) &hdr, (uint32_t) address, 4))
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
		FlashProgram(&v.i32, (uint32_t) (address + 4 + i), 4);
	}
	return true;
}

bool Persistent::get(uint8_t tag, uint8_t* data, uint8_t& length) {
// find address
	uint8_t* address = 0;
	if (!pageFindTag(_pageActive, tag, &address))
		return false;

	Header* pHdr = (Header*) address;
	if (pHdr->used > length)
		return false;
	memcpy(data, (uint8_t*) address + 4, pHdr->used);
	length = pHdr->used;
	return true;
}


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
	if (pHdr->used > length)
		return false;

	memcpy(*start, (uint8_t*) addr + 4, pHdr->used);
	length = pHdr->used;
	return true;
}

uint32_t pageSpaceLeft(uint8_t* page) {
	uint8_t* addr = pageFreeBegin(page);
	return page + 0x400 - addr;
}

uint32_t pageSequence(uint8_t* page) {
	return *(uint32_t*) page;
}

bool pageIsUsed(uint8_t* page) {
	return (*(uint32_t*) page != 0xFFFFFFFF
			&& *(uint32_t*) (page + 4) == 0xFFFFFFFF);
}

uint8_t* pageFreeBegin(uint8_t* page) {
	uint8_t* addr;
	uint8_t* ptr = page + 8;
	while (ptr < page + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*) ptr == 0xFFFFFFFF) {
			*addr = ptr;
			return addr;
		}
		ptr += pHdr->allocated;
		if (pHdr->allocated == 0)
			break;
	}
	return 0;
}

void Persistent::findActivePage() {
	uint8_t* page;
	uint32_t* activePage = 0;
	uint32_t sequence;
	for (page = _flashStart; page < _flashEnd; page += 0x100) {
		if (pageIsUsed(page))
			if (*(uint32_t*) page != 0xFFFFFFFF
					&& *(uint32_t*) (page + 4) == 0xFFFFFFFF)
				if (*page > sequence) {
					activePage = page;
					sequence = *page;
				}
	}
	if (activePage) {
		_pageActive = activePage;
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
}

void Persistent::pageCopy(uint8_t* dst, uint8_t* src) {
	uint8_t* page = pageNext(src);
	uint8_t* tagAddress;
	uint8_t length;
	pageInit(dst, _sequence + 1);
	for (uint8_t tag = 0; tag < 10; tag++) {
		if (pageFindTag(src, tag, tagAddress))
			pagePutTag(page, tag);
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

bool Persistent::findPageActive() {
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
	if (!freeSpace(&address))
		return false;
	if (address + 4 + ROUNDUP4(length) >= _activePage + 0x400) {
		switchPage(&address);
	}
	Header hdr = { 0xFF, ROUNDUP4(length), length, index };
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
	if (!pageFindTag(_pageActive,tag, &address))
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

bool Persistent::freeSpace(uint8_t** addr) {
	uint8_t* ptr = _pageActive + 8;
	while (ptr < _pageActive + 0x400) {
		Header* pHdr = (Header*) ptr;
		if (*(uint32_t*) ptr == 0xFFFFFFFF) {
			*addr = ptr;
			return true;
		}
		ptr += pHdr->allocated;
		if (pHdr->allocated == 0)
			break;
	}
	return false;
}

bool Persistent::erasePage(uint8_t* ptr) {
	return FlashErase((uint32_t) ptr) == 0;
}

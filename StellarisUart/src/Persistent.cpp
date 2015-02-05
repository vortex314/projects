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
#include <inc/hw_types.h>
#include <driverlib/eeprom.h>
#include <driverlib/sysctl.h>

// <max_length(4)><real_length(4)><name><max_length><length><data>

void writeData(uint32_t offset, uint32_t* data, uint32_t length) {
	uint32_t roundLength = ((length + 3) >> 2) << 2;
	EEPROMProgram((uint32_t*) data, offset, roundLength);
}

Persistent::Persistent() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
}

Persistent::~Persistent() {

}

bool Persistent::getByte(uint8_t* start, uint32_t offset) {
	static uint32_t lastOffset = 2000000;
	static uint32_t lastWord;
	uint32_t value;
	uint32_t _offset = (offset << 2) >> 2;
	if (!(lastOffset == _offset)) {
		EEPROMRead(&lastWord, _offset, 1);
	};
	value = lastWord;
	uint32_t byteIndex = offset % 4;
	while (byteIndex < 3) {
		value >>= 8;
		byteIndex++;
	}
	*start = value & 0xFF;
	return false;
}

uint32_t Persistent::readWord(uint16_t offset) {
	uint32_t word;
	EEPROMRead(&word, offset, 4);
	return word;
}

uint16_t Persistent::next(uint16_t offset) {
	return offset + (readWord(offset) >> 16);
}
bool Persistent::put(const char* s, void* start, uint16_t length,
		uint16_t maxLength) {
	char name[40];
	uint32_t word;
	if ( (maxLength & 0xFFFC)!=maxLength) return false;
	if ( length > maxLength ) return false;
	word = ( maxLength << 16 ) + length;
	uint16_t offset=0;
	EEPROMProgram(&word,offset,4);
	for(uint16_t pos;pos < maxLength;pos += 4) {
		memcpy(&word,s+pos,4);
		EEPROMProgram(&word,offset+pos,4);
	}

	for (uint16_t offset = 0; offset != 0xFFFF; offset = next(offset)) {
		uint32_t word = readWord(offset);
		uint16_t maxLength = word >> 16;
		uint16_t length = word & 0xFFFF;
		uint16_t start = offset + 4;
		for (int i = 0; i < length + 3; i += 4) {
			EEPROMRead(&word, start+i, 4);
			memcpy(name+i,&word,4);
		}
	}
	return false;
}
bool Persistent::get(const char* s, void* start, uint16_t& length,
		uint16_t maxLength) {
	char name[40];
		for (uint16_t offset = 0; offset != 0xFFFF; offset = next(offset)) {
			uint32_t word = readWord(offset);
			uint16_t maxLength = word >> 16;
			uint16_t length = word & 0xFFFF;
			uint16_t start = offset + 4;
			for (int i = 0; i < length + 3; i += 4) {
				EEPROMRead(&word, start+i, 4);
				memcpy(name+i,&word,4);
			}
		}
	return false;
}

bool Persistent::reset() {
	EEPROMMassErase();
}

/*
 * ByteBuffer.cpp
 *
 *  Created on: 24-aug.-2012
 *      Author: lieven
 */
#include <malloc.h>
#include "Bytes.h"
#ifdef __linux__
#include <stdlib.h>
#else
//#include "new.h"
#endif
void myMemcpy(uint8_t *dst, uint8_t* src, int length) {
	for (int i = 0; i < length; i++)
		dst[i] = src[i];
}

Bytes::Bytes() {
	_start = 0;
	_offset = 0;
	_limit = 0;
	_capacity = 0;
	isMemoryOwner = false;
}

Bytes::Bytes(uint8_t *st, uint32_t length) {
	_start = st;
	_offset = 0;
	_limit = length;
	_capacity = length;
	isMemoryOwner = false;
}

void Bytes::map(uint8_t* st, uint32_t length) {
	_start = st;
	_offset = 0;
	_limit = length;
	_capacity = length;
	isMemoryOwner = false;
}

/* Bytes::Bytes() {
 _start = (uint8_t*) 0;
 _offset = 0;
 _limit = 0;
 _capacity = 0;
 isMemoryOwner = false;
 }*/

Bytes::Bytes(uint32_t size) {
	_start = 0;
	if (size > 0) {
		_start = (uint8_t*)malloc(size); // (uint8_t*) Sys::malloc(size);
		ASSERT(_start != 0);
	}
	_offset = 0;
	_limit = 0;
	_capacity = size;
	isMemoryOwner = true;
}

Bytes::~Bytes() {
	if (isMemoryOwner)
		if ( _start)
		free( _start);
}
Bytes::Bytes(Bytes& src) {
	_start = (uint8_t*)malloc(src._capacity);
	_offset = 0;
	_limit = src._limit;
	_capacity = src._capacity;
	myMemcpy(_start, src._start, src._limit);
	isMemoryOwner = true;
}

void Bytes::clone(Bytes& src) {
	myMemcpy(_start, src._start, _capacity);
	_offset = 0;
	_limit = (_capacity > src._limit) ? src._limit : _capacity;
}

void Bytes::sub(Bytes* parent, uint32_t length) {
	_start = parent->_start + parent->_offset;
	_offset = 0;
	if (length <= (parent->_capacity - parent->_offset))
		_limit = length;
	else
		_limit = parent->_capacity - parent->_offset;
	_capacity = _limit;
	isMemoryOwner = false;
}

Bytes& Bytes::append(Bytes& b) {
	b.offset(0);
	while (b.hasData())
		write(b.read());
	return *this;
}

Bytes& Bytes::append(const char s[]) {
	while (*s != '\0') {
		write((uint8_t) (*s));
		s++;
	}
	return *this;
}

Bytes& Bytes::operator=(Bytes& s) {
	clear();
	return append(s);
}

Bytes& Bytes::operator=(const char* s) {
	clear();
	return append(s);
}



void Bytes::move(int32_t dist) {
	if ((_offset + dist) > _limit)
		_offset = _limit;
	else
		_offset += dist;
}

/* ByteBuffer::ByteBuffer(ByteBuffer& in) {
 start = in.start;
 _offset = 0;
 _limit = in._limit;
 _capacity = in._capacity;
 isMemoryOwner = false;
 }*/

uint8_t* Bytes::data() const {
	return _start;
}

int Bytes::capacity() {
	return _capacity;
}

uint32_t Bytes::length() const {
	return _limit;
}

int Bytes::offset(int32_t pos) {
	if (pos < 0)
		_offset = _limit;
	else if ((uint32_t) pos < _capacity)
		_offset = pos;
	return 0;
}

Erc Bytes::insert(uint32_t offset, Bytes* data) {
	if (data->_limit + _limit > _capacity)
		return E_LACK_RESOURCE;
	if (offset > _limit)
		return E_INVAL;
	// move last part further
	uint32_t delta = data->length();
	uint32_t i;
	for (i = _limit; i >= offset; i--)
		_start[i + delta] = _start[i];
	// insert data
	for (i = 0; i < delta; i++)
		_start[offset + i] = data->_start[i];
	return E_OK;
}

int Bytes::offset() {
	return _offset;
}

int Bytes::length(int l) {
	_offset = 0;
	_limit = l;
	return 0;
}

int Bytes::available() {
	if (_offset < _limit)
		return _limit - _offset;
	else
		return 0;
}

Erc Bytes::write(uint8_t value) {
	if (_offset < _capacity) {
		_start[_offset++] = value;
		_limit = _offset;
	} else
		return E_LACK_RESOURCE;
	return 0;
}

Erc Bytes::write(uint8_t* data, int offset, int length) {
	for (int i = 0; i < length; i++) {
		int erc;
		erc = write(data[offset + i]);
		if (erc)
			return erc;
	}
	return 0;
}

Erc Bytes::write(Bytes* data) {
	return write(data->_start, 0, data->_limit);
}

Erc Bytes::read(uint8_t* data) {
	if (_offset < _limit)
		*data = _start[_offset++];
	else
		return E_AGAIN;
	return 0;
}

uint8_t Bytes::read() {
	if (_offset < _limit)
		return _start[_offset++];
	return '-';
}

void Bytes::clear() {
	_offset = 0;
	_limit = 0;
}

bool Bytes::equals(const uint8_t* pb,uint32_t length){
	if ( length != _limit ) return false;
	for(uint32_t i=0;i<length;i++){
		if ( _start[i] != pb[i]) return false;
	}
	return true;
}

//PUBLIC
//_________________________________________________________________________

void Bytes::AddCrc() //PUBLIC
//_________________________________________________________________________
{
	offset(-1); // position at end
	uint16_t crc = Fletcher16(_start, _limit);
	write(crc & 0xFF);
	write(crc >> 8);
}

uint16_t Bytes::Fletcher16(uint8_t *begin, int length) {
	uint16_t sum1 = 0;
	uint16_t sum2 = 0;
	uint8_t *end = begin + length;
	uint8_t *index;

	for (index = begin; index < end; index++) {
		sum1 = (sum1 + *index) % 255;
		sum2 = (sum2 + sum1) % 255;
	}

	return (sum2 << 8) | sum1;
}
//_________________________________________________________________________

bool Bytes::isGoodCrc() //PUBLIC
//_________________________________________________________________________
{
	if (_limit < 3)
		return false; // need at least 3 bytes
	uint16_t crc = Fletcher16(_start, _limit - 2);
	if ((*(_start + _limit - 2) == (crc & 0xFF))
			&& ((*(_start + _limit - 1) == (crc >> 8))))
		return true;
	return false;
}

//_________________________________________________________________________

void Bytes::RemoveCrc() //PUBLIC
//_________________________________________________________________________
{
	_limit -= 2;
}
//PUBLIC

//PUBLIC
//_________________________________________________________________________

void Bytes::Decode() //PUBLIC
//_________________________________________________________________________
{
	uint8_t *p, *q;
	uint8_t *_capacity = _start + _limit;
	for (p = _start; p < _capacity; p++) {
		if (*p == ESC) {
			*p = (uint8_t) (*(p + 1) ^ 0x20);
			for (q = p + 1; q < _capacity; q++)
				*q = *(q + 1);
			_capacity--;
			//		p++; // skip next uint8_t could also be an escape
		}
	}
	_limit = _capacity - _start;
}

//_________________________________________________________________________

void Bytes::Encode() //PUBLIC
//_________________________________________________________________________
{
	uint8_t *p, *q;
	uint8_t *_capacity = _start + _limit;
	for (p = _start; p < _capacity; p++) {
		if ((*p == SOF) || (*p == ESC)) {
			for (q = _capacity; q > p; q--)
				*(q + 1) = *q;
			_capacity++;
			*(p + 1) = *p ^ 0x20;
			*p = ESC;
		}
	}
	_limit = _capacity - _start;
}

//_________________________________________________________________________

void Bytes::Frame() //PUBLIC
//_________________________________________________________________________
{
	uint8_t *q;
	uint8_t *end = _start + _limit;
	for (q = end; q >= _start; q--)
		*(q + 1) = *q;
	*_start = SOF;
	*(end + 1) = SOF;
	end += 2;
	_limit = end - _start;
}

bool Bytes::Feed(uint8_t b) {
	if (b == SOF) {
		if (_offset > 0)
			return true;
		else {
			return false;  // don't add SOF
		}
	} else
		write(b);
	return false;
}

int Bytes::poke(uint32_t idx, uint8_t b) {
	if (idx > _limit)
		return E_LACK_RESOURCE;
	_start[idx] = b;
	return 0;
}

int Bytes::peek() {
	return _start[_offset];
}

int Bytes::peek(int32_t offset) {
	return _start[offset];
}

bool Bytes::hasData() {
	return _offset < _limit;
}

bool Bytes::hasSpace() {
	return _limit < _capacity;
}
const char *HEX = "0123456789ABCDEF";
#include "Str.h"
void Bytes::toString(Str& str) {
	uint32_t i;
	uint8_t b;
	for (i = 0; i < _limit; i++) {
		b = *(_start + i);
		str.append(HEX[b >> 4]);
		str.append(HEX[b & 0xF]);
		str.append(' ');
	}
}
#define TEST
#ifdef TEST
bool testBytes() {
	Bytes bytes(100);

	for (int i = 0; i < 254; i++) {
		bytes.clear();
		bytes.write((uint8_t) i);
		bytes.write((uint8_t) i + 1);
		bytes.write((uint8_t) i + 2);
		bytes.Encode();
		bytes.AddCrc();
		if (bytes.isGoodCrc()) {
			bytes.RemoveCrc();
			bytes.Decode();
			bytes.offset(0);
			if (i != bytes.read())
				return false;
			if ((i + 1) != bytes.read())
				return false;
			if ((i + 2) != bytes.read())
				return false;
		} else
			return false;
	}
	return true;
}
#endif


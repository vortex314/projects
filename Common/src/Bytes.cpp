/*
 * ByteBuffer.cpp
 *
 *  Created on: 24-aug.-2012
 *      Author: lieven
 */

#include "Bytes.h"

Bytes::Bytes(uint8_t *st, uint32_t length) {
    _start = st;
    _offset = 0;
    _limit = 0;
    _capacity = length;
    isMemoryOwner = false;
}

Bytes::Bytes() {
    _start = (uint8_t*) 0;
    _offset = 0;
    _limit = 0;
    _capacity = 0;
    isMemoryOwner = false;
}

Bytes::Bytes(uint32_t size) {
    _start = (uint8_t*) Sys::malloc(size);
    _offset = 0;
    _limit = 0;
    _capacity = size;
    isMemoryOwner = true;
}

Bytes::Bytes(Bytes& parent) {
    _start = parent._start + parent._limit;
    _offset = 0;
    _limit = 0;
    _capacity = parent._capacity - parent._limit;
    isMemoryOwner = false;
}

void Bytes::sub(Bytes* parent, uint32_t length) {
    _start = parent->_start + parent->_offset;
    _offset = 0;
    if (length <= (parent->_capacity - parent->_offset)) _limit = length;
    else _limit = parent->_capacity - parent->_offset;
    _capacity = _limit;
    isMemoryOwner = false;
}

Bytes::~Bytes() {
    if (isMemoryOwner)
        Sys::free(_start);
}

void Bytes::move(int32_t dist) {
    if ((_offset + dist) > _limit) _offset = _limit;
    else _offset += dist;
}

/* ByteBuffer::ByteBuffer(ByteBuffer& in) {
 start = in.start;
 _offset = 0;
 _limit = in._limit;
 _capacity = in._capacity;
 isMemoryOwner = false;
 }*/

uint8_t* Bytes::data() {
    return _start;
}

int Bytes::capacity() {
    return _capacity;
}

void myMemcpy(uint8_t *dst, uint8_t* src, int length) {
    for (int i = 0; i < length; i++)
        dst[i] = src[i];
}

int Bytes::length() {
    return _limit;
}

int Bytes::offset(uint32_t pos) {
    if (pos < 0)
        _offset = _limit;
    else if (pos < _capacity)
        _offset = pos;
    return 0;
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

int Bytes::poke(uint32_t idx, uint8_t b) {
    if (idx > _limit)
        return E_LACK_RESOURCE;
    _start[idx] = b;
    return 0;
}

int Bytes::peek() {
    return _start[_offset];
}

bool Bytes::hasData() {
    return _offset < _limit;
}

bool Bytes::hasSpace() {
    return _limit < _capacity;
}

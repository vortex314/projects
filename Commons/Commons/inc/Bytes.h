/*
 * ByteBuffer.h
 *
 *  Created on: 24-aug.-2012
 *      Author: lieven
 */

#ifndef BYTES_H_
#define BYTES_H_
//#include "base.h"
#include <stdint.h>

#include "Erc.h"
#include "Sys.h"

#define MAX_BUFFER_SIZE 512
#define SOF  0x7E
#define ESC  0x7D

class Bytes {
public:
    Bytes();
    Bytes(uint32_t size);
    Bytes(uint8_t* start, uint32_t size);
    Bytes(Bytes& in);
    ~Bytes();
    void map(uint8_t *start, uint32_t size);
    void sub(Bytes* parent,uint32_t length);

    int capacity();
    int length();
    int length(int l);
    int available();
    int offset(uint32_t offset);
    int offset();
    void move(int32_t distance);
    uint8_t *data();

    int poke(uint32_t offset, uint8_t b);
    int peek();

    bool hasData();
    bool hasSpace();

    uint8_t read();

    Erc read(uint8_t* data);
    Erc write(uint8_t value);
    Erc write(uint8_t* data, int offset, int length);
    void clear();

    void AddCrc();
    void RemoveCrc();
    uint16_t Fletcher16(uint8_t *begin, int length);
    void Encode();
    void Decode();
    bool isGoodCrc();
    void Frame();
public:
    uint8_t *_start;
    uint32_t _limit;
    uint32_t _offset;
    uint32_t _capacity;
    bool isMemoryOwner;
};


#endif /* BYTEBUFFER_H_ */

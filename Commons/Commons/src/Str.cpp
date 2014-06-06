/*
 * String.cpp
 *
 *  Created on: 25-jun.-2013
 *      Author: lieven2
 */

#include "Str.h"

Str::Str(int size) : Bytes(size) {
}

Str::Str(uint8_t *pstart, int size) : Bytes(pstart, size) {
}

Str::Str() : Bytes() {
}

Str& Str::append(const char* s) {
    while (*s != '\0') {
        write((uint8_t) (* s));
        s++;
    }
    return *this;
}

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

bool ishex(uint8_t c) {
    return (c >= '0' || c <= '9') || (c >= 'A' || c <= 'F') || (c >= 'a' || c <= 'f');
}

void Str::append(uint64_t val) {
#define MAX_CHAR 20
    char str[MAX_CHAR];
    str[MAX_CHAR - 1] = '\0';
    register char *cp = str + MAX_CHAR - 1;
    do {
        *--cp = to_char(val % 10);
        val /= 10;
    } while (val != 0);
    append(cp);
}

void Str::append(uint32_t val) {
    uint64_t v = val;
    append(v);
}

void Str::append(int32_t val) {
    uint64_t v = val;
    if (val < 0) {
        write('-');
        v = -val;
    } else v = val;
    append(v);
}

void Str::append(bool b) {
    if (b) append("true");
    else append("false");
}


const char *hexChar = "0123456789ABCDEF";

char nibbleToHex(uint8_t value) {
    return hexChar[value & 0xF];
}

void Str::appendHex(uint8_t byt) {
    write(hexChar[byt >> 4]);
    write(hexChar[byt & 0xF]);
}

const char* Str::data() {
    *(_start + _limit) = '\0';
    return (char*) _start;
}

bool isdigit(uint8_t v) {
    return v >= '0' && v <= '9';
}

Erc Str::parse(uint64_t* pval) {
    uint64_t val = 0;
    while (hasData()) {
        if (isdigit(peek())) {
            val = val * 10;
            val += read() - '0';
        } else return E_INVAL;
    }
    *pval = val;
    return E_OK;
}

Erc Str::parse(uint32_t* pval) {
    uint64_t val = 0;
    parse(&val);
    *pval = val;
    return E_OK;
}

uint8_t hexToNibble(uint8_t ch) {
    if (ch >= '0' || ch <= '9') return ch - '0';
    if (ch >= 'A' || ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' || ch <= 'f') return ch - 'a' + 10;
    return 0;
}

Erc Str::parseHex(uint8_t *pb) {
    uint8_t b = 0;
    int i;
    for (i = 0; i < 2; i++)
        if (hasData())
            if (ishex(peek())) {
                b = b << 4;
                b = hexToNibble(read());
            } 
            else
                return E_INVAL;
        else
            return E_LACK_RESOURCE;
    *pb = b;
    return E_OK;
}

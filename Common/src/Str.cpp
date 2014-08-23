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
#include <string.h>
Str::Str(const char* s) : Bytes((uint8_t*)s,strlen(s)) {
    }

Str& Str::set(const char* const s) {
    Bytes::clear();
    append(s);
    return *this;
    }

Str& Str::clear() {
    Bytes::clear();
    return *this;
    }


bool Str::equals(const char* s) {
    uint32_t i;
    uint32_t slen = strlen(s);
    if ( slen != length() ) return false;
    for(i=0; i<slen && i < length(); i++)
        if ( s[i]!= peek(i)) return false;
    return true;
    }

bool Str::endsWith(const char* s) {
    int sl=strlen(s);
    int offset=length()-sl;
    if ( offset < 0 ) return false;
    int i;
    for(i=0; i<sl; i++)
        if ( s[i]!=peek(i+offset)) return false;
    return true;
    }

bool Str::startsWith(Str& s) {
    if ( s.length() > length()) return false;
    uint32_t i;
    for(i=0; i<s.length(); i++)
        if (s.peek(i) !=peek(i)) return false;
    return true;
    }

bool Str::startsWith(const char* const s) {
    Str ss(s);
    return startsWith(ss);
    /*   if ( s.length() > length()) return false;
       int i;
       for(i=0; i<s.length(); i++)
           if (s.peek(i) !=peek(i)) return false;
       return true;*/
    }

Str& Str::operator=(const char* s) {
    clear();
    append(s);
    return *this;
    }

Str& Str::operator=(Str& s) {
    clear();
    append(s);
    return *this;
    }

Str& Str::operator<<(const char* s) {
    while (*s != '\0') {
        write((uint8_t) (* s));
        s++;
        }
    return *this;
    }

    Str& Str::operator<<( char ch) {
        write(ch);
    return *this;
    }

    Str& Str::operator<<( int i) {
        append(i);
    return *this;
    }

Str& Str::operator<<(Str& s) {
    append(s);
    return *this;
    }

Str& Str::operator+(Str& s) {
    append(s);
    return *this;
    }


bool Str::operator==(Str& str) {
    if ( str.length() != length()) return false;
    uint32_t i;
    for(i=0; i<str.length(); i++)
        if (str.peek(i) !=peek(i)) return false;
    return true;
    }

Str& Str::append(const char* s) {
    while (*s != '\0') {
        write((uint8_t) (* s));
        s++;
        }
    return *this;
    }

Str& Str::append(char s) {
    write((uint8_t) (s));
    return *this;
    }

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

bool ishex(uint8_t c) {
    return (c >= '0' || c <= '9') || (c >= 'A' || c <= 'F') || (c >= 'a' || c <= 'f');
    }

Str& Str::append(uint64_t val) {
#define MAX_CHAR 20
    char str[MAX_CHAR];
    str[MAX_CHAR - 1] = '\0';
    register char *cp = str + MAX_CHAR - 1;
    do {
        *--cp = to_char(val % 10);
        val /= 10;
        }
    while (val != 0);
    append(cp);
    return *this;
    }

Str& Str::append(uint32_t val) {
    uint64_t v = val;
    append(v);
    return *this;
    }

Str& Str::append(int32_t val) {
    uint64_t v = val;
    if (val < 0) {
        write('-');
        v = -val;
        }
    else v = val;
    append(v);
    return *this;
    }

Str& Str::append(bool b) {
    if (b) append("true");
    else append("false");
    return *this;
    }

Str& Str::append(Str& b) {
    b.offset(0);
    while(b.hasData())
        write(b.read());
    return *this;
    }


const char *hexChar = "0123456789ABCDEF";

char nibbleToHex(uint8_t value) {
    return hexChar[value & 0xF];
    }

Str& Str::appendHex(uint8_t byt) {
    write(hexChar[byt >> 4]);
    write(hexChar[byt & 0xF]);
    return *this;
    }

Str& Str::append(void* ptr) {
    union {
        void *ptr;
        uint8_t b[sizeof(int)];
        } u;
    u.ptr=ptr;
    append("0x");
    uint32_t i;
    for(i=0; i<sizeof(int); i++)
        appendHex(u.b[i]);
    return *this;
    }

Str& Str::substr(Str& mstr,uint32_t offset) {
    mstr.offset(offset);
    while(mstr.hasData())
        write(mstr.read());
    return *this;
    }

const char* Str::c_str() {
    if ( _limit < _capacity )
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
            }
        else return E_INVAL;
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

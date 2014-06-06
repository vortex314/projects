/*
 * String.h
 *
 *  Created on: 25-jun.-2013
 *      Author: lieven2
 */

#ifndef STRING_H_
#define STRING_H_
#include "Bytes.h"
#include "Erc.h"

class Str : public Bytes {
public:
    Str();
    Str(int size);
    Str(uint8_t* data, int capacity);
    const char* data();
    Str& append(const char* s);
    void append(uint64_t val);
    void append(uint32_t val);
    void append(int32_t val);
    void append(bool b);
    void appendHex(uint8_t byte);
    Erc parse(uint64_t* pval);
    Erc parse(uint32_t* pval);
    Erc parse(int32_t* pval);
    Erc parseHex(uint8_t* pb);
};


#endif /* STRING_H_ */

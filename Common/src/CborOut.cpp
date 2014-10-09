#include "CborOut.h"

CborOut::CborOut(int size):Bytes(size)
{
    //ctor
}

CborOut::~CborOut()
{
    //dtor
}

CborOut& CborOut::add(int i)
{
    if ( i > 0) addToken(C_PINT,(uint64_t)i);
    else addToken(C_NINT,(uint64_t)-i);;
    return *this;
}

CborOut& CborOut::add(float fl)
{
    union
    {
        float f;
        uint8_t b[4];
    };
    f=fl;
    addHeader(C_SPECIAL,26);
    for(int i=3; i>=0; i--)
        write(b[i]);
    return *this;
}
CborOut& CborOut::add(double d)
{
    union
    {
        double dd;
        uint8_t b[8];
    };
    dd=d;
    addHeader(C_SPECIAL,27);
    for(int i=7; i>=0; i--)
        write(b[i]);
    return *this;
}
CborOut& CborOut::add(Bytes& b)
{
    addToken(C_BYTES,b.length());
    b.offset(0);
    while(b.hasData())
        write(b.read());

    return *this;
}
CborOut& CborOut::add(Str& str)
{
    addToken(C_STRING,str.length());
    str.offset(0);
    while(str.hasData())
        write(str.read());

    return *this;
}
#include <cstring>
CborOut& CborOut::add(const char* s)
{
    uint32_t size=strlen(s);
    addToken(C_STRING,size);
    for(uint32_t i=0; i<size; i++) write(*s++);
    return *this;
}

CborOut& CborOut::add(uint64_t i64)
{
    addToken(C_PINT,i64);
    return *this;
}

CborOut& CborOut::add(int64_t i64)
{
    if ( i64 > 0) addToken(C_PINT,(uint64_t)i64);
    else addToken(C_NINT,(uint64_t)-i64);
    return *this;
}

CborOut& CborOut::add(bool b)
{
    if ( b ) addHeader(C_SPECIAL,21);
    else addHeader(C_SPECIAL,20);
    return *this;
}

CborOut& CborOut::addMap(int size)
{
    if ( size < 0 ) addHeader(C_MAP,31);
    else addToken(C_MAP,size);
    return *this;
}

CborOut& CborOut::addArray(int size)
{
    if ( size < 0 ) addHeader(C_ARRAY,31);
    else addToken(C_ARRAY,size);
    return *this;
}

CborOut& CborOut::addTag(int nr)
{
    addToken(C_TAG,nr);
    return *this;
}

CborOut& CborOut::addBreak()
{
    addHeader(C_SPECIAL, 31);
    return *this;
}


void CborOut::addToken(CborType ctype, uint64_t value)
{
    uint8_t majorType = (uint8_t)(ctype<<5);
    if(value < 24ULL)
    {
        write(majorType | value);
    }
    else if(value < 256ULL)
    {
        write(majorType | 24);
        write(value);
    }
    else if(value < 65536ULL)
    {
        write(majorType | 25);
        write(value >> 8);
    }
    else if(value < 4294967296ULL)
    {
        write(majorType | 26);
        write(value >> 24);
        write(value >> 16);
        write(value >> 8);
        write(value);
    }
    else
    {
        write(majorType | 27);
        write(value >> 56);
        write(value >> 48);
        write(value >> 40);
        write(value >> 32);
        write(value >> 24);
        write(value >> 16);
        write(value >> 8);
        write(value);
    }
}

inline void CborOut::addHeader(uint8_t major,uint8_t minor)
{
    write((major<<5)|minor);
}



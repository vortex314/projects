#include "CborIn.h"

CborIn::CborIn(uint8_t* pb,uint32_t size) : Bytes(pb,size)
{
    //ctor
}

CborIn::~CborIn()
{
    //dtor
}

// <major:5><minor:3>[<length:0-64>][<data:0-n]
// if minor<24 => length=0
uint8_t size[]= {1,2,4,8};

int CborIn ::readToken(uint32_t& major,uint32_t& minor,uint32_t& length)
{
    if ( !hasData() ) return E_NO_DATA;
    uint8_t hdr=read();
    major = hdr>>5;
    minor = hdr & 0x1F;
    if(minor < 24)
    {
        length=0;
    }
    else if(minor == 24)
    {
        length = 1;
    }
    else if(minor == 25)     // 2 byte
    {
        length=2;
    }
    else if(minor == 26)     // 4 byte
    {
        length=4;
    }
    else if(minor == 27)     // 8 byte
    {
        length=8;
    }
    else
    {
        return E_INVAL;
    }
    if ( major == C_STRING || major==C_BYTES )
    {
        if ( length == 0 )
        {
            length=minor;
        }
        else
        {
            uint32_t l=0;
            while(length)
            {
                l <<= 8;
                l+=read();
                length--;
            }
            length=l;
        }
    }
    return E_OK;
}

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

int CborIn ::readToken(CborToken& token)
{
    if ( !hasData() ) return E_NO_DATA;
    uint8_t hdr=read();
    token.major = hdr>>5;
    token.minor = hdr & 0x1F;
    if(token.minor < 24)
    {
        token.length=0;
    }
    else if(token.minor == 24)
    {
        token.length = 1;
    }
    else if(token.minor == 25)     // 2 byte
    {
        token.length=2;
    }
    else if(token.minor == 26)     // 4 byte
    {
        token.length=4;
    }
    else if(token.minor == 27)     // 8 byte
    {
        token.length=8;
    }
    else
    {
        return E_INVAL;
    }
    if ( token.major == C_STRING || token.major==C_BYTES )
    {
        if ( token.length == 0 )
        {
            token.length=token.minor;
        }
        else
        {
            uint32_t l=0;
            while(token.length)
            {
                l <<= 8;
                l+=read();
                token.length--;
            }
            token.length=l;
        }
    }
    return E_OK;
}

uint64_t CborIn::getUint64(CborToken& token)
{
    if ( token.length == 0 )
    {
        return token.minor;
    }
    else
    {
        uint64_t l=0;
        uint32_t length=token.length;
        while(length)
        {
            l <<= 8;
            l+=read();
            length--;
        }
        return l;
    }
}


int CborIn::anyToString(Str& str )
{
    CborToken token;
    readToken(token);
    switch(token.major)
    {
    case C_INT :
    {
        str << getUint64(token);
        break;
    }
    case C_UINT :
    {
        str << getUint64(token);
        break;
    }
    case C_STRING :
    {
        str << "\"";
        uint32_t i;
        for(i=0; i<token.length; i++)
            if(hasData()) str.append((char)read());
        str << "\"";
        break;
    }
    case C_MAP :
    {
        int count = getUint64(token);
        str << "{";
        for(int i=0; i<count; i++)
        {
            if ( i ) str <<",";
            anyToString(str);
            str << ":";
            anyToString(str);
        }
        str << "}";
        break;
    }
    case C_ARRAY :
    {
        int count = getUint64(token);
        str << "[";
        for(int i=0; i<count; i++) {
            if ( i ) str << ",";
            anyToString(str);
        }
        str << "]";
        break;
    }
    case C_TAG :{
        int count = getUint64(token);
        str << "(";
        str << count;
        str << ":";
        anyToString(str);
        str << ")";
    break;
    }
    case C_SPECIAL:{
        uint64_t value= getUint64(token);
        if ( token.minor = 4 )
    break;
    }
    default:{
        break;
    }
    }
    return E_OK;
}

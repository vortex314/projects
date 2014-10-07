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
        token.value=0;
    }
    else if(token.minor == 24 )
    {
        token.value = 1;
    }
    else if(token.minor == 25)     // 2 byte
    {
        token.value=2;
    }
    else if(token.minor == 26)     // 4 byte
    {
        token.value=4;
    }
    else if(token.minor == 27)     // 8 byte
    {
        token.value=8;
    }
    else
    {
        return E_INVAL;
    }
    token.value = getUint64(token);
    return E_OK;
}

uint64_t CborIn::getUint64(CborToken& token)
{
    if ( token.value == 0 )
    {
        return token.minor;
    }
    else
    {
        uint64_t l=0;
        uint32_t length=token.value;
        while(length)
        {
            l <<= 8;
            l+=read();
            length--;
        }
        return l;
    }
}


int CborIn::toString(Str& str )
{
    CborToken token;
    readToken(token);
    switch(token.major)
    {
    case C_PINT :
    {
        str.append(token.value);
        break;
    }
    case C_NINT :
    {
        int64_t v=-token.value;
        str.append(v);
        break;
    }
    case C_BYTES :
    {
        str << "0x";
        uint32_t i;
        for(i=0; i<token.value; i++)
            if(hasData()) str.appendHex(read());
        break;
    }
    case C_STRING :
    {
        str << "\"";
        uint32_t i;
        for(i=0; i<token.value; i++)
            if(hasData()) str.append((char)read());
        str << "\"";
        break;
    }
    case C_MAP :
    {
        int count = token.value;
        str << "{";
        for(int i=0; i<count; i++)
        {
            if ( i ) str <<",";
            toString(str);
            str << ":";
            toString(str);
        }
        str << "}";
        break;
    }
    case C_ARRAY :
    {
        int count = token.value;
        str << "[";
        for(int i=0; i<count; i++)
        {
            if ( i ) str << ",";
            toString(str);
        }
        str << "]";
        break;
    }
    case C_TAG :
    {
        int count = token.value;
        str << "(";
        str << count;
        str << ":";
        toString(str);
        str << ")";
        break;
    }
    case C_SPECIAL:
    {
        uint64_t value= token.value;
        switch(token.minor)
        {
        case 21 :   //TRUE
        {
            str << "true";
            break;
        }
        case 20 :   //FALSE
        {
            str << "false";
            break;
        }
        case 22 :   //NILL
        {
            str << "nill";
            break;
        }
        case 26 :   //FLOAT32
        {
            union
            {
                float f;
                uint32_t i;
            };
            i = value;
            str << f;
            break;
        }
        case 31 :   //BREAK
        {
            str << "BREAK";
            break;
        }
        default :
        {
        }
        }
    }
    }
    return E_OK;
}

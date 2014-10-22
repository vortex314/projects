#include "Cbor.h"

Cbor::Cbor(uint32_t size) : Bytes(size)
{
}

Cbor::Cbor(uint8_t* pb,uint32_t size) : Bytes(pb,size)
{
}

Cbor::~Cbor()
{
    //dtor
}

// <type:5><minor:3>[<length:0-64>][<data:0-n]
// if minor<24 => length=0
int tokenSize[]= {1,2,4,8};

Erc Cbor::readToken(CborToken& token)
{
    CborType type;
    int minor;
    int length;
    uint64_t value;

    if ( !hasData() ) return E_NO_DATA;
    uint8_t hdr=read();
    type = (CborType)(hdr>>5);
    minor = hdr & 0x1F;
    if ( minor < 24 )
    {
        value = minor;
    }
    else if(minor < 28  )
    {
        length = tokenSize[minor-24];
        value = getUint64(length);
    }
    else if ( minor < 31  )
    {
        return E_INVAL;
    }
    else
    {
        value = 65535;  // suppoze very big length will be stopped by BREAK, side effect limited arrays and maps can also be breaked
    }
    if ( type == C_SPECIAL )
        switch(minor)
        {
        case 21 :   //TRUE
        {
            type = C_BOOL;
            value = 1;
            break;
        }
        case 20 :   //FALSE
        {
            type = C_BOOL;
            value = 0;
            break;
        }
        case 22 :   //NILL
        {
            type = C_NILL;
            break;
        }
        case 26 :   //FLOAT32
        {
            type = C_FLOAT;
            break;
        }
        case 27 :   //FLOAT64
        {
            type = C_DOUBLE;
            break;
        }

        case 31 :   //BREAK
        {
            type = C_BREAK;
            break;
        }
        }
    token.type = type;
    token.value=value;

    return E_OK;
}

uint64_t Cbor::getUint64(int length)
{
    uint64_t l=0;
    while(length)
    {
        l <<= 8;
        l+=read();
        length--;
    }
    return l;
}


CborType Cbor::tokenToString(Str& str )
{
    CborToken token;
    if ( readToken(token) != E_OK) return C_ERROR;
    switch(token.type)
    {
    case C_PINT :
    {
        str.append(token.value);
        return C_PINT;
    }
    case C_NINT :
    {
        int64_t v=-token.value;
        str.append(v);
        return C_NINT;
    }
    case C_BYTES :
    {
        str << "0x";
        uint32_t i;
        for(i=0; i<token.value; i++)
            if(hasData()) str.appendHex(read());
        return C_BYTES;
    }
    case C_STRING :
    {
        str << "\"";
        uint32_t i;
        for(i=0; i<token.value; i++)
            if(hasData()) str.append((char)read());
        str << "\"";
        return C_STRING;
    }
    case C_MAP :
    {
        int count = token.value;
        str << "{";
        for(int i=0; i<count; i++)
        {
            if ( i ) str <<",";
            if ( tokenToString(str)==C_BREAK) break;
            str << ":";
            tokenToString(str);
        }
        str << "}";
        return C_MAP;
    }
    case C_ARRAY :
    {
        int count = token.value;
        str << "[";
        for(int i=0; i<count; i++)
        {
            if ( i ) str << ",";
            if ( tokenToString(str)==C_BREAK) break;
        }
        str << "]";
        return C_ARRAY;
    }
    case C_TAG :
    {
        int count = token.value;
        str << "(";
        str << count;
        str << ":";
        tokenToString(str);
        str << ")";
        return C_TAG;
    }
    case C_BOOL :
    {
        if ( token.value ) str << "true";
        else str <<"false";
        return C_BOOL;
    }
    case C_NILL :
    {
        str << "null";
        return C_NILL;
    }
    case C_FLOAT :
    {
        union
        {
            float f;
            uint32_t i;
        };
        i = token.value;
        str << f;
        return C_FLOAT;
    }
    case C_DOUBLE :
    {
        union
        {
            double d;
            uint64_t i;
        };
        i = token.value;
        str << "DOUBLE";
//                str << f;
        return C_DOUBLE;
    }
    case C_BREAK :
    {
        str << "BREAK";
        return C_BREAK;
    }
    case C_SPECIAL:
    {
        return C_ERROR;
    }
    default:  // avoid warnings about additional types > 7
    {
        return C_ERROR;
    }
    }

}
Erc Cbor::toString(Str& str)
{
    CborType ct;
    offset(0);
    while ( hasData() )
    {
        ct=tokenToString(str);
        if ( ct == C_BREAK || ct==C_ERROR) return E_INVAL;
        if ( hasData() ) str <<",";
    };
    return E_OK;
}


CborType Cbor::parse(CborListener& listener)
{
    CborToken token;

    while ( hasData() )
    {
        token.value=0;
        if ( readToken(token) != E_OK) return C_ERROR;
        token.u._uint64 = token.value;
        switch(token.type)
        {
        case C_PINT :
        {
            token.u._uint64 = token.value;
            listener.onToken(token);
            break;
        }
        case C_NINT :
        {
            token.u._int64 = -token.value;
            listener.onToken(token);
            break;
        }
        case C_BYTES :
        {
            token.u.pb = data() + offset();
            listener.onToken(token);
             move(token.value); // skip bytes
            break;
        }
        case C_STRING :
        {
            token.u.pb = data() + offset();
            listener.onToken(token);
            move(token.value);// skip bytes
            break;
        }
        case C_MAP :
        {
            listener.onToken(token);
            int count = token.value;
            for(int i=0; i<count; i++)
            {
                parse(listener);
                if ( parse(listener)==C_BREAK) break;
                parse(listener);
            }
            break;
        }
        case C_ARRAY :
        {
            listener.onToken(token);
            int count = token.value;
            for(int i=0; i<count; i++)
            {
                if ( parse(listener)==C_BREAK) break;
            }
            break;
        }
        case C_TAG :
        {
            listener.onToken(token);
            parse(listener);
            break;
        }
        case C_BOOL :
        {
            token.u._bool= token.value == 1 ? true : false;
            listener.onToken(token);
            break;
        }
        case C_NILL :
        case C_BREAK :
        {
            listener.onToken(token);
            break;
        }
        case C_FLOAT :
        {
            token.u._uint64=token.value ;
            listener.onToken(token);
            break;
        }
        case C_DOUBLE:
        {
            token.u._uint64=token.value;
            listener.onToken(token);
            break;
        }
        case C_SPECIAL:
        {
            listener.onToken(token);
            break;
        }
        default:  // avoid warnings about additional types > 7
        {
            return C_ERROR;
        }
        }
    };
    return token.type;

}

Cbor& Cbor::add(int i)
{
    if ( i > 0) addToken(C_PINT,(uint64_t)i);
    else addToken(C_NINT,(uint64_t)-i);;
    return *this;
}

Cbor& Cbor::add(float fl)
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
Cbor& Cbor::add(double d)
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
Cbor& Cbor::add(Bytes& b)
{
    addToken(C_BYTES,b.length());
    b.offset(0);
    while(b.hasData())
        write(b.read());

    return *this;
}
Cbor& Cbor::add(Str& str)
{
    addToken(C_STRING,str.length());
    str.offset(0);
    while(str.hasData())
        write(str.read());

    return *this;
}
#include <cstring>
Cbor& Cbor::add(const char* s)
{
    uint32_t size=strlen(s);
    addToken(C_STRING,size);
    for(uint32_t i=0; i<size; i++) write(*s++);
    return *this;
}

Cbor& Cbor::add(uint64_t i64)
{
    addToken(C_PINT,i64);
    return *this;
}

Cbor& Cbor::add(int64_t i64)
{
    if ( i64 > 0) addToken(C_PINT,(uint64_t)i64);
    else addToken(C_NINT,(uint64_t)-i64);
    return *this;
}

Cbor& Cbor::add(bool b)
{
    if ( b ) addHeader(C_SPECIAL,21);
    else addHeader(C_SPECIAL,20);
    return *this;
}

Cbor& Cbor::addMap(int size)
{
    if ( size < 0 ) addHeader(C_MAP,31);
    else addToken(C_MAP,size);
    return *this;
}

Cbor& Cbor::addArray(int size)
{
    if ( size < 0 ) addHeader(C_ARRAY,31);
    else addToken(C_ARRAY,size);
    return *this;
}

Cbor& Cbor::addTag(int nr)
{
    addToken(C_TAG,nr);
    return *this;
}

Cbor& Cbor::addBreak()
{
    addHeader(C_SPECIAL, 31);
    return *this;
}


void Cbor::addToken(CborType ctype, uint64_t value)
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

inline void Cbor::addHeader(uint8_t major,uint8_t minor)
{
    write((major<<5)|minor);
}




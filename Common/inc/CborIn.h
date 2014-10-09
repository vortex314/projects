#ifndef CBORIN_H
#define CBORIN_H
#include "Bytes.h"
#include "Str.h"
#include "errno.h"

typedef enum { C_PINT=0,C_NINT,C_BYTES,C_STRING,C_ARRAY,C_MAP,C_TAG,C_SPECIAL=7, // major types
               C_BOOL,C_FLOAT,C_DOUBLE,C_BREAK,C_NILL,C_ERROR
             } // minor additional types
CborType;

typedef struct cborToken
{
    CborType type;
    uint64_t value;
    union  {
        uint64_t _uint64;
        int64_t _int64;
        double _double;
        float _float;
        uint8_t* pb;
        bool _bool;
    } u;
} CborToken;

class CborInListener
{
    public:
    virtual Erc onToken(CborToken& token)=0;
};

class CborIn : public Bytes
{
public:
    CborIn(uint8_t* pb,uint32_t size);
    virtual ~CborIn();
    Erc readToken(CborToken& token);
    Erc toString(Str& str );
    CborType parse(CborInListener& listener);

protected:
private:
    uint64_t getUint64(int length);
    CborType tokenToString(Str& str);

};


#endif // CBORIN_H

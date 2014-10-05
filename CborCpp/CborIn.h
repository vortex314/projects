#ifndef CBORIN_H
#define CBORIN_H
#include "Bytes.h"
#include "errno.h"

class CborIn : public Bytes
{
    public:
    typedef enum { C_INT=0,C_UINT,C_BYTES,C_STRING,C_ARRAY,C_MAP,C_TAG,C_SPECIAL } CborMajor;
        CborIn(uint8_t* pb,uint32_t size);
        virtual ~CborIn();
        int readToken(uint32_t& major,uint32_t& minor,uint32_t& length);
    protected:
    private:


};

#endif // CBORIN_H

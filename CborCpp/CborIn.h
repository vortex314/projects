#ifndef CBORIN_H
#define CBORIN_H
#include "Bytes.h"
#include "Str.h"
#include "errno.h"
 typedef struct {
        uint32_t major;
        uint32_t minor;
        uint64_t value;
    } CborToken;

class CborIn : public Bytes
{
    public:

    typedef enum { C_PINT=0,C_NINT,C_BYTES,C_STRING,C_ARRAY,C_MAP,C_TAG,C_SPECIAL } CborMajor;
        CborIn(uint8_t* pb,uint32_t size);
        virtual ~CborIn();
        int readToken(CborToken& token);
        int toString(Str& str );
        uint64_t getUint64(CborToken& token);
    protected:
    private:


};

#endif // CBORIN_H

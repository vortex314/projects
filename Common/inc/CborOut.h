#ifndef CBOROUT_H
#define CBOROUT_H

#include <Bytes.h>
#include <CborIn.h>

class CborOut : public Bytes
{
    public:
        CborOut(int size);
        virtual ~CborOut();
        CborOut& add(int i);
        CborOut& add(float f);
        CborOut& add(double d);
        CborOut& add(Bytes& b);
        CborOut& add(Str& str);
        CborOut& add(const char* s);
        CborOut& add(uint64_t i64);
        CborOut& add(int64_t i64);
        CborOut& add(bool b);
        CborOut& addMap(int size);
        CborOut& addArray(int size);
        CborOut& addTag(int nr);
        CborOut& addBreak();
    protected:
    private:
        void addToken(CborType type,uint64_t data);
        void addHeader(uint8_t major,uint8_t minor);
        };

#endif // CBOROUT_H

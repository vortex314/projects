#ifndef CBOR_H
#define CBOR_H
#include "Bytes.h"
#include "Str.h"
#include "errno.h"

typedef enum {
	C_PINT = 0, C_NINT, C_BYTES, C_STRING, C_ARRAY, C_MAP, C_TAG, C_SPECIAL = 7, // major types
	C_BOOL,
	C_FLOAT,
	C_DOUBLE,
	C_BREAK,
	C_NILL,
	C_ERROR
} // minor additional types
CborType;

typedef struct cborToken {
	CborType type;
	uint64_t value;
	union {
		uint64_t _uint64;
		int64_t _int64;
		double _double;
		float _float;
		uint8_t* pb;
		bool _bool;
	} u;
} CborToken;

class CborListener {
public:
	virtual Erc onToken(CborToken& token)=0;
};

class Cbor: public Bytes {
public:
	Cbor(uint8_t* pb, uint32_t size);
	Cbor(uint32_t size);
	virtual ~Cbor();

	Cbor& add(int i);
	Cbor& add(float f);
	Cbor& add(double d);
	Cbor& add(Bytes& b);
	Cbor& add(Str& str);
	Cbor& add(const char* s);
	Cbor& add(uint64_t i64);
	Cbor& add(int64_t i64);
	Cbor& add(bool b);
	Cbor& addMap(int size);
	Cbor& addArray(int size);
	Cbor& addTag(int nr);
	Cbor& addBreak();

	Erc readToken(CborToken& token);
	Erc toString(Str& str);
	CborType parse(CborListener& listener);

protected:
private:
	void addToken(CborType type, uint64_t data);
	void addHeader(uint8_t major, uint8_t minor);
	uint64_t getUint64(int length);
	CborType tokenToString(Str& str);

};

#endif // CBOR_H
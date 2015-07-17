/*
 * Json.h
 *
 *  Created on: 26-okt.-2014
 *      Author: lieven2
 */

#ifndef JSON_H_
#define JSON_H_

#include "Packer.h"

class Json : public Packer  {
public:
	Json(Str& str);
	Json(Bytes& bytes);
	Json(uint32_t size);
	virtual ~Json();

	Json& clear();
	Json& operator=(Json& src);

	Json& add(int i);
	Json& add(uint32_t i);
#ifdef DOUBLE
	Json& add(float f);
	Json& add(double d);
#endif
	Json& add(Bytes& b);
	Json& add(Str& str);
	Json& add( char const* s);
	Json& add(uint64_t i64);
	Json& add(int64_t i64);
	Json& add(bool b);
	Json& addMap(int size);
	Json& addArray(int size);
	Json& addKey(const char* s);
	Json& addTag(int nr);
	Json& addBreak();
	Json& addNull();

	bool get(double& d);
	bool get(bool& bl);
	bool get(Str& str);
	bool get(int64_t& ul);

	Erc readToken(PackType& type,Variant& variant);
	Erc toString(Str& str);

protected:
private:
	void addToken(PackType type, uint64_t data);
	void addHeader(uint8_t major, uint8_t minor);
	uint64_t getUint64(int length);
	PackType tokenToString(Str& str);
	uint8_t _break[10];
	int _breakIndex;
	Str& _str;
};

#endif /* JSON_H_ */

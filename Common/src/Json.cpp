/*
 * Json.cpp
 *
 *  Created on: 26-okt.-2014
 *      Author: lieven2
 */

#include "Json.h"

/*
 Json::Json(uint32_t size) :
 _bytes(size) {
 _breakIndex=0;
 }*/

Json::Json(Str& str) :
		_str(str) {
	_breakIndex = 0;
}

Json::Json(Bytes& bytes) :
		_str((Str&) bytes) {
	_breakIndex = 0;
}



Json::~Json() {
	//dtor
}

Json& Json::clear(){
	_str.clear();
	return *this;
}

Json& Json::operator=(Json& src){
	_str = src._str;
	return *this;
}



Erc Json::readToken(PackType& type, Variant& v) {

	return E_OK;
}

uint64_t Json::getUint64(int length) {
	uint64_t l = 0;
	while (length) {
		l <<= 8;
		l += _str.read();
		length--;
	}
	return l;
}

Erc Json::toString(Str& str) {

	return E_OK;
}

Json& Json::add(int i) {
	_str.append((long) i);
	return *this;
}

Json& Json::add(uint32_t i) {
	_str.append((long) i);
	return *this;
}
#ifdef DOUBLE
Json& Json::add(float fl) {
	_str.append(fl);
	return *this;
}
Json& Json::add(double d) {
	_str.append(d);
	return *this;
}
#endif
Json& Json::add(Bytes& b) {
	_str.append('"');
	b.offset(0);
	while (b.hasData())
		_str.appendHex(b.read());
	_str.append('"');
	return *this;
}
Json& Json::add(Str& str) {
	_str.append('"');
	str.offset(0);
	while (str.hasData())
		_str.write(str.read());
	_str.append('"');
	return *this;
}
#include <cstring>
Json& Json::add(const char* s) {
	uint32_t size = strlen(s);
	_str.append('"');
	for (uint32_t i = 0; i < size; i++)
		_str.append(*s++);
	_str.append('"');
	return *this;
}

Json& Json::addKey(const char* s) {
	if (_break[_breakIndex]++)
		_str.append(',');
	add(s);
	_str.append(':');
	return *this;
}

Json& Json::add(uint64_t i64) {
	_str.append(i64);
	return *this;
}

Json& Json::add(int64_t i64) {
	_str.append(i64);
	return *this;
}

Json& Json::add(bool b) {
	if (b)
		_str.append("true");
	else
		_str.append("false");
	return *this;
}

Json& Json::addMap(int size) {
	_str.append('{');
	_break[++_breakIndex] = 0;
	return *this;
}

Json& Json::addArray(int size) {
	_str.append('[');
	_break[++_breakIndex] = 0x80;
	return *this;
}

Json& Json::addTag(int nr) {
	_str.append('(');
	_break[_breakIndex++] = ')';
	return *this;
}

Json& Json::addBreak() {
	if (_break[_breakIndex] < 0x80)
		_str.append('}');
	else
		_str.append(']');
	_breakIndex--;
	return *this;
}

Json& Json::addNull() {
	_str.append("null");
	return *this;
}
#include "jsmn.h"
#include <stdlib.h>
jsmn_parser parser;
jsmntok_t tokens[10];
bool Json::get(int64_t& ll){
	jsmn_init(&parser);
	jsmn_parse(&parser, _str.c_str(), _str.length(), tokens, 10);
	if (tokens[0].type != JSMN_PRIMITIVE)
		return false;
	Str value;
	value.map(_str.data() + tokens[0].start, tokens[0].end);
	ll = atoll(value.c_str());
	return true;
}
bool Json::get(double & d) {
	jsmn_init(&parser);
	jsmn_parse(&parser, _str.c_str(), _str.length(), tokens, 10);
	if (tokens[0].type != JSMN_PRIMITIVE)
		return false;
	Str value;
	value.map(_str.data() + tokens[0].start, tokens[0].end);
	double t = atof(value.c_str());
	d = t;
	return true;
}
bool Json::get(bool & bl) {
	jsmn_init(&parser);
	jsmn_parse(&parser, _str.c_str(), _str.length(), tokens, 10);
	if (tokens[0].type != JSMN_PRIMITIVE)
		return false;
	const char* s = _str.c_str() + tokens[0].start;
	if (strncmp(s, "true", tokens[0].end-tokens[0].start) == 0) {
		bl = true;
		return true;
	} else if (strncmp(s, "false", tokens[0].end-tokens[0].start) == 0) {
		bl = false;
		return true;
	}
	return false;
}

bool Json::get(Str& str) {
	jsmn_init(&parser);
	jsmn_parse(&parser, _str.c_str(), _str.length(), tokens, 10);
	if (tokens[0].type != JSMN_PRIMITIVE)
		return false;
	const char* s = _str.c_str() + tokens[0].start;
	if ( s[tokens[0].start]=='"' && s[tokens[0].end]=='"' )
	{
		str.clear();
		_str.offset(tokens[0].start);
		for(int i=tokens[0].start;i< tokens[0].end;i++){
			str.write(_str.read());
		}
	}
	return false;
}


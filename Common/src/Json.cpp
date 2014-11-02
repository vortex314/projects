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

Json::Json(Str& bytes) :
		_str(bytes) {
	_breakIndex=0;
}

Json::~Json() {
	//dtor
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
/*
JsonType Json::parse(JsonListener& listener) {
	JsonToken token;

	while (hasData()) {
		token.value = 0;
		if (readToken(token) != E_OK)
			return C_ERROR;
		token.u._uint64 = token.value;
		switch (token.type) {
		case C_PINT: {
			token.u._uint64 = token.value;
			listener.onToken(token);
			break;
		}
		case C_NINT: {
			token.u._int64 = -token.value;
			listener.onToken(token);
			break;
		}
		case C_BYTES: {
			token.u.pb = data() + offset();
			listener.onToken(token);
			move(token.value); // skip bytes
			break;
		}
		case C_STRING: {
			token.u.pb = data() + offset();
			listener.onToken(token);
			move(token.value); // skip bytes
			break;
		}
		case C_MAP: {
			listener.onToken(token);
			int count = token.value;
			for (int i = 0; i < count; i++) {
				parse(listener);
				if (parse(listener) == C_BREAK)
					break;
				parse(listener);
			}
			break;
		}
		case C_ARRAY: {
			listener.onToken(token);
			int count = token.value;
			for (int i = 0; i < count; i++) {
				if (parse(listener) == C_BREAK)
					break;
			}
			break;
		}
		case C_TAG: {
			listener.onToken(token);
			parse(listener);
			break;
		}
		case C_BOOL: {
			token.u._bool = token.value == 1 ? true : false;
			listener.onToken(token);
			break;
		}
		case C_NILL:
		case C_BREAK: {
			listener.onToken(token);
			break;
		}
		case C_FLOAT: {
			token.u._uint64 = token.value;
			listener.onToken(token);
			break;
		}
		case C_DOUBLE: {
			token.u._uint64 = token.value;
			listener.onToken(token);
			break;
		}
		case C_SPECIAL: {
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

}*/

Json& Json::add(int i) {
	_str.append((long )i);
	return *this;
}

Json& Json::add(float fl) {
	_str.append((uint32_t)fl);
	return *this;
}
Json& Json::add(double d) {
	_str.append((uint64_t)d);
	return *this;
}
Json& Json::add(Bytes& b) {
	_str.append("0x");
	b.offset(0);
	while (b.hasData())
		_str.appendHex(b.read());

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
Json& Json::add(  const char* s) {
	uint32_t size = strlen(s);
	_str.append('"');
	for (uint32_t i = 0; i < size; i++)
		_str.append(*s++);
	_str.append('"');
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
	_break[_breakIndex++]='}';
	return *this;
}

Json& Json::addArray(int size) {
	_str.append('[');
	_break[_breakIndex++]=']';
	return *this;
}

Json& Json::addTag(int nr) {
	_str.append('(');
	_break[_breakIndex++]=')';
	return *this;
}

Json& Json::addBreak() {
	_str.append(_break[--_breakIndex]);
	return *this;
}


Json& Json::addNull() {
	_str.append("null");
	return *this;
}


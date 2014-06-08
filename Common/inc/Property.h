/*
 * Property.h
 *
 *  Created on: 23-jun.-2013
 *      Author: lieven2
 */

#ifndef PROPERTY_H_
#define PROPERTY_H_
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "Sys.h"
#include "Str.h"
#include "Erc.h"
#include "Strpack.h"
extern const char *PREFIX;
typedef Erc (*Setter)(Strpack& str);
typedef Erc (*Getter)(Strpack& str);

enum Type {
	T_UINT8,
	T_UINT16,
	T_UINT32,
	T_UINT64,
	T_INT8,
	T_INT16,
	T_INT32,
	T_INT64,
	T_BOOL,
	T_FLOAT,
	T_DOUBLE,
	T_BYTES,
	T_ARRAY,
	T_MAP,
	T_STR
};

enum Mode {
	M_READ, M_WRITE, M_RW
};

enum Interface {
	I_ADDRESS, I_INTERFACE, I_SETTER
};

class PackerInterface {
public:
	virtual Erc toPack(Strpack& packer)=0;
	virtual Erc fromPack(Strpack& packer)=0;
	~PackerInterface() {
	}
	;
};

typedef struct {
	union {
		void *_pv;
		Setter _setter;
		Getter _getter;
		PackerInterface* _instance;
	};
	const char *_name;
	const char *_meta;
//	uint16_t _id;
	Type _type;
	Mode _mode;
	Interface _interface;
} PropertyConst;

class Property {
public:
	Property(void *pv, Type type, Mode mode, const char *name,
			const char* meta);
	Property(Setter setter, Getter getter, Type type, Mode mode,
			const char *name, const char* meta);
	Property(PackerInterface* instance, Type type, Mode mode, const char *name,
			const char* meta);
	Property(const PropertyConst* pc);
	virtual ~Property();
	static void add(Property* pp);
	static Property *find(void *pv);
	static Property *find(char* name);
	static Property *find(Str& name);
	static Property* find(const char *prefix, Str& name);
	static Property *get(int id);
	static Property* findNextUpdated();
	static void updated(void *pv);
	static uint32_t count();
	bool isUpdated();
	void updated();
	static void updatedAll();
	void published();
	Erc getMeta(Str& str);
	Erc toPack(Strpack& packer);
	Erc fromPack(Strpack& packer);
	int id();
	void* addr();
	const char* meta();
	const char* name();
private:
	void init(Type t, Mode m, const char* name, const char* meta);
	bool _updated;
	PropertyConst* _pc;
	static uint16_t _count;
};

#endif /* PROPERTY_H_ */

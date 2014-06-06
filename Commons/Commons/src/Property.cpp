/*
 * Property.cpp
 *
 *  Created on: 23-jun.-2013
 *      Author: lieven2
 */

#include "Property.h"
#define MAX_PROPERTIES  30
Property* properties[MAX_PROPERTIES];
uint16_t Property::_count = 0;
const char* typeToString[] = { "UINT8", "UINT16", "UINT32", "UINT64", "INT8",
		"INT16", "INT32", "INT64", "BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY",
		"MAP", "STR" };
const char* modeToString[] = { "R", "W", "RW" };

void Property::add(Property *p) {
	ASSERT(Property::_count < MAX_PROPERTIES)
	properties[Property::_count++] = p;
}

Property::Property(void *pv, Type type, Mode mode, const char* name,
		const char *meta) {
	init(type, mode, name, meta);
	_pc->_pv = pv;
	_pc->_interface = I_ADDRESS;
	Property::add(this);
}

Property::Property(const PropertyConst* pc) {
	_updated = true;
	_pc = (PropertyConst*) pc;
	add(this);
}

void Property::init(Type type, Mode mode, const char* name, const char *meta) {
	_pc = (PropertyConst*) Sys::malloc(sizeof(PropertyConst));
	_pc->_name = name;
	_pc->_meta = meta;
	_pc->_type = type;
	_pc->_mode = mode;
	_updated = true;
	_pc->_setter = (Setter) 0;
	_pc->_getter = NULL;
	_pc->_instance = NULL;
	_pc->_pv = NULL;
}

Property::Property(Setter setter, Getter getter, Type type, Mode mode,
		const char* name, const char* meta) {
	init(type, mode, name, meta);
	_pc->_setter = setter;
	_pc->_getter = getter;
	_pc->_interface = I_SETTER;
	add(this);
}

Property::Property(PackerInterface* instance, Type type, Mode mode,
		const char* name, const char* meta) {
	init(type, mode, name, meta);
	_pc->_instance = instance;
	_pc->_interface = I_INTERFACE;
	add(this);
}

Property::~Property() {
	ASSERT(this != NULL);
	Sys::free(this);
}

/*
 int Property::id() {
 return _id;
 }*/

void* Property::addr() {
	return _pc->_pv;
}

const char* Property::meta() {
	return _pc->_meta;
}

const char* Property::name() {
	return _pc->_name;
}

bool Property::isUpdated() {
	ASSERT(this != NULL);
	return _updated;
}

void Property::published() {
	_updated = false;
}

void Property::updated() {
	ASSERT(this != NULL);
	_updated = true;
}

Erc Property::getMeta(Str& str) {
	str.clear();
	str.append("{ ");
	str.append(" \"name\" : \"");
	str.append(_pc->_name);
	str.append("\" ,\"type\" : \"");
	str.append(typeToString[_pc->_type]);
	str.append("\" ,\"mode\" : \"");
	str.append(modeToString[_pc->_mode]);
	//	str.append(", id :");
	//	str.append((uint32_t) _id);
	if (strlen(_pc->_meta)) {
		str.append("\" , ");
		str.append(_pc->_meta);
	} else {
		str.append("\"");
	};
	str.append(" }");
	return E_OK;
}

Erc Property::toPack(Strpack& packer) {
	if (_pc->_mode == M_WRITE)
		return E_NO_ACCESS;
	if (_pc->_interface == I_SETTER)
		return _pc->_setter(packer);
	if (_pc->_interface == I_INTERFACE)
		return _pc->_instance->toPack(packer);
	switch (_pc->_type) {
	case T_UINT64: {
		uint64_t v = *(uint64_t *) _pc->_pv;
		packer.pack(v);
		break;
	}
	case T_UINT32: {
		packer.pack(*(uint32_t*) _pc->_pv);
		break;
	}
	case T_STR: {
		const char* ps = (const char*) _pc->_pv;
		packer.pack(ps);
		break;
	}
	case T_BYTES: {
		const Bytes* ps = (const Bytes*) _pc->_pv;
		packer.pack((Bytes*) ps);
		break;
	}
	case T_BOOL: {
		const bool* pb = (const bool*) _pc->_pv;
		packer.pack(*pb);
		break;
	}
	case T_FLOAT: {
		const float *pf = (const float*) _pc->_pv;
		packer.pack(*pf);
		break;
	}
	case T_UINT8:
	case T_UINT16:
	case T_INT8:
	case T_INT16:
	case T_INT32:
	case T_INT64:
	case T_DOUBLE:
	case T_ARRAY:
	case T_MAP: { /* TODO implement other serializations */
		return E_OK;
	}
	}
	return E_OK;
}

Erc Property::fromPack(Strpack& unpacker) {
	if (_pc->_mode == M_READ)
		return E_NO_ACCESS;
	if (_pc->_interface == I_SETTER)
		return _pc->_setter(unpacker);
	if (_pc->_interface == I_INTERFACE)
		return _pc->_instance->fromPack(unpacker);
	switch (_pc->_type) {
	case T_UINT64: {
		return unpacker.unpack((uint64_t*) _pc->_pv);
	}
	case T_UINT32: {
		return unpacker.unpack((uint32_t*) _pc->_pv);
	}
	case T_INT32: {
		return unpacker.unpack((int32_t*) _pc->_pv);
	}
	case T_UINT8:
	case T_UINT16:
	case T_INT8:
	case T_INT16:
	case T_INT64:
	case T_DOUBLE:
	case T_BYTES:
	case T_ARRAY:
	case T_MAP:
	case T_BOOL:
	case T_FLOAT:
	case T_STR: { /* TODO implement other deserializations */
		break;
	}
	}
	return E_INVAL;
}

Property* Property::find(void* pv) {
	uint32_t i;
	for (i = 0; i < count(); i++) {
		if (properties[i]->_pc->_pv == pv)
			return properties[i];
	}
	return (Property*) 0;
}

Property* Property::findNextUpdated() {
	uint32_t i;
	for (i = 0; i < count(); i++) {
		if (properties[i]->_updated)
			return properties[i];
	}
	return (Property*) 0;
}

Property* Property::find(char* name) {
	uint32_t i;
	for (i = 0; i < count(); i++) {
		if (strcmp((char*) (properties[i]->_pc->_name), name) == 0)
			return properties[i];
	}
	return (Property*) 0;
}

Property* Property::find(const char* prefix, Str& name) {
	name.offset(strlen(prefix)); // skip prefix for this device
	char str[30];
	int i = 0;
	while (name.hasData()) {
		str[i++] = name.read();
	}
	str[i] = '\0';
	return Property::find(str);
}

Property* Property::find(Str& name) {
	uint32_t i;
	for (i = 0; i < count(); i++) {
		if (strncmp((char*) (properties[i]->_pc->_name), name.data(),
				name.length()) == 0)
			return properties[i];
	}
	return (Property*) 0;
}

void Property::updated(void* pv) {
	Property* pp = find(pv);
	if (pp)
		pp->updated();
}

Property *Property::get(int id) {
	if (id < _count)
		return properties[id];
	return (Property*) 0;
}

uint32_t Property::count() {
	return _count;
}

void Property::updatedAll() {
	uint32_t i;
	for (i = 0; i < count(); i++) {
		properties[i]->updated();
	}
}


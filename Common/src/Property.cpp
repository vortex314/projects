/*
 * Property.cpp
 *
 *  Created on: 23-jun.-2013
 *      Author: lieven2
 */

#include "Property.h"
#define MAX_PROPERTIES  30

Property* Property::_first=NULL;

const char* typeToString[] = { "UINT8", "UINT16", "UINT32", "UINT64", "INT8",
                               "INT16", "INT32", "INT64", "BOOL", "FLOAT", "DOUBLE", "BYTES", "ARRAY",
                               "MAP", "STR","OBJ"
                             };
const char* modeToString[] = { "R", "W", "RW" };


Property* Property::first()
{
    return _first;
}

Property* Property::next(Property* p)
{
    return p->_next;
}

void Property::add(Property *p)
{
    p->_next=NULL;
    Property* cursor;
    if ( _first==NULL )
    {
        _first=p;
    }
    else
    {
        cursor = _first;
        while ( cursor->_next != NULL ) cursor=cursor->_next;
        cursor->_next = p;
    }
}

Property::Property(void *pv, Flags flags, const char* name,
                   const char *meta)
{
    init(flags, name, meta);
    _pc->_pv = pv;
    _pc->_flags.interface=  I_ADDRESS;
    Property::add(this);
}


Property::Property(const PropertyConst* pc)
{
    _updated = true;
    _pc = (PropertyConst*) pc;
    add(this);
}

void Property::init(Flags flags, const char* name, const char *meta)
{
    _pc = (PropertyConst*) new PropertyConst;
    _pc->_name = name;
    _pc->_meta = meta;
    _pc->_flags=flags;
    _updated = true;
    _pc->_setter = (Setter) 0;
    _pc->_getter = NULL;
    _pc->_instance = NULL;
    _pc->_pv = NULL;
}

Property::Property(Setter setter, Getter getter, Flags flags,
                   const char* name, const char* meta)
{
    init(flags, name, meta);
    _pc->_setter = setter;
    _pc->_getter = getter;
    _pc->_flags.interface = I_SETTER;
    add(this);
}

Property::Property(PackerInterface* instance, Flags flags,
                   const char* name, const char* meta)
{
    init(flags, name, meta);
    _pc->_instance = instance;
    _pc->_flags.interface = I_INTERFACE;
    add(this);
}

Property::Property(ObjectInterface* object, Flags flags,
                   const char* name)
{
    init(flags, name, NULL);
    _pc->_object = object;
    _pc->_flags.interface = I_OBJECT;
    add(this);
}

Property::~Property()
{
    ASSERT(this != NULL);
}

/*
 int Property::id() {
 return _id;
 }*/

Flags Property::flags()
{
    return _pc->_flags;
}


void* Property::addr()
{
    return _pc->_pv;
}

char* Property::meta()
{
    return (char *)_pc->_meta;
}

char* Property::name()
{
    return (char *)_pc->_name;
}

bool Property::isUpdated()
{
    ASSERT(this != NULL);
    return _updated;
}

void Property::published()
{
    _updated = false;
}

void Property::updated()
{
    ASSERT(this != NULL);
    _updated = true;
}

uint32_t Property::mode()
{
    return _pc->_flags.mode;
}

Erc Property::getMeta(Str& str)
{
    str.clear();
    str.append("{ ");
    str.append(" \"name\" : \"");
    str.append(_pc->_name);
    str.append("\" ,\"type\" : \"");
    str.append(typeToString[_pc->_flags.type]);
    str.append("\" ,\"mode\" : \"");
    str.append(modeToString[_pc->_flags.mode]);
    //	str.append(", id :");
    //	str.append((uint32_t) _id);
    if (strlen(_pc->_meta))
    {
        str.append("\" , ");
        str.append(_pc->_meta);
    }
    else
    {
        str.append("\"");
    };
    str.append(" }");
    return E_OK;
}

Erc Property::toPack(Strpack& packer)
{
    if (_pc->_flags.interface == I_SETTER)
        return _pc->_setter(_pc->_name,packer);
    else if (_pc->_flags.interface == I_INTERFACE)
        return _pc->_instance->toPack(packer);
    else if (_pc->_flags.interface == I_OBJECT)
    {
        Str prop("");
        return _pc->_object->getValue(prop,packer);
    }
    switch (_pc->_flags.type)
    {
    case T_UINT64:
    {
        uint64_t v = *(uint64_t *) _pc->_pv;
        packer.pack(v);
        break;
    }
    case T_UINT32:
    {
        packer.pack(*(uint32_t*) _pc->_pv);
        break;
    }
    case T_INT32:
    {
        packer.pack(*(int32_t*) _pc->_pv);
        break;
    }
    case T_STR:
    {
        const Str* ps = (Str*) _pc->_pv;
        packer.pack((Str*)ps);
        break;
    }
    case T_BYTES:
    {
        const Bytes* ps = (const Bytes*) _pc->_pv;
        packer.pack((Bytes*) ps);
        break;
    }
    case T_BOOL:
    {
        const bool* pb = (const bool*) _pc->_pv;
        packer.pack(*pb);
        break;
    }
    case T_FLOAT:
    {
        const float *pf = (const float*) _pc->_pv;
        packer.pack(*pf);
        break;
    }
    case T_UINT8:
    case T_UINT16:
    case T_INT8:
    case T_INT16:
    case T_INT64:
    case T_DOUBLE:
    case T_ARRAY:
    case T_MAP:   /* TODO implement other serializations */
    {
        return E_OK;
    }
    }
    return E_OK;
}

Erc Property::fromPack(Strpack& unpacker)
{
    if (_pc->_flags.mode == M_READ)
        return E_NO_ACCESS;
    else if (_pc->_flags.interface == I_SETTER)
        return _pc->_setter(_pc->_name,unpacker);
    else if (_pc->_flags.interface == I_INTERFACE)
        return _pc->_instance->fromPack(unpacker);
    else if (_pc->_flags.interface == I_OBJECT)
    {
        Str str("");
        _pc->_object->setValue(str,unpacker);
        return E_OK;
    }
    switch (_pc->_flags.type)
    {
    case T_UINT64:
    {
        return unpacker.unpack((uint64_t*) _pc->_pv);
    }
    case T_UINT32:
    {
        return unpacker.unpack((uint32_t*) _pc->_pv);
    }
    case T_INT32:
    {
        return unpacker.unpack((int32_t*) _pc->_pv);
    }
    case T_BOOL :
    {
        return unpacker.unpack((bool*) _pc->_pv);
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
    case T_FLOAT:
    case T_STR:   /* TODO implement other deserializations */
    {
        break;
    }
    }
    return E_INVAL;
}

Property* Property::find(void* pv)
{
    Property* p;
    for(p=first(); p!=NULL; p=next(p))
        if (p->_pc->_pv == pv)
            return p;
    return 0;
}
#include "Log.h"
void Property::set( Str* name, Strpack* message)
{
 /*   Log::log().clear().append(" SET ").append(name).append(" = ").append(message);
    Log::log().flush();

    Property* p = find(name);

    Str prefix=Sys::getDeviceName();
    Str str;
    name->offset(prefix.length());
    str.sub(name,name->length()-prefix.length());
    if ( name->endsWith(".set"))
    {
        str.length(str.length()-4); // remove end
        if (p->_pc->_flags.interface == I_OBJECT)
            p->_pc->_object->setValue(str,*message);
    }
    else if ( name->endsWith(".meta"))
    {
    }
        char localName[30];
        int i=0;
        name->offset(strlen(prefix));
        while(name->hasData() && i<30 )
            localName[i++]=name->read();
        localName[i]='\0';*/
}

Property* Property::findNextUpdated()
{
    Property* p;
    for(p=first(); p!=NULL; p=next(p))
        if (p->_updated)
            return p;
    return (Property*) 0;
}

Property* Property::find(char* name)
{
    Property* p;
    for(p=first(); p!=NULL; p=next(p))
        if (strcmp((char*) (p->_pc->_name), name) == 0)
            return p;
    return  0;
}

Property* Property::find(const char* prefix, Str& name)
{
    name.offset(strlen(prefix)); // skip prefix for this device
    char str[30];
    int i = 0;
    while (name.hasData())
    {
        str[i++] = name.read();
    }
    str[i] = '\0';
    return Property::find(str);
}

Property* Property::find(Str& name)
{
    Property* p;
    for(p=first(); p!=NULL; p=next(p))
        if (strncmp((char*) (p->_pc->_name), name.c_str(),
                    name.length()) == 0)
            return p;
    return 0;
}

void Property::updated(void* pv)
{
    Property* pp = find(pv);
    if (pp)
        pp->updated();
}


void Property::updatedAll()
{
    Property* p;
    for(p=first(); p!=NULL; p=next(p))
        p->updated();
}





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
//#include "Prop.h"
extern const char *PREFIX;
typedef Erc (*Setter)(const char* topic,Strpack& str);
typedef Erc (*Getter)(const char* topic,Strpack& str);

enum Type
{
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
    T_STR,
    T_OBJECT
};

enum Mode
{
    M_READ, M_WRITE
};

enum Qos
{
    QOS_0,QOS_1,QOS_2
} ;



enum Interface
{
    I_ADDRESS, I_INTERFACE, I_SETTER,I_OBJECT
};

typedef struct
{
    enum Type type:5;
    enum Mode mode:2;
    enum Qos qos:2;
    enum Interface interface:2;
    bool retained:1;
    bool publishValue:1;
    bool publishMeta:1;
} Flags;
class PackerInterface
{
public:
    virtual Erc toPack(Strpack& packer)=0;
    virtual Erc fromPack(Strpack& packer)=0;
    ~PackerInterface()
    {
    }
};

class ObjectInterface {
    public:
    virtual void setValue(Str& prop,Strpack& value)=0;
    virtual Erc getValue(Str& prop,Strpack& value)=0;
    virtual Erc getMeta(Str& prop,Strpack& meta)=0;
};

typedef struct
{
    union
    {
        void *_pv;
        Setter _setter;
        Getter _getter;
        PackerInterface* _instance;
        ObjectInterface* _object;
    };
    const char *_name;
    const char *_meta;
//	uint16_t _id;
    Flags _flags;
} PropertyConst;

class Property
{
public:
    Property(void *pv, Flags flags, const char *name,
             const char* meta);
    Property(Setter setter, Getter getter, Flags flags,
             const char *name, const char* meta);
    Property(PackerInterface* instance, Flags flags, const char *name,
             const char* meta);
    Property(ObjectInterface* instance, Flags flags, const char *objectPrefix);
    Property(const PropertyConst* pc);
    virtual ~Property();
    static void add(Property* pp);
    static Property* first();
    static Property* next(Property* curr);
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
    Flags flags();
    Erc toPack(Strpack& packer);
    Erc fromPack(Strpack& packer);
    static void set( Str* name, Strpack* message);
    int id();
    void* addr();
     char* meta();
     char* name();
    uint32_t mode();
private:
    void init(Type t, Mode m, const char* name, const char* meta);
    void init(Flags flags, const char* name, const char* meta);
    bool _updated;
    PropertyConst* _pc;
    Property* _next;
    static uint16_t _count;
    static Property* _first;
};

#endif /* PROPERTY_H_ */

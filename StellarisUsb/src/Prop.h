/*
 * Prop.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef PROP_H_
#define PROP_H_
#include <stdint.h>
#include "Sequence.h"

#include "Fsm.h"

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
};

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

Flags flags={T_INT32,M_READ,QOS_0,I_ADDRESS,false,true,true};
#include "Mqtt.h"
#include "Strpack.h"
typedef enum { CMD_GET, CMD_DESC, CMD_PUT } Cmd;
typedef void( *Xdr )( void*, Cmd ,  Strpack& );

class Mqtt;

class  Prop  {
    public :

        const char* _name;
        void* _instance;
        Xdr _xdr;
        Flags _flags;

    public :
Prop();
        Prop ( const char* name, void* instance, Xdr xdr,
               Flags flags ) ;
        static Prop* findProp ( Str& name );
        static void set( Str& topic, Strpack& message, uint8_t header );
    };

class PropertyListener : public Fsm {

private :
	Mqtt& _mqtt;
	struct pt t;
	Str _topic;
public :
	PropertyListener( Mqtt& mqtt ) ;

	void dispatch(Event& event){handler(&event);};

	int handler(Event* event);
};


#endif /* PROP_H_ */

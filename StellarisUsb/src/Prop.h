/*
 * Prop.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef PROP_H_
#define PROP_H_
#include "Property.h"
#include "Sequence.h"
#include "Mqtt.h"

#include "Fsm.h"

#include <stdint.h>

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

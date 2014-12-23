/*
 * Prop.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef PROP_H_
#define PROP_H_
#include <stdint.h>

#include "pt.h"
#include "Handler.h"
#include "Bytes.h"
#include "Mqtt.h"

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
	T_STR,
	T_OBJECT
};

enum Mode {
	M_READ, M_WRITE,M_RW
};

enum Qos {
	QOS_0, QOS_1, QOS_2
};

enum Interface {
	I_ADDRESS, I_INTERFACE, I_SETTER, I_OBJECT
};
enum Interval {
	T_1MSEC,T_10MSEC,T_100MSEC,T_1SEC,T_10SEC,T_100SEC,T_1KSEC,T_10KSEC
};

enum Retained {
	NO_RETAIN,RETAIN
};



typedef struct {
	enum Type type :5;
	enum Mode mode :2;
	enum Interval interval:3;// 0=1 msec -> 7=10^7 msec=10000sec=2,777 hr
	enum Qos qos :2;
	enum Retained retained :1;
	bool doPublish:1;
} Flags;

//Flags flags={T_INT32,M_READ,QOS_0,I_ADDRESS,false,true,true};


typedef enum {
	CMD_GET, CMD_DESC, CMD_PUT
} Cmd;

typedef void (*Xdr)(void*, Cmd, Bytes&);

class Mqtt;

class Prop {
public:

	const char* _name;
	Flags _flags;
	uint64_t _lastPublished;
	Prop* _next;

	static Prop* _first;

public:
	Prop();
	Prop(const char* name, Flags flags);
	Prop(const char* name,const char* value);
	Prop(const char* name,uint64_t&  value);
	void init(const char* name, Flags flags);

	static Prop* findProp(Str& name);

//	static void xdrUint64(void* addr, Cmd cmd, Bytes& strp);
//	static void xdrString(void* addr, Cmd cmd, Bytes& strp);

	virtual void toBytes(Bytes& msg) {};
	virtual void fromBytes(Bytes& msg) {};
	void metaToBytes(Bytes& msg);

	void updated();
	bool hasToBePublished();
	void doPublish();
	void isPublished();
	static void publishAll();
};

class PropMgr: public Handler {

private:
	Mqtt* _mqtt;
	struct pt t;
	Str _topic;
	Bytes _message;
	Prop* _cursor;
	Prop* _next;
	bool _publishMeta;
	enum State { ST_DISCONNECTED,ST_PUBLISHING,ST_WAIT_PUBRESP} _state;

public:
	PropMgr();
	~PropMgr(){};
	void dispatch(Msg& event);
	void nextProp();
	void nextProp(Prop* p);
	void set(Str& topic, Bytes& message);
	void mqtt(Mqtt& mqtt);
};

#endif /* PROP_H_ */

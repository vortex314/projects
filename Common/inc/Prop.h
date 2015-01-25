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
#include "Msg.h"
#include "Handler.h"
#include "Bytes.h"
#include "Flags.h"
#include "Mqtt.h"

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
	Prop(const char* name, const char* value);
	Prop(const char* name, uint64_t& value);
	void init(const char* name, Flags flags);

	static Prop* findProp(Str& name);

//	static void xdrUint64(void* addr, Cmd cmd, Bytes& strp);
//	static void xdrString(void* addr, Cmd cmd, Bytes& strp);

	virtual void toBytes(Bytes& msg) {
	}
	;
	virtual void fromBytes(Bytes& msg) {
	}
	;
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
	Str _prefix;
	Str _getPrefix;
	Str _putPrefix;
	Str _headPrefix;
	struct pt t;
	Str _topic;
	Bytes _message;
	Prop* _cursor;
	Prop* _next;
	void *_src;
	bool _publishMeta;
	enum State {
		ST_DISCONNECTED, ST_PUBLISHING, ST_WAIT_PUBRESP
	} _state;

public:
	PropMgr();
	~PropMgr() {
	}

	int dispatch(Msg& msg);
	void nextProp();
	void nextProp(Prop* p);
	void onPublish(Str& topic, Bytes& message);
	void setMqtt(Mqtt* mqtt) {
		_mqtt = mqtt;
	}
	void setPrefix(const char* s);
};

#endif /* PROP_H_ */

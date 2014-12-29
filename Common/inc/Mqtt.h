/*
 * MqttSeq.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef MQTT_H_
#define MQTT_H_

#include "Event.h"
#include "Sequence.h"
#include "Link.h"
#include "MqttIn.h"
#include "CircBuf.h"
#include "pt.h"
#include "MqttConstants.h"
#include "Log.h"
#include "Prop.h"
#include "MqttOut.h"

#include "Handler.h"
#include "Event.h"
#include "Msg.h"

//************************************** CONSTANTS ****************************
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 2000
#define TIME_WAIT_CONNECT 5000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )
#define TOPIC_MAX_SIZE	40
#define MSG_MAX_SIZE	256

class Subscriber: public Handler {
public:
	Subscriber(Link& link);
	int ptRun(Msg& msg);
	void sendPubRec();
	// will invoke
private:
	Link& _link;
	Str _topic;
	Bytes _message;
	Flags _flags;
	uint16_t _messageId;
	uint16_t _retries;
};

class Publisher: public Handler {
public:
	Publisher(Link& link);
	int ptRun(Msg& msg);
	bool publish(Str& topic, Bytes& msg, Flags flags);
	// will send PUB_OK,PUB_FAIL
private:
	void sendPublish();
	void sendPubRel();
	Link& _link;
	enum State {
		ST_READY,
		ST_BUSY,
	} _state;
	Str _topic;
	Bytes _message;
	uint16_t _messageId;
	Flags _flags;
	uint16_t _retries;
};

class Subscription: public Handler {
public:
	Subscription(Link& link);
	int ptRun(Msg& msg);
	void sendSubscribePut();
	void sendSubscribeGet();
private:
	Link& _link;
	enum State {
			ST_DISCONNECTED,
			ST_WAIT_SUBACK_PUT,
			ST_WAIT_SUBACK_GET,
			ST_SLEEP
		} _state;
	uint16_t _retries;
	uint16_t _messageId;
};

class Pinger: public Handler {
public:
	Pinger(Link& link);
	int ptRun(Msg& msg);
private:
	Link& _link;
	uint32_t _retries;
};

class Mqtt: public Handler {

private:
	Link& _link;
	Pinger* _pinger;
	Publisher* _publisher;
	Subscriber* _subscriber;
	Subscription* _subscription;
	uint32_t _retries;
public:
	Mqtt(Link& link);
	~Mqtt();
	void sendConnect();
	int ptRun(Msg& msg);
	static uint16_t nextMessageId();
	bool publish(Str& topic, Bytes& msg, Flags flags);
	bool isConnected();
//	Erc disconnect();
private:
	void sendSubscribe(uint8_t flags);
};

#endif /* MQTT_H_ */

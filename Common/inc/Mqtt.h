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

//************************************** CONSTANTS ****************************
#define MAX_MSG_SIZE 100
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 1000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )

class Subscriber: public Handler {
public:
	Subscriber(Mqtt& mqtt);
	void onMqttMessage(MqttIn& msg);
	void onTimeout();
	// will invoke
private:
	Mqtt& _mqtt;
};

class Publisher: public Handler {
public:
	Publisher(Link& link);
	void onMqttMessage(MqttIn& msg);
	void onTimeout();
	void onOther(Msg& msg);
	bool publish(Str& topic, Bytes& msg, Flags flags);
	// will send PUB_OK,PUB_FAIL
private:
	void sendPublish();
	void sendPubRel();
	Link& _link;
	enum State {
		ST_DISCONNECTED,
		ST_READY,
		ST_WAIT_PUBACK,
		ST_WAIT_PUBREC,
		ST_WAIT_PUBCOMP
	} _state;
	Str _topic;
	Bytes _message;
	uint16_t _messageId;
	Flags _flags;
	uint16_t _retries;
};

class Subscription: public Handler {
public:
	Subscription(Mqtt& mqtt);
	void onMqttMessage(MqttIn& msg);
	void onTimeout();
private:
	Mqtt& _mqtt;
};

class Pinger: public Handler {
public:
	Pinger(Link& link);
	void onMqttMessage(MqttIn& msg);
	void onTimeout();
	void onOther(Msg& msg);
private:
	Link& _link;
	uint32_t _retries;
	enum State {
		ST_DISCONNECTED, ST_CONNECTED, ST_WAIT_PINGRESP
	} _state;
};

class Mqtt: public Handler {

private:
	Link& _link;
	Pinger* _pinger;
	Publisher* _publisher;
	uint32_t _retries;
	enum State {
		ST_DISCONNECTED, ST_CONNECTED, ST_WAIT_CONNACK
	} _state;
public:
	Mqtt(Link& link);
	~Mqtt();
	void onTimeout();
	void onMqttMessage(MqttIn& mqttIn);
	void onOther(Msg& msg);
	static uint16_t nextMessageId();
	bool publish(Str& topic, Bytes& msg, Flags flags);
	bool isConnected();
	Erc disconnect();
private:
	void sendSubscribe(uint8_t flags);
};

#endif /* MQTT_H_ */

/*
 * MqttSeq.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef MQTT_H_
#define MQTT_H_

// #include "Event.h"
//#include "Sequence.h"
#include "Link.h"
#include "MqttIn.h"
#include "CircBuf.h"
#include "pt.h"
#include "MqttConstants.h"
#include "Log.h"
#include "MqttOut.h"

#include "Handler.h"
#include "Flags.h"
//#include "Event.h"
#include "Msg.h"

//************************************** CONSTANTS ****************************
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 2000
#define TIME_WAIT_CONNECT 5000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )
#define TIME_FOREVER UINT32_MAX
#define TOPIC_MAX_SIZE	40
#define MSG_MAX_SIZE	256

class MqttPublisher;
class MqttSubscriber;
class MqttSubscription;
class MqttPinger;
class Mqtt;

class Mqtt: public Handler {
public:
	Link& _link;
	Str _prefix;
	MqttSubscriber* _mqttSubscriber;
	MqttPublisher* _mqttPublisher;
	MqttSubscription* _mqttSubscription;
	MqttPinger* _mqttPinger;
	MqttIn _mqttIn; // temp storage in one event call
	MqttOut _mqttOut; // "
	bool _isConnected;

private:
	uint32_t _retries;

public:
	Mqtt(Link& link);
	~Mqtt();
	void sendConnect();
	void onMessage(Msg& msg);
	int dispatch(Msg& msg);
	static uint16_t nextMessageId();
	void getPrefix(Str& prefix);
	void setPrefix(const char * prefix);
	bool isConnected();
	void* subscribe(Str& topic);
	void* publish(Str& topic,Bytes& message,Flags flags);
private:
	void sendSubscribe(uint8_t flags);
};

class MqttSubscriber: public Handler {
public:
	MqttSubscriber(Mqtt& mqtt);
	int dispatch(Msg& msg);
	void sendPubRec();
	void callBack();
	// will invoke
private:
	Mqtt& _mqtt;
	Str _topic;
	Bytes _message;
	Flags _flags;
	uint16_t _messageId;
	uint16_t _retries;
};

class MqttPublish {
public:
	Str topic;
	Bytes message;
	Flags flags;
	MqttPublish(int topicSize, int messageSize) :
			topic(topicSize), message(messageSize) {
	}
};

class MqttPublisher: public Handler {
public:
	MqttPublisher(Mqtt& mqtt);
	int dispatch(Msg& msg);
	void* publish(Str& topic, Bytes& msg, Flags flags);
	// will send PUB_OK,PUB_FAIL
private:
	void sendPublish();
	void sendPubRel();
	Mqtt& _mqtt;
	enum State {
		ST_READY, ST_BUSY,
	} _state;
	Str _topic;
	Bytes _message;
	uint16_t _messageId;
	Flags _flags;
	uint16_t _retries;
};

class MqttSubscription: public Handler {
public:
	MqttSubscription(Mqtt& mqtt);
	int dispatch(Msg& msg);
	void* subscribe(Str& topic);
private:
	Mqtt& _mqtt;
	uint16_t _retries;
	uint16_t _messageId;
	Str _topic;
	void sendSubscribe();
};

class MqttPinger: public Handler {
public:
	MqttPinger(Mqtt& mqtt);
	int dispatch(Msg& msg);
private:
	Mqtt& _mqtt;
	uint32_t _retries;
};

#endif /* MQTT_H_ */

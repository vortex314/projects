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

#include "Fsm.h"
#include "main.h"




//************************************** CONSTANTS ****************************

#define MAX_MSG_SIZE 100
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 1000
#define	TIME_PING 3000

class MqttPing;
class MqttPub;
class MqttSub;



class Mqtt: public Fsm {

private:
	struct pt t;
	uint16_t _messageId;
	bool _isConnected;
	Str str;
	Bytes msg;
	Link& _link;
	MqttPing* mqttPing;
	MqttPub* _mqttPub;
	MqttSub* _mqttSub;

public:

 // const static int DISCONNECTED, CONNECTED, IN,RXD,MESSAGE,DO_PUBLISH;


	Mqtt(Link& link);
	int handler(Msg* event);
	Erc send(Bytes& pb);
	bool isEvent(Msg& event, uint8_t type, uint16_t messageId, uint8_t qos);
	bool Publish(Flags flags,uint16_t id,Str& topic,Bytes& msg);
	MqttIn* getBuffer(uint32_t idx);
	bool isConnected();
	Erc disconnect();

	void connecting(Msg& event);
	void sleep(Msg& event);
	void subscribing(Msg& event);
	void waitDisconnect(Msg& event);
private:
	void sendSubscribe(uint8_t flags);
};

class MqttPub:public Fsm {
private:
	uint32_t _retryCount;
	uint16_t _messageId;
	Str _topic;
	Bytes _message;
	Flags _flags;
	uint32_t _id;
	Mqtt& _mqtt;
public :
	MqttPub(Mqtt& mqtt);
	bool send(Flags flags, uint16_t id, Str& topic, Bytes& msg);
	void Publish();
	void sleep(Msg& event);
	void ready(Msg& event);
	void qos1Pub(Msg& event);
	void qos2Pub(Msg& event);
	void qos2Comp(Msg& event);
};

class MqttPing :public Fsm {
private:
	uint32_t _retryCount;
	Mqtt& _mqtt;
public :
	MqttPing(Mqtt& mqtt);
	void sleep(Msg& event);
	void sleepBetweenPings(Msg& event);
	void waitPingResp(Msg& event);
};




#endif /* MQTT_H_ */

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


class Prop;

//************************************** CONSTANTS ****************************

#define MAX_MSG_SIZE 100
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 1000
#define	TIME_PING 3000

class MqttPing;
class MqttPub;
class MqttSubQos0;
class MqttSubQos1;
class MqttSubQos2;


class Mqtt: public Fsm {

private:
	struct pt t;
	uint16_t _messageId;
	bool _isConnected;
	Str str;
	Str msg;
	Link& _link;
	MqttPing* mqttPing;
	MqttPub* _mqttPub;
	MqttSubQos0* mqttSubQos0;
	MqttSubQos1* mqttSubQos1;
	MqttSubQos2* mqttSubQos2;

public:

  const static int DISCONNECTED, CONNECTED, IN,RXD,MESSAGE,DO_PUBLISH;


	Mqtt(Link& link);
	int handler(Event* event);
	Erc send(Bytes& pb);
	bool isEvent(Event* event, uint8_t type, uint16_t messageId, uint8_t qos);
	bool Publish(Flags flags,uint16_t id,Str& topic,Strpack& strp);
	MqttIn* recv();
	bool isConnected();
	Erc disconnect();

	void waitConnect(Event& event);
	void waitConnAck(Event& event);
	void waitDisconnect(Event& event);
};

class MqttPub:public Fsm {
private:
	uint32_t _retryCount;
	uint16_t _messageId;
	Str _topic;
	Str _message;
	Flags _flags;
	uint32_t _id;
	Mqtt& _mqtt;
public :
	MqttPub(Mqtt& mqtt);
	bool send(Flags flags, uint16_t id, Str& topic, Strpack& strp);
	void Publish();
	void sleep(Event& event);
	void ready(Event& event);
	void qos1Pub(Event& event);
	void qos2Pub(Event& event);
	void qos2Comp(Event& event);
};

class MqttPing :public Fsm {
private:
	uint32_t _retryCount;
	Mqtt& _mqtt;
public :
	MqttPing(Mqtt& mqtt);
	void sleep(Event& event);
	void sleepBetweenPings(Event& event);
	void waitPingResp(Event& event);
};




#endif /* MQTT_H_ */

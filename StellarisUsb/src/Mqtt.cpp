/*
 * MqttSeq.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"

//*************************************************** GLOBAL VARS *************

Log log;

MqttIn mqttIn(MAX_MSG_SIZE);
MqttOut mqttOut(MAX_MSG_SIZE);
Str putPrefix(20);
Str getPrefix(20);
Str prefix(20);

uint16_t gMessageId = 1;

/*
 LedBlink ledBlink;
 Usb usb;
 Mqtt mqtt(usb);
 PropertyListener propertyListener(mqtt);
 MqttPing mqttPing(mqtt);
 new MqttSubQos0(mqtt);
 new MqttSubQos1(mqtt);
 new MqttSubQos2(mqtt);
 */

/*****************************************************************************
 *   HANDLE PING keep alive
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE MQTT send publish message with qos=2 and 1
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE Property scan for changes
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE MQTT received publish message with qos=2
 ******************************************************************************/

class MqttSub: public Fsm {
private:
	Mqtt& _mqtt;
public:
	MqttSub(Mqtt& mqtt);

	bool send(Flags flags, uint16_t id, Str& topic, Strpack& strp);
	void Publish();
	void sleep(Msg& event);
	void ready(Msg& event);
	void qos1Sub(Msg& event);
	void qos2Sub(Msg& event);
	void qos2Rel(Msg& event);

};

/*****************************************************************************
 *   Scan for topic changes and publish with right qos and retain
 ******************************************************************************/

#include "Usb.h"
/*****************************************************************************
 *   Generate next message id
 ******************************************************************************/

uint16_t nextMessageId() {
	return gMessageId++;
}

void GlobalInit() {

	prefix << "Stellaris" << "/";
	mqttOut.prefix(prefix);
	getPrefix << "GET/" << prefix;
	putPrefix << "PUT/" << prefix;

}
;
/*****************************************************************************
 *   MQTT EVENT filter
 ******************************************************************************/
#include "Pool.h"

bool Mqtt::isEvent(Msg& event, uint8_t type, uint16_t messageId, uint8_t qos) {

	Signal sig=event.sig();

	if (sig != SIG_MQTT_MESSAGE)
		return false;
	/*
	 * m.create(100).add((uint8_t) SIG_MQTT_MESSAGE).add(
	 msg->_header).add(msg->_messageId).add(msg->_topic).add(
	 msg->_message).send();
	 */

	MqttIn mqttIn(100);
	event.get(mqttIn);
	mqttIn.parse();

	if (mqttIn.type() != type)
		return false;
	if (messageId)
		if (mqttIn.messageId() != messageId)
			return false;
	return true;
}

bool Mqtt::Publish(Flags flags, uint16_t id, Str& topic, Strpack& strp) {
	return _mqttPub->send(flags, id, topic, strp);
}

void Mqtt::waitConnect(Msg& event) {
	switch (event.sig()) {
	case SIG_USB_CONNECTED: {
		timeout(TIME_WAIT_REPLY);
		mqttOut.Connect(MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
				"system/online", "false", "", "",
				TIME_KEEP_ALIVE / 1000);
		_link.send(mqttOut);
		TRAN(Mqtt::waitConnAck);
		break;
	}
	default: {

	}
	}
}

void Mqtt::waitConnAck(Msg& event) {
	switch (event.sig()) {
	case SIG_TIMER_TICK: {
		if (timeout())
			TRAN(Mqtt::waitConnect);
		break;
	}
	case SIG_USB_DISCONNECTED: {
		TRAN(Mqtt::waitConnect);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (isEvent(event, MQTT_MSG_CONNACK, 0, 0)) {
			Msg::publish(SIG_MQTT_CONNECTED);
			TRAN(Mqtt::waitDisconnect);
		}
		break;
	}
	default: {

	}
	}
}

void Mqtt::waitDisconnect(Msg& event) {
	switch (event.sig()) {
	case SIG_USB_DISCONNECTED: {
		Msg::publish(SIG_MQTT_DISCONNECTED);
		TRAN(Mqtt::waitConnect);
		break;
	}
	default: {

	}
	}
}

Mqtt::Mqtt(Link & link) :
		str(30), msg(10), _link(link) {
	mqttPing = new MqttPing(*this);
	_mqttPub = new MqttPub(*this);
	init(static_cast<SF>(&Mqtt::waitConnect));

//	PT_INIT(&t);
	_messageId = 2000;
	_isConnected = false;
//	new MqttPing(*this);
	GlobalInit();
}

Erc Mqtt::send(Bytes & pb) {
	if (_link.isConnected())
		return _link.send(pb);
	else
		return E_AGAIN;
}


bool Mqtt::isConnected() {
	return _isConnected;
}

Erc Mqtt::disconnect() {
	return E_OK;
}

/*****************************************************************************
 *   HANDLE PING keep alive
 ******************************************************************************/

MqttPing::MqttPing(Mqtt & mqtt) :
		_mqtt(mqtt) {
	_retryCount = 0;
	init(static_cast<SF>(&MqttPing::sleep));
}

void MqttPing::sleep(Msg& event) {

	if (event.sig() == (SIG_MQTT_CONNECTED))
		TRAN(MqttPing::waitPingResp);
}

void MqttPing::sleepBetweenPings(Msg& event) {
	switch (event.sig()) {
	case SIG_ENTRY: {
		timeout(TIME_PING);
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout())
			TRAN(MqttPing::waitPingResp);
		break;
	}
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	default: {

	}
	}
}

void MqttPing::waitPingResp(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	case SIG_ENTRY: {
		mqttOut.PingReq();
		_mqtt.send(mqttOut);
		timeout(TIME_WAIT_REPLY);
		break;
	}
	case SIG_EXIT: {
		timeout(INT32_MAX);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PINGRESP, 0, 0)) {
			TRAN(MqttPing::sleepBetweenPings);
		}
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout()) {
			if (_retryCount < 3) {
				mqttOut.PingReq();
				_mqtt.send(mqttOut);
				timeout(TIME_WAIT_REPLY);
			} else {
				TRAN(MqttPing::sleep);
				_mqtt.disconnect();
			}
		}
		break;
	}
	default: {
	}
	}

}

/*****************************************************************************
 *   HANDLE MQTT send publish message with qos=2 and 1
 ******************************************************************************/

MqttPub::MqttPub(Mqtt & mqtt) :
		_topic(100), _message(100), _mqtt(mqtt) {
	_messageId = 1000;
	_retryCount = 0;
	_id = 0;
	init(static_cast<SF>(&MqttPub::sleep));
}

bool MqttPub::send(Flags flags, uint16_t id, Str& topic, Strpack& message) {
	if (!Fsm::isInState(static_cast<SF>(&MqttPub::ready)))
		return false;

	_messageId = nextMessageId();
	_topic = topic;
	_message = message;
	_flags = flags;
	_id = id;
	Msg::publish(SIG_MQTT_DO_PUBLISH);
	return true;
}

void MqttPub::sleep(Msg& event) {
	if (event.sig() == (SIG_MQTT_CONNECTED))
		TRAN(MqttPub::ready);
}

void MqttPub::ready(Msg& event) {
	switch (event.sig()) {

	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	case SIG_MQTT_DO_PUBLISH: {
		if (_flags.qos == QOS_0) {
			Publish();
			TRAN(MqttPub::ready);
		} else if (_flags.qos == QOS_1) {
			TRAN(MqttPub::qos1Pub);
		} else if (_flags.qos == QOS_2) {
			TRAN(MqttPub::qos2Pub);
		}
		break;
	}
	default: {

	}
	}
}

void MqttPub::qos1Pub(Msg& event) {
	switch (event.sig()) {

	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	case SIG_ENTRY: {
		_retryCount = 0;
		timeout(TIME_WAIT_REPLY);
		Publish();
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PUBACK, _messageId, 0)) {
			TRAN(MqttPub::ready);
		}
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout()) {
			if (_retryCount < 3) {
				timeout(TIME_WAIT_REPLY);
				Publish();
				_retryCount++;
			} else {
				Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
				TRAN(MqttPub::ready);
			}
		}
		break;
	}
	default: {

	}
	}

}

void MqttPub::qos2Pub(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	case SIG_ENTRY: {
		_retryCount = 0;
		timeout(TIME_WAIT_REPLY);
		Publish();
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PUBREC, _messageId, 0)) {
			TRAN(MqttPub::qos2Comp);
		} // else ignore
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout()) {
			if (_retryCount < 3) {
				timeout(TIME_WAIT_REPLY);
				Publish();
				_retryCount++;
			} else {
				Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
				TRAN(MqttPub::ready);
			}
		}
		break;
	}
	default: {

	}
	}
}

void MqttPub::qos2Comp(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPub::sleep);
		break;
	}
	case SIG_ENTRY: {
		_retryCount = 0;
		timeout(TIME_WAIT_REPLY);
		mqttOut.PubRel(_messageId);
		_mqtt.send(mqttOut);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PUBCOMP, _messageId, 0)) {
			TRAN(MqttPub::qos2Comp);
		} // else ignore
		break;
	}
	case SIG_TIMER_TICK: {
		if (timeout()) {
			if (_retryCount < 3) {
				timeout(TIME_WAIT_REPLY);
				mqttOut.PubRel(_messageId);
				_mqtt.send(mqttOut);
				_retryCount++;
			} else {
				Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
				TRAN(MqttPub::ready);
			}
		}
		break;
	}
	default: {

	}
	}
}

void MqttPub::Publish() {
	uint8_t header = 0;
	if (_flags.qos == QOS_1) {
		header += MQTT_QOS1_FLAG;
	} else if (_flags.qos == QOS_2) {
		header += MQTT_QOS2_FLAG;
	}
	if (_flags.retained)
		header += MQTT_RETAIN_FLAG;
	if (_retryCount)
		header += MQTT_DUP_FLAG;
	mqttOut.Publish(header, _topic, _message, _messageId);
	_mqtt.send(mqttOut);

}

/*****************************************************************************
 *   HANDLE Property scan for changes
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE MQTT received publish message with qos=2
 ******************************************************************************/

/*
 int MqttSubQos0::handler(Msg* event) {
 PT_BEGIN(&t)
 ;
 while (true) {
 PT_YIELD_UNTIL(&t, _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, 0));
 MqttIn* mqttIn = _mqtt.recv();
 if (mqttIn->qos() == 0)
 Prop::set(*mqttIn->topic(), *mqttIn->message(), mqttIn->qos());
 }
 PT_END(&t);
 }

 MqttSubQos1::MqttSubQos1(Mqtt & mqtt) :
 _mqtt(mqtt) {
 PT_INIT(&t);
 }
 int MqttSubQos1::handler(Msg* event) {
 PT_BEGIN(&t)
 ;
 while (true) {
 PT_YIELD_UNTIL(&t,
 _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, MQTT_QOS1_FLAG));
 MqttIn* mqttIn = _mqtt.recv();
 mqttOut.PubAck(mqttIn->messageId()); // send PUBACK
 _mqtt.send(mqttOut);
 Prop::set(*mqttIn->topic(), *mqttIn->message(), mqttIn->qos());

 }
 PT_END(&t);
 }

 MqttSubQos2::MqttSubQos2(Mqtt & mqtt) :
 _mqtt(mqtt), save(MAX_MSG_SIZE) {
 _messageId = 0;
 _isReady = true;
 PT_INIT(&t);
 }
 int MqttSubQos2::handler(Msg* event) {
 MqttIn* mqttIn;
 PT_BEGIN(&t)
 ;
 while (true) {
 PT_YIELD_UNTIL(&t,
 _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, MQTT_QOS2_FLAG));
 _isReady = false;
 mqttIn = _mqtt.recv();
 _messageId = mqttIn->messageId();

 mqttOut.PubRec(_messageId);
 _mqtt.send(mqttOut);

 save.clone(*_mqtt.recv()); // clone the message
 timeout(TIME_WAIT_REPLY);
 PT_YIELD_UNTIL(&t,
 timeout() || _mqtt.isEvent(event, MQTT_MSG_PUBREL, _messageId, 0));
 if (timeout()) {
 _isReady = true;
 continue; // abandon and go for destruction
 }

 mqttOut.PubComp(_messageId);
 _mqtt.send(mqttOut);
 Prop::set(*save.topic(), *save.message(), save.qos());
 _isReady = true;
 }
 PT_END(&t);
 }
 */
/*
 int Mqtt::handler(Msg* event) {
 _mqttPub->dispatch(*event);
 PT_BEGIN(&t)
 ;
 while (1) {
 _messageId = nextMessageId();

 PT_YIELD_UNTIL(&t, _link.isConnected());

 timeout(5000);
 PT_YIELD_UNTIL(&t, event->is(Link::MESSAGE, MQTT_MSG_CONNACK) || timeout());
 if (timeout()) {
 Sequence::publish(SIG_MQTT_DISCONNECTED);
 continue;
 }

 str.clear() << putPrefix << "#";
 mqttOut.Subscribe(0, str, ++_messageId, MQTT_QOS1_FLAG);
 _link.send(mqttOut);

 str.clear() << getPrefix << "#";
 mqttOut.Subscribe(0, str, ++_messageId, MQTT_QOS1_FLAG);
 _link.send(mqttOut);

 str = "system/online";
 msg = "true";
 mqttOut.Publish(MQTT_QOS1_FLAG, str, msg, ++_messageId);
 _link.send(mqttOut);

 timeout(TIME_WAIT_REPLY);
 PT_YIELD_UNTIL(&t, event->is(Link::MESSAGE, MQTT_MSG_PUBACK) || timeout());
 // isEvent(event, MQTT_MSG_PUBACK, _messageId, 0) || timeout());
 if (timeout()) {
 Sequence::publish(SIG_MQTT_DISCONNECTED);
 continue;
 }
 Sequence::publish(SIG_MQTT_CONNECTED);
 _isConnected = true;
 PT_YIELD_UNTIL(&t, event->is(Link::DISCONNECTED) || !_link.isConnected());

 Sequence::publish(SIG_MQTT_DISCONNECTED);
 _isConnected = false;
 }
 PT_END(&t);
 return 0;
 }
 */


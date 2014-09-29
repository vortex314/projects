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

uint16_t nextMessageId() {
	return ++gMessageId;
}

/**********************************************************************/

/*****************************************************************************
 *   HANDLE MQTT received publish message with qos=2
 ******************************************************************************/

class MqttSub: public Fsm {
private:
	Mqtt& _mqtt;
	Str _topic;
	Strpack _message;
	uint16_t _messageId;
	Flags _flags;
	uint8_t _header;
	int _id;
	uint32_t _retryCount;
public:
	MqttSub(Mqtt& mqtt);

	bool send(Flags flags, uint16_t id, Str& topic, Strpack& strp);
	void Publish();
	void sleep(Msg& event);
	void ready(Msg& event);
//	void qos1Sub(Msg& event);
	void qos2Sub(Msg& event);
//	void qos2Rel(Msg& event);

};

/*****************************************************************************
 *   Scan for topic changes and publish with right qos and retain
 ******************************************************************************/

#include "Usb.h"
/*****************************************************************************
 *   Generate next message id
 ******************************************************************************/

void GlobalInit() {

	prefix << "Stellaris";
	prefix << "/";
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

	Signal sig = event.sig();

	if (sig != SIG_MQTT_MESSAGE)
		return false;

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

bool Mqtt::Publish(Flags flags, uint16_t id, Str& topic, Bytes& strp) {
	return _mqttPub->send(flags, id, topic, strp);
}

void Mqtt::sleep(Msg& event) {
	switch (event.sig()) {
	case SIG_ENTRY: {
		Msg::publish(SIG_MQTT_DISCONNECTED);
		break;
	}
	case SIG_USB_CONNECTED: {
		TRAN(Mqtt::connecting);
		break;
	}
	default: {
	}
	}
}

void Mqtt::connecting(Msg& event) {

	switch (event.sig()) {
	case SIG_ENTRY: {
		timeout(TIME_WAIT_REPLY);
		mqttOut.Connect(MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
				"system/online", "false", "", "",
				TIME_KEEP_ALIVE / 1000);
		_link.send(mqttOut);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (isEvent(event, MQTT_MSG_CONNACK, 0, 0)) {
			Msg::publish(SIG_MQTT_CONNECTED);
			TRAN(Mqtt::subscribing);
		}
		break;
	}
	case SIG_TIMEOUT: {
		if (_link.isConnected()) {
			timeout(TIME_WAIT_REPLY);
			mqttOut.Connect(MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
					"system/online", "false", "", "",
					TIME_KEEP_ALIVE / 1000);
			_link.send(mqttOut);
		}
		break;
	}
	case SIG_USB_DISCONNECTED: {
		TRAN(Mqtt::sleep);
		break;
	}
	case SIG_EXIT: {
		timeout(UINT32_MAX);
		break;
	}

	default: {

	}
	}
}

void Mqtt::sendSubscribe(uint8_t flags) {

	str.clear() << putPrefix << "#";
	mqttOut.Subscribe(flags, str, nextMessageId(), MQTT_QOS1_FLAG);
	_link.send(mqttOut);

	str.clear() << getPrefix << "#";
	mqttOut.Subscribe(flags, str, nextMessageId(), MQTT_QOS1_FLAG);
	_link.send(mqttOut);

	str = "system/online";
	msg.clear();
	msg << "true";
	mqttOut.Publish(MQTT_QOS1_FLAG + flags, str, msg, _messageId =
			nextMessageId());
	_link.send(mqttOut);
}

void Mqtt::subscribing(Msg& event) {
	switch (event.sig()) {
	case SIG_ENTRY: {
		sendSubscribe(0);
		timeout(TIME_WAIT_REPLY);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (isEvent(event, MQTT_MSG_PUBACK, _messageId, 0)) {
			TRAN(Mqtt::waitDisconnect);
		}
		break;
	}
	case SIG_TIMEOUT: {
		if (_link.isConnected()) {
			timeout(TIME_WAIT_REPLY);
			sendSubscribe(MQTT_DUP_FLAG);
		}

		break;
	}
	case SIG_USB_DISCONNECTED: {
		TRAN(Mqtt::sleep);
		break;
	}
	case SIG_EXIT: {
		Msg::publish(SIG_MQTT_CONNECTED);
		timeout(UINT32_MAX);
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
		TRAN(Mqtt::connecting);
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
	_mqttSub = new MqttSub(*this);
	init(static_cast<SF>(&Mqtt::connecting));

//	PT_INIT(&t);
	_messageId = nextMessageId();
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
	switch (event.sig()) {
	case SIG_MQTT_CONNECTED: {
		TRAN(MqttPing::waitPingResp);
		break;
	}
	default: {
	}
	}
}

void MqttPing::sleepBetweenPings(Msg& event) {
	switch (event.sig()) {
	case SIG_ENTRY: {
		timeout(TIME_PING);
		break;
	}
	case SIG_EXIT: {
		timeout(INT32_MAX);
		break;
	}
	case SIG_TIMEOUT: {
		TRAN(MqttPing::waitPingResp);
		break;
	}
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPing::sleep);
		break;
	}
	default: {

	}
	}
}

void MqttPing::waitPingResp(Msg& event) {
	switch (event.sig()) {
	case SIG_ENTRY: {
		mqttOut.PingReq();
		_mqtt.send(mqttOut);
		timeout(TIME_WAIT_REPLY);
		_retryCount = 0;
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PINGRESP, 0, 0)) {
			TRAN(MqttPing::sleepBetweenPings);
		}
		break;
	}
	case SIG_TIMEOUT: {
		if (_retryCount < 3) {
			_retryCount++;
			mqttOut.PingReq();
			_mqtt.send(mqttOut);
			timeout(TIME_WAIT_REPLY);
		} else {
			TRAN(MqttPing::sleep);
			_mqtt.disconnect();
		}

		break;
	}
	case SIG_EXIT: {
		timeout(INT32_MAX);
		break;
	}
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttPing::sleep);
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

bool MqttPub::send(Flags flags, uint16_t id, Str& topic, Bytes& message) {
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
		_messageId = nextMessageId();
		if (_flags.qos == QOS_0) {
			Publish();
			TRAN(MqttPub::ready);
			Msg::publish(SIG_MQTT_PUBLISH_OK);
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
			Msg::publish(SIG_MQTT_PUBLISH_OK);
		}
		break;
	}
	case SIG_TIMEOUT: {
		if (_retryCount < 3) {
			timeout(TIME_WAIT_REPLY);
			Publish();
			_retryCount++;
		} else {
			Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
			TRAN(MqttPub::ready);
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
	case SIG_TIMEOUT: {
		if (_retryCount < 3) {
			timeout(TIME_WAIT_REPLY);
			Publish();
			_retryCount++;
		} else {
			Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
			TRAN(MqttPub::ready);
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
			TRAN(MqttPub::ready);
			Msg::publish(SIG_MQTT_PUBLISH_OK);
		} // else ignore
		break;
	}
	case SIG_TIMEOUT: {
		if (_retryCount < 3) {
			timeout(TIME_WAIT_REPLY);
			mqttOut.PubRel(_messageId);
			_mqtt.send(mqttOut);
			_retryCount++;
		} else {
			Msg::publish(SIG_MQTT_PUBLISH_FAILED, _id);
			TRAN(MqttPub::ready);
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
 *   HANDLE MQTT send publish message with qos=2 and 1
 ******************************************************************************/

MqttSub::MqttSub(Mqtt & mqtt) :
		_mqtt(mqtt), _topic(100), _message(100) {
	_messageId = 1000;
	_retryCount = 0;
	_id = 0;
	init(static_cast<SF>(&MqttSub::sleep));
}

void MqttSub::sleep(Msg& event) {
	if (event.sig() == (SIG_MQTT_CONNECTED))
		TRAN(MqttSub::ready);
}

void MqttSub::ready(Msg& event) {
	switch (event.sig()) {

	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttSub::sleep);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, 0)) {
			event.rewind();
			event.sig();
			MqttIn mqttIn(100);
			event.get(mqttIn);
			mqttIn.parse();
			uint8_t qos = mqttIn._header & MQTT_QOS_MASK;
			if (qos == MQTT_QOS0_FLAG) {
				Prop::set(mqttIn._topic, mqttIn._message);
			} else if (qos == MQTT_QOS1_FLAG) {
				_messageId = mqttIn._messageId;
				mqttOut.PubAck(_messageId);
				_mqtt.send(mqttOut);
				Prop::set(mqttIn._topic, mqttIn._message);
			} else if (qos == MQTT_QOS2_FLAG) {
				_topic = mqttIn._topic;
				_message = mqttIn._message;
				_messageId = mqttIn._messageId;
				_header = mqttIn._header;
				TRAN(MqttSub::qos2Sub);
			}
		}
		break;
	}
	default: {

	}
	}
}

void MqttSub::qos2Sub(Msg& event) {
	switch (event.sig()) {
	case SIG_MQTT_DISCONNECTED: {
		TRAN(MqttSub::sleep);
		break;
	}
	case SIG_ENTRY: {
		_retryCount = 0;
		mqttOut.PubRec(_messageId);
		_mqtt.send(mqttOut);
		timeout(TIME_WAIT_REPLY);
		break;
	}
	case SIG_MQTT_MESSAGE: {
		if (_mqtt.isEvent(event, MQTT_MSG_PUBREL, _messageId, 0)) {
			mqttOut.PubComp(_messageId);
			_mqtt.send(mqttOut);
			Prop::set(_topic, _message);
			TRAN(MqttSub::ready);
		} // else ignore
		break;
	}
	case SIG_TIMEOUT: {
		if (_retryCount < 3) {
			timeout(TIME_WAIT_REPLY);
			mqttOut.PubRec(_messageId);
			_mqtt.send(mqttOut);
			_retryCount++;
		} else {
			TRAN(MqttSub::ready);
		}

		break;
	}
	default: {

	}
	}
}

void MqttSub::Publish() {
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

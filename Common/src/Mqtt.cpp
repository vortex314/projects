/*
 * MqttSeq.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"

//*************************************************** GLOBAL VARS *************

Log log;

MqttIn mqttIn(new Bytes(MSG_MAX_SIZE));
MqttOut mqttOut(MSG_MAX_SIZE);
Str putPrefix(30);
Str getPrefix(30);
Str headPrefix(30);
Str prefix(20);

/*****************************************************************************
 *   Generate next message id
 ******************************************************************************/
uint16_t gMessageId = 1;

uint16_t Mqtt::nextMessageId() {
	return ++gMessageId;
}

//____________________________________________________________________________
//
//____________________________________________________________________________

void GlobalInit() {

	prefix << "Stellaris";
	prefix << "/";
	mqttOut.prefix(prefix);
	getPrefix << "GET/" << prefix;
	putPrefix << "PUT/" << prefix;
	headPrefix << "HEAD/" << prefix;
}
//____________________________________________________________________________
//			M	Q	T	T
//____________________________________________________________________________

Mqtt::Mqtt(Link & link) :
		_link(link) {
	_state = ST_DISCONNECTED;
	timeout(100);
	GlobalInit();
	_pinger = new Pinger(link);
	_publisher = new Publisher(link);
	_subscriber = new Subscriber(link);
	_subscription = new Subscription(link);
}
Mqtt::~Mqtt() {
}

bool Mqtt::publish(Str& topic, Bytes& msg, Flags flags) {
	return _publisher->publish(topic, msg, flags);
}

void Mqtt::onTimeout() {
	if (_state == ST_DISCONNECTED || _state == ST_WAIT_CONNACK) {
		_link.connect();
		Bytes msg(8);
		Cbor cbor(msg);
		cbor.add(false);
		mqttOut.Connect(MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
				"system/online", msg, "", "",
				TIME_KEEP_ALIVE / 1000);
		_link.send(mqttOut);
		timeout(TIME_WAIT_REPLY);
	}
}

void Mqtt::onMqttMessage(MqttIn& mqttIn) {
	if (mqttIn.type() == MQTT_MSG_CONNACK) {
		Msg::publish(SIG_MQTT_CONNECTED);
		timeout(UINT32_MAX);
		_state = ST_CONNECTED;
	}
}

void Mqtt::onOther(Msg& msg) {
	if (msg.sig() == SIG_LINK_DISCONNECTED) {
		_state = ST_DISCONNECTED;
		Msg::publish(SIG_MQTT_DISCONNECTED);
		timeout(100);
	}
}
//____________________________________________________________________________
//			PINGER
//____________________________________________________________________________
Pinger::Pinger(Link& link) :
		_link(link) {
	_retries = 0;
	_state = ST_DISCONNECTED;
}

void Pinger::onTimeout() {
	if (_state == ST_WAIT_PINGRESP) {
		if (_retries > 3) {
			_link.disconnect();
		} else {
			_retries++;
			mqttOut.PingReq();
			_link.send(mqttOut);
			timeout(TIME_WAIT_REPLY);
		}
	}
}

void Pinger::onMqttMessage(MqttIn& mqttIn) {
	if (mqttIn.type() == MQTT_MSG_PINGRESP) {
		if (_state == ST_WAIT_PINGRESP) {
			timeout(TIME_PING);
			_retries = 0;
		}
	} else {
		timeout(TIME_PING); // restart timer on any new incoming message;
		_retries=0;
	}
}

void Pinger::onOther(Msg& msg) {
	if (msg.sig() == SIG_MQTT_DISCONNECTED) {
		_state = ST_DISCONNECTED;
		_retries = 0;
	} else if (msg.sig() == SIG_MQTT_CONNECTED) {
		_state = ST_WAIT_PINGRESP;
		timeout(TIME_PING);
		_retries = 0;
	}
}

//____________________________________________________________________________
//			PUBLISHER
//____________________________________________________________________________
Publisher::Publisher(Link& link) :
		_link(link), _topic(TOPIC_MAX_SIZE), _message(MSG_MAX_SIZE) {
	_messageId = 0;
	_retries = 0;
	_state = ST_DISCONNECTED;
}

bool Publisher::publish(Str& topic, Bytes& msg, Flags flags) {
	if (_state == ST_READY) {
		_retries = 0;
		_topic = topic;
		_message = msg;
		_messageId = Mqtt::nextMessageId();
		_flags = flags;
		sendPublish();
		return true;
	}
	return false;
}

void Publisher::sendPublish() {
	uint8_t header = 0;
	if (_flags.qos == QOS_0) {
		_state = ST_READY;
		Msg::publish(SIG_MQTT_PUBLISH_OK);
	} else if (_flags.qos == QOS_1) {
		header += MQTT_QOS1_FLAG;
		_state = ST_WAIT_PUBACK;
		timeout(TIME_WAIT_REPLY);
	} else if (_flags.qos == QOS_2) {
		header += MQTT_QOS2_FLAG;
		_state = ST_WAIT_PUBREC;
		timeout(TIME_WAIT_REPLY);
	}
	if (_flags.retained)
		header += MQTT_RETAIN_FLAG;
	if (_retries) {
		header += MQTT_DUP_FLAG;
	}
	mqttOut.Publish(header, _topic, _message, _messageId);
	_link.send(mqttOut);
	_retries++;
	timeout(TIME_WAIT_REPLY);
}

void Publisher::sendPubRel() {
	mqttOut.PubRel(_messageId);
	_link.send(mqttOut);
	timeout(TIME_WAIT_REPLY);
}

void Publisher::onTimeout() {
	if (_retries > 3) {
		Msg::publish(SIG_MQTT_PUBLISH_FAILED);
		_state = ST_READY;
		timeout(UINT32_MAX);
	} else if (_state == ST_WAIT_PUBACK || _state == ST_WAIT_PUBREC) {
		sendPublish();
	} else if (_state == ST_WAIT_PUBCOMP) {
		_retries++;
		mqttOut.PubRel(_messageId);
		_link.send(mqttOut);
		timeout(TIME_WAIT_REPLY);
	}
}

void Publisher::onMqttMessage(MqttIn& mqttIn) {
	if (_state != ST_DISCONNECTED) {
		if (_state == ST_WAIT_PUBACK && mqttIn.type() == MQTT_MSG_PUBACK
				&& mqttIn.messageId() == _messageId) {
			Msg::publish(SIG_MQTT_PUBLISH_OK);
			_state = ST_READY;
			timeout(UINT32_MAX);
		} else if (_state == ST_WAIT_PUBCOMP
				&& mqttIn.type() == MQTT_MSG_PUBCOMP
				&& mqttIn.messageId() == _messageId) {
			Msg::publish(SIG_MQTT_PUBLISH_OK);
			_state = ST_READY;
			timeout(UINT32_MAX);
		} else if (_state == ST_WAIT_PUBREC && mqttIn.type() == MQTT_MSG_PUBREC
				&& mqttIn.messageId() == _messageId) {
			_state = ST_WAIT_PUBCOMP;
			sendPubRel();
		}
	}
}

void Publisher::onOther(Msg& msg) {
	if (msg.sig() == SIG_MQTT_DISCONNECTED) {
		_state = ST_DISCONNECTED;
		_retries = 0;
	} else if (msg.sig() == SIG_MQTT_CONNECTED) {
		_state = ST_READY;
		timeout(UINT32_MAX);
		_retries = 0;
	}
}
//____________________________________________________________________________
//			SUBSCRIBER
//____________________________________________________________________________
Subscriber::Subscriber(Link& link) :
		_link(link), _topic(30), _message(MSG_MAX_SIZE) {
	_messageId = 0;
	_retries = 0;
	_state = ST_DISCONNECTED;
}

void Subscriber::sendPubRec() {
	mqttOut.PubRec(_messageId);
	_link.send(mqttOut);
	_state = ST_WAIT_PUBREL;
	timeout(TIME_WAIT_REPLY);
}

void Subscriber::onTimeout() {
	if (_retries > 3) {
		_state = ST_READY;
		timeout(UINT32_MAX);
	} else if (_state == ST_WAIT_PUBREL) {
		_retries++;
		sendPubRec();
	}
}

extern PropMgr propMgr;

void Subscriber::onMqttMessage(MqttIn& mqttIn) {
	if (_state != ST_DISCONNECTED) {
		if (_state == ST_READY && mqttIn.type() == MQTT_MSG_PUBLISH) {
			if (mqttIn.qos() == MQTT_QOS0_FLAG) {
				propMgr.set(*mqttIn.topic(), *mqttIn.message());
			} else if (mqttIn.qos() == MQTT_QOS1_FLAG) {
				propMgr.set(*mqttIn.topic(), *mqttIn.message());
				mqttOut.PubAck(mqttIn.messageId());
				_link.send(mqttOut);
				timeout(TIME_WAIT_REPLY);
			} else if (mqttIn.qos() == MQTT_QOS2_FLAG) {
				_retries = 0;
				_topic = *mqttIn.topic();
				_message = *mqttIn.message();
				_messageId = mqttIn.messageId();
				sendPubRec();
			}
		} else if (_state == ST_WAIT_PUBREL && mqttIn.type() == MQTT_MSG_PUBREL
				&& mqttIn.messageId() == _messageId) {
			propMgr.set(_topic, _message);
			mqttOut.PubComp(mqttIn.messageId());
			_link.send(mqttOut);
			timeout(UINT32_MAX);
			_state = ST_READY;
		}
	}
}

void Subscriber::onOther(Msg& msg) {
	if (msg.sig() == SIG_MQTT_DISCONNECTED) {
		_state = ST_DISCONNECTED;
		_retries = 0;
	} else if (msg.sig() == SIG_MQTT_CONNECTED) {
		_state = ST_READY;
		timeout(UINT32_MAX);
		_retries = 0;
	}
}
//____________________________________________________________________________
//			SUBSCRIPTION
//____________________________________________________________________________
Subscription::Subscription(Link& link) :
		_link(link) {
	_retries = 0;
	_state = ST_DISCONNECTED;
}

void Subscription::sendSubscribePut() {
	Str topic(100);
	topic << putPrefix << "#";
	mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
	_link.send(mqttOut);
	_retries++;
	timeout(TIME_WAIT_REPLY);
	_state = ST_WAIT_SUBACK_PUT;
}

void Subscription::sendSubscribeGet() {
	Str topic(100);
	topic << getPrefix << "#";
	mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
	_link.send(mqttOut);
	_retries++;
	timeout(TIME_WAIT_REPLY);
	_state = ST_WAIT_SUBACK_GET;
}

void Subscription::onTimeout() {
	if (_state == ST_WAIT_SUBACK_PUT) {
		sendSubscribePut();
	} else if (_state == ST_WAIT_SUBACK_GET) {
		sendSubscribeGet();
	}
}

void Subscription::onMqttMessage(MqttIn& mqttIn) {
	if (_state != ST_DISCONNECTED) {
		if (_state == ST_WAIT_SUBACK_PUT && mqttIn.type() == MQTT_MSG_SUBACK
				&& mqttIn.messageId() == _messageId) {
			_retries = 0;
			_messageId = Mqtt::nextMessageId();
			sendSubscribeGet();
		}
		if (_state == ST_WAIT_SUBACK_GET && mqttIn.type() == MQTT_MSG_SUBACK
				&& mqttIn.messageId() == _messageId) {
			timeout(UINT32_MAX);
			_state = ST_SLEEP;
		}
	}
}

void Subscription::onOther(Msg& msg) {
	if (msg.sig() == SIG_MQTT_DISCONNECTED) {
		_state = ST_DISCONNECTED;
	} else if (msg.sig() == SIG_MQTT_CONNECTED) {
		_retries = 0;
		_messageId = Mqtt::nextMessageId();
		sendSubscribePut();
	}
}



/*
 * MqttSeq.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"

//*************************************************** GLOBAL VARS *************

Log log;

MqttIn mqttIn(new Bytes(MAX_MSG_SIZE));
MqttOut mqttOut(MAX_MSG_SIZE);
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
		_link(link), _topic(30), _message(MAX_MSG_SIZE) {
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
		_link(link), _topic(30), _message(MAX_MSG_SIZE) {
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

void Subscriber::onMqttMessage(MqttIn& mqttIn) {
	if (_state != ST_DISCONNECTED) {
		if (_state == ST_READY && mqttIn.type() == MQTT_MSG_PUBLISH) {
			if (mqttIn.qos() == MQTT_QOS0_FLAG) {
				Prop::set(*mqttIn.topic(), *mqttIn.message());
			} else if (mqttIn.qos() == MQTT_QOS1_FLAG) {
				Prop::set(*mqttIn.topic(), *mqttIn.message());
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
			Prop::set(_topic, _message);
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
	topic << putPrefix << "/#";
	mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
	_link.send(mqttOut);
	_retries++;
	timeout(TIME_WAIT_REPLY);
	_state = ST_WAIT_SUBACK_PUT;
}

void Subscription::sendSubscribeGet() {
	Str topic(100);
	topic << getPrefix << "/#";
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
/*
 bool Mqtt::isEvent(Msg& event, uint8_t type, uint16_t messageId, uint8_t qos) {

 Signal sig = event.sig();

 if (sig != SIG_MQTT_MESSAGE)
 return false;

 Bytes bytes(100);

 event.get(bytes);
 MqttIn mqttIn(bytes);
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
 _link.connect();
 break;
 }
 case SIG_LINK_CONNECTED: {
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
 case SIG_LINK_DISCONNECTED:
 case SIG_MQTT_DISCONNECTED: {
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
 msg.append("true");
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
 case SIG_LINK_DISCONNECTED:
 case SIG_MQTT_DISCONNECTED: {
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
 case SIG_LINK_DISCONNECTED:
 case SIG_MQTT_DISCONNECTED: {
 Msg::publish(SIG_MQTT_DISCONNECTED);
 TRAN(Mqtt::sleep);
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
 _retries = 0;
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
 Msg::publish(SIG_MQTT_DISCONNECTED);
 return E_OK;
 }

 // *****************************************************************************
 // *   HANDLE PING keep alive
 // ****************************************************************************

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

 // *****************************************************************************
 // *   HANDLE MQTT send publish message with qos=2 and 1
 // ******************************************************************************

 MqttPub::MqttPub(Mqtt & mqtt) :
 _topic(100), _message(100), _mqtt(mqtt) {
 _messageId = 1000;
 _retryCount = 0;
 _id = 0;
 _retries = 0;
 init(static_cast<SF>(&MqttPub::sleep));
 }

 bool MqttPub::send(Flags flags, uint16_t id, Str& topic, Bytes& message) {
 if (!Fsm::isInState(static_cast<SF>(&MqttPub::ready)))
 return false;

 _messageId = Mqtt::nextMessageId();
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
 _messageId = Mqtt::nextMessageId();
 if (_flags.qos == QOS_0) {
 _retryCount = 0;
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
 _retries++;
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
 } else {
 if (_mqtt.isEvent(event, MQTT_MSG_PUBREC, 0, 0))
 Sys::warn(EINVAL, "QOS2PUB_TYPE"); // else ignore
 }
 break;
 }
 case SIG_TIMEOUT: {
 if (_retryCount < 3) {
 timeout(TIME_WAIT_REPLY);
 Publish();
 _retryCount++;
 _retries++;
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

 // *****************************************************************************
 // *   HANDLE MQTT send publish message with qos=2 and 1
 // *****************************************************************************

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
 event.rewind();
 event.sig();
 Bytes bytes(100);
 event.get(bytes);
 MqttIn mqttIn(bytes);
 mqttIn.parse();
 if (mqttIn.type() == MQTT_MSG_PUBLISH) {
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
 } else {
 Sys::warn(EINVAL, "QOS_ERR");
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
 } else if (_mqtt.isEvent(event, MQTT_MSG_PUBREL, 0, 0)) {

 Sys::warn(EINVAL, ""); // else ignore
 }
 break;
 }
 case SIG_TIMEOUT: {
 if (_retryCount < 3) {
 timeout(TIME_WAIT_REPLY);
 mqttOut.PubRec(_messageId);
 _mqtt.send(mqttOut);
 _retryCount++;
 _retries++;
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

 #include "pt.h"
 int Mqtt::handler(Msg& msg) {
 enum Signal sig = msg.sig();
 if (sig == SIG_LINK_DISCONNECTED) {
 PT_INIT(&pt);
 return PT_YIELDED;
 }
 if (sig == SIG_LINK_DISCONNECTED) {
 _isConnected = true;
 return PT_YIELDED;
 }

 PT_BEGIN ( &pt )
 ;
 while (true) {

 while (isConnected()) {
 //           PT_YIELD_UNTIL(&_pt,event->is(RXD) || event->is(FREE) || ( inBuffer.hasData() && (_isComplete==false)) );

 PT_YIELD_UNTIL(&pt, isConnected());
 }
 }

 PT_END ( &pt );
 }*/


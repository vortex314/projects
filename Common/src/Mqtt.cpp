/*
 * MqttSeq.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"

//*************************************************** GLOBAL VARS *************
Log log;
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
void Mqtt::setPrefix(const char* prefix) {

	_prefix.clear() << prefix;
	_prefix << "/";
//    _mqttOut.prefix(_prefix);
	_getPrefix.clear() << "GET/" << _prefix;
	_putPrefix.clear() << "PUT/" << _prefix;
	_headPrefix.clear() << "HEAD/" << _prefix;
}
//____________________________________________________________________________
//			M	Q	T	T
//  sends MQTT_CONNECTED, MQTT_DISCONNECTED
//  listens for TCPCONNECTED, TCP_DISCONNECTED
//____________________________________________________________________________

Mqtt::Mqtt(Link& link) :
		Handler("Mqtt"), _link(link), _prefix(30), _putPrefix(30), _getPrefix(
				30), _headPrefix(30), _mqttIn(MSG_MAX_SIZE), _mqttOut(
		MSG_MAX_SIZE) {
	_retries = 0;
	timeout(100);
	setPrefix("Stellaris-1");
	_pinger = new Pinger(*this);
	reg(_pinger);
	_publisher = new MqttPublisher(*this);
	reg(_publisher);
	_subscriber = new MqttSubscriber(*this);
	_subscription = new Subscription(*this);
}
Mqtt::~Mqtt() {
}

bool Mqtt::publish(Str& topic, Bytes& msg, Flags flags) {
	if (!_isConnected)
		return false;
	return _publisher->publish(topic, msg, flags);
}

void Mqtt::sendConnect() {
	Str str(8);
	str << "false";
	Str online(30);
	online << _prefix << "system/online";
	_mqttOut.Connect(MQTT_QOS2_FLAG, _prefix.c_str(), MQTT_CLEAN_SESSION,
			online.c_str(), str, "", "",
			TIME_KEEP_ALIVE / 1000);
	_link.send(_mqttOut);
}

//________________________________________________________________________________________________
//
//
//________________________________________________________________________________________________

int Mqtt::dispatch(Msg& msg) {
	PT_BEGIN(&pt)
		DISCONNECTED: {
			_isConnected = false;
			while (true) // DISCONNECTED STATE
			{
				MsgQueue::publish(this, SIG_DISCONNECTED, 0, 0);
				_isConnected = false;

				_link.connect();
				timeout(TIME_FOREVER);
				PT_YIELD_UNTIL(&pt,
						msg.is(&_link, SIG_CONNECTED, 0, 0) || timeout()); //  wait Uart connected
				goto LINK_CONNECTED;
			}
		}
		LINK_CONNECTED: {
			while (true) // LINK_CONNECTED
			{
				sendConnect();
				timeout(TIME_WAIT_CONNECT);
				PT_YIELD_UNTIL(&pt,
						msg.is(&_link, SIG_RXD | SIG_DISCONNECTED)
								|| timeout()); // wait reply or timeout on connect send
				if (msg.is(&_link, SIG_DISCONNECTED))
					goto DISCONNECTED;;
				if (msg.is(&_link, SIG_RXD, MQTT_MSG_CONNACK, 0)) {
					MsgQueue::publish(this, SIG_CONNECTED);
					_isConnected = true;
					goto MQTT_CONNECTED;
				}
			}
		}
		MQTT_CONNECTED: {
			_pinger->restart();
			_publisher->restart();
			_subscriber->restart();
			_subscription->restart();
			while (true) // MQTT_CONNECTED
			{
				PT_YIELD_UNTIL(&pt,
						msg.is(&_link, SIG_DISCONNECTED | SIG_RXD)
								|| msg.is(0, SIG_TICK)); // wait for disconnect or message
				if (msg.is(0, SIG_DISCONNECTED))
					goto DISCONNECTED;
				else
					dispatchToChilds(msg);
			}
		}
	PT_END(&pt)
}

//____________________________________________________________________________
//			PINGER
//____________________________________________________________________________
Pinger::Pinger(Mqtt& mqtt) :
	Handler("Pinger"), _mqtt(mqtt) {
_retries = 0;
}

int Pinger::dispatch(Msg& msg) {
PT_BEGIN(&pt)
	PING_SLEEP: {
		timeout(TIME_PING);
		PT_YIELD_UNTIL(&pt, timeout());
		_retries = 0;
		goto PINGING;
	}
	PINGING: {
		_mqtt._mqttOut.PingReq();
		_mqtt._link.send(_mqtt._mqttOut);
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				(msg.is(&_mqtt._link ,SIG_RXD,MQTT_MSG_PINGRESP, 0) || timeout()));
		if (msg.signal == SIG_RXD)
			goto PING_SLEEP;
		if (++_retries > 3) {
			_mqtt._link.disconnect();
			goto PING_SLEEP;
		}
		goto PINGING;
	}
PT_END(&pt)
}
//____________________________________________________________________________
//			PUBLISHER
//____________________________________________________________________________
MqttPublisher::MqttPublisher(Mqtt& mqtt) :
Handler("MqttPublisher"), _mqtt(mqtt), _topic(TOPIC_MAX_SIZE), _message(
MSG_MAX_SIZE) {
_messageId = 0;
_retries = 0;
_state = ST_READY;
}

bool MqttPublisher::publish(Str& topic, Bytes& msg, Flags flags) {
if (_state == ST_READY) {
_retries = 0;
_topic = topic;
_message = msg;
_messageId = Mqtt::nextMessageId();
_flags = flags;
// sendPublish();
Msg msg = { this, SIG_START, 0, 0 };
dispatch(msg);
return true;
}
return false;
}

void MqttPublisher::sendPublish() {
uint8_t header = 0;
if (_flags.qos == QOS_0) {
_state = ST_READY;
} else if (_flags.qos == QOS_1) {
header += MQTT_QOS1_FLAG;
timeout(TIME_WAIT_REPLY);
} else if (_flags.qos == QOS_2) {
header += MQTT_QOS2_FLAG;
timeout(TIME_WAIT_REPLY);
}
if (_flags.retained)
header += MQTT_RETAIN_FLAG;
if (_retries) {
header += MQTT_DUP_FLAG;
}
_mqtt._mqttOut.Publish(header, _topic, _message, _messageId);
_mqtt._link.send(_mqtt._mqttOut);
}

void MqttPublisher::sendPubRel() {
_mqtt._mqttOut.PubRel(_messageId);
_mqtt._link.send(_mqtt._mqttOut);
}

int MqttPublisher::dispatch(Msg& msg) {
PT_BEGIN(&pt)
READY: {
	_state = ST_READY;
	PT_YIELD_UNTIL(&pt, msg.is(this, SIG_START));
	_state = ST_BUSY;
	if (_flags.qos == QOS_0) {
		sendPublish();
		MsgQueue::publish(&_mqtt, SIG_SUCCESS);
	} else if (_flags.qos == QOS_1)
		goto QOS1_ACK;
	else if (_flags.qos == QOS_2)
		goto QOS2_REC;
	goto READY;
}
QOS1_ACK: {
	for (_retries = 0; _retries < 4; _retries++) {
		sendPublish();
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBACK,0) || timeout());
		if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBACK, 0)) {
			MsgQueue::publish(&_mqtt, SIG_SUCCESS); //TODO check _messageId
			goto READY;
		}
	}
	MsgQueue::publish(&_mqtt, SIG_FAIL);
	goto READY;
}
QOS2_REC: {
	for (_retries = 0; _retries < 4; _retries++) {
		sendPublish();
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBREC,0) || timeout());
		if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBREC, 0)) {
			goto QOS2_COMP;
		}
	}
	MsgQueue::publish(&_mqtt, SIG_FAIL);
	goto READY;
}
QOS2_COMP: {
	for (_retries = 0; _retries < 4; _retries++) {
		sendPubRel();
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBCOMP,0) || timeout());
		if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBCOMP, 0)) {
			MsgQueue::publish(&_mqtt, SIG_SUCCESS);
			goto READY;
		}
	}
	MsgQueue::publish(&_mqtt, SIG_FAIL);
	goto READY;
}
PT_END(&pt)
}

//____________________________________________________________________________
//			SUBSCRIBER
//____________________________________________________________________________
MqttSubscriber::MqttSubscriber(Mqtt &mqtt) :
Handler("Subscriber"), _mqtt(mqtt), _topic(30), _message(MSG_MAX_SIZE) {
_messageId = 0;
_retries = 0;
}

void MqttSubscriber::sendPubRec() {
_mqtt._mqttOut.PubRec(_messageId);
_mqtt._link.send(_mqtt._mqttOut);
timeout(TIME_WAIT_REPLY);
}

// #define PT_WAIT_FOR( ___pt, ___signals, ___timeout ) listen(___signals,___timeout);PT_YIELD(___pt);

int MqttSubscriber::dispatch(Msg& msg) {
PT_BEGIN(&pt)
READY: {
timeout(TIME_FOREVER);
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD,MQTT_MSG_PUBLISH,0) || timeout());
if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBLISH, 0)) {
}
if (_mqtt._mqttIn.qos() == MQTT_QOS0_FLAG) {
	MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn);
} else if (_mqtt._mqttIn.qos() == MQTT_QOS1_FLAG) {

	MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn);
	_mqtt._mqttOut.PubAck(_mqtt._mqttIn.messageId());
	_mqtt._link.send(_mqtt._mqttOut);

} else if (_mqtt._mqttIn.qos() == MQTT_QOS2_FLAG) {
	_topic = *_mqtt._mqttIn.topic();
	_message = *_mqtt._mqttIn.message();
	_messageId = _mqtt._mqttIn.messageId();
	goto QOS2_REL;
}
goto READY;
}
QOS2_REL: {
for (_retries = 0; _retries < 4; _retries++) {
	sendPubRec();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD ,MQTT_MSG_PUBREL,0) || timeout());
	if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBREL, 0)) {
		MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn); //TODO
		_mqtt._mqttOut.PubComp(_mqtt._mqttIn.messageId());
		_mqtt._link.send(_mqtt._mqttOut);
		goto READY;
	}
}
goto READY;
}
PT_END(&pt)
}

//____________________________________________________________________________
//			SUBSCRIPTION
//____________________________________________________________________________
Subscription::Subscription(Mqtt & mqtt) :
Handler("Subscription"), _mqtt(mqtt) {
_retries = 0;
_state = ST_DISCONNECTED;
_messageId = 0;
// listen(&_mqtt);
}

int Subscription::dispatch(Msg& msg) {
PT_BEGIN(&pt)
SUB_PUT: {
for (_retries = 0; _retries < 4; _retries++) {
sendSubscribePut();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_DISCONNECTED | SIG_RXD) || timeout());
if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_SUBACK, 0))
	goto SUB_GET;
}
_mqtt._link.disconnect();
goto SUB_PUT;
}

SUB_GET: {
for (_retries = 0; _retries < 4; _retries++) {
sendSubscribeGet();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_DISCONNECTED | SIG_RXD) || timeout());
if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_SUBACK, 0))
	goto SUB_SLEEP;
}
_mqtt._link.disconnect();
goto SUB_PUT;
}
SUB_SLEEP: {
timeout(TIME_FOREVER);
PT_YIELD_UNTIL(&pt, timeout());
}

PT_END(&pt);
}

void Subscription::sendSubscribePut() {
Str topic(100);
topic << _mqtt._putPrefix << "#";
_mqtt._mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}

void Subscription::sendSubscribeGet() {
Str topic(100);
topic << _mqtt._getPrefix << "#";
_mqtt._mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}


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

//____________________________________________________________________________
//			M	Q	T	T
//  sends MQTT_CONNECTED, MQTT_DISCONNECTED
//  listens for TCPCONNECTED, TCP_DISCONNECTED
//____________________________________________________________________________

Mqtt::Mqtt(Link& link) :
		Handler("Mqtt"), _link(link), _prefix(SIZE_TOPIC), _mqttIn(SIZE_MQTT), _mqttOut(
		SIZE_MQTT) {
	_mqttPublisher = new MqttPublisher(*this);
	_mqttSubscriber = new MqttSubscriber(*this);
	_mqttSubscription = new MqttSubscription(*this);
	_mqttPinger = new MqttPinger(*this);

	_mqttPublisher->stop();
	_mqttSubscriber->stop();
	_mqttPinger->stop();
	_mqttSubscription->stop();

	_retries = 0;
	timeout(100);
	_isConnected = false;
}
Mqtt::~Mqtt() {
}

void Mqtt::setPrefix(const char* s) {
	_prefix.clear() << s;
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

void Mqtt::onMessage(Msg& msg) {
	Msg n = msg;
	n.src = this;
	msg.data = 0;
	MsgQueue::publish(n);
}

void* Mqtt::publish(Str& topic, Bytes& message, Flags flags) {
	return _mqttPublisher->publish(topic, message, flags);
}

void* Mqtt::subscribe(Str& topic) {
	return _mqttSubscription->subscribe(topic);
}

//________________________________________________________________________________________________
//
//
//________________________________________________________________________________________________

int Mqtt::dispatch(Msg& msg) {
	PT_BEGIN(&pt)
		DISCONNECTED: {
			_isConnected = false;
			_mqttPublisher->stop();
			_mqttPinger->stop();
			_mqttSubscription->stop();
			MsgQueue::publish(this, SIG_DISCONNECTED, 0, 0);
			_isConnected = false;
			while (true) // DISCONNECTED STATE
			{

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
						msg.is(&_link, SIG_RXD, MQTT_MSG_CONNACK, 0) || msg.is(&_link, SIG_DISCONNECTED) || timeout()); // wait reply or timeout on connect send
				if (msg.is(&_link, SIG_DISCONNECTED))
					goto DISCONNECTED;
				if (msg.is(&_link, SIG_RXD, MQTT_MSG_CONNACK, 0)) {
					MsgQueue::publish(this, SIG_CONNECTED);
					_isConnected = true;
					goto MQTT_CONNECTED;
				}
			}
		}
		MQTT_CONNECTED: {
			while (true) // MQTT_CONNECTED
			{
				_mqttPinger->start();
				_mqttSubscriber->start();
				timeout(TIME_FOREVER);
				PT_YIELD_UNTIL(&pt, msg.is(&_link, SIG_DISCONNECTED)); // wait for disconnect or message
				if (msg.is(&_link, SIG_DISCONNECTED))
					goto DISCONNECTED;
			}
		}
	PT_END(&pt)
}

int MqttSubscription::dispatch(Msg& msg) {
PT_BEGIN(&pt)

	_messageId = Mqtt::nextMessageId();
	for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
		sendSubscribe();
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_SUBACK, 0) || msg.is(&_mqtt._link, SIG_DISCONNECTED ) || timeout());
		if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_SUBACK, 0)) {
			MsgQueue::publish(this, SIG_SUCCESS);
			stop();
		} else if (msg.is(&_mqtt._link, SIG_DISCONNECTED)) {
			MsgQueue::publish(this, SIG_FAIL);
			stop();
		} else if (timeout()) {

		} else
			while (1)
				;
	}
	MsgQueue::publish(this, SIG_FAIL);
	stop();
PT_END(&pt);
}

 //____________________________________________________________________________
 //			PINGER
 //____________________________________________________________________________
MqttPinger::MqttPinger(Mqtt& mqtt) :
Handler("Pinger"), _mqtt(mqtt) {
_retries = 0;
}

int MqttPinger::dispatch(Msg& msg) {
PT_BEGIN(&pt)
PING_SLEEP: {
	timeout(TIME_PING);
	PT_YIELD_UNTIL(&pt, timeout());
	_retries = 0;
	goto PINGING;
}
PINGING: {
	for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
		_mqtt._mqttOut.PingReq();
		_mqtt._link.send(_mqtt._mqttOut);
		timeout(TIME_WAIT_REPLY);
		PT_YIELD_UNTIL(&pt,
				(msg.is(&_mqtt._link ,SIG_RXD,MQTT_MSG_PINGRESP, 0) || timeout()));
		if (msg.signal == SIG_RXD)
			goto PING_SLEEP;
	}
	_mqtt._link.disconnect();
	stop();
}
PT_END(&pt)
}
//____________________________________________________________________________
//			PUBLISHER
//____________________________________________________________________________
MqttPublisher::MqttPublisher(Mqtt& mqtt) :
Handler("MqttPublisher"), _mqtt(mqtt), _topic(SIZE_TOPIC), _message(
SIZE_MESSAGE) {
_messageId = 0;
_retries = 0;
_state = ST_READY;
}

void* MqttPublisher::publish(Str& topic, Bytes& msg, Flags flags) {
if (isRunning())
return 0;
_retries = 0;
_topic = topic;
_message = msg;
_messageId = Mqtt::nextMessageId();
_flags = flags;
start();
return this;
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
/*	MQTT_DISCONNECTED: {
 timeout(TIME_FOREVER);
 PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_CONNECTED)); // wait for connect
 goto READY;
 }*/
READY: {
_state = ST_READY;
PT_YIELD_UNTIL(&pt, msg.is(0, SIG_TICK));
_state = ST_BUSY;
if (_flags.qos == QOS_0) {
	sendPublish();
	MsgQueue::publish(this, SIG_SUCCESS);
	stop();
	PT_YIELD(&pt);
} else if (_flags.qos == QOS_1)
	goto QOS1_ACK;
else if (_flags.qos == QOS_2)
	goto QOS2_REC;
stop();
PT_YIELD(&pt);
}
QOS1_ACK: {
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPublish();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(&pt,
			msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBACK,0) || timeout());
	if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBACK, 0)) {
		MsgQueue::publish(this, SIG_SUCCESS); //TODO check _messageId
		stop();
		PT_YIELD(&pt);
	}
}
MsgQueue::publish(this, SIG_FAIL);
stop();
PT_YIELD(&pt);
}
QOS2_REC: {
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPublish();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(&pt,
			msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBREC,0) || timeout());
	if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBREC, 0)) {
		goto QOS2_COMP;
	}
}
MsgQueue::publish(this, SIG_FAIL);
stop();
PT_YIELD(&pt);
}
QOS2_COMP: {
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
	sendPubRel();
	timeout(TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(&pt,
			msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBCOMP,0) || timeout());
	if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBCOMP, 0)) {
		MsgQueue::publish(this, SIG_SUCCESS);
		stop();
		PT_YIELD(&pt);
	}
}
MsgQueue::publish(this, SIG_FAIL);
stop();
PT_YIELD(&pt);
}
PT_END(&pt)
}

//____________________________________________________________________________
//			SUBSCRIBER
//____________________________________________________________________________
MqttSubscriber::MqttSubscriber(Mqtt &mqtt) :
Handler("Subscriber"), _mqtt(mqtt), _topic(SIZE_TOPIC), _message(SIZE_MESSAGE) {
_messageId = 0;
_retries = 0;
}

void MqttSubscriber::sendPubRec() {
_mqtt._mqttOut.PubRec(_messageId);
_mqtt._link.send(_mqtt._mqttOut);
timeout(TIME_WAIT_REPLY);
}
#include "Prop.h"
extern PropMgr propMgr;

void MqttSubscriber::callBack() {
propMgr.onPublish(_topic, _message);
/* MqttPublish* mp=new MqttPublish(_topic.length(),_message.length());
 mp->topic=_topic;
 mp->message=_message;
 mp->flags = _flags;
 MsgQueue::publish(this,SIG_RXD,0,mp);*/
}

// #define PT_WAIT_FOR( ___pt, ___signals, ___timeout ) listen(___signals,___timeout);PT_YIELD(___pt);

int MqttSubscriber::dispatch(Msg& msg) {
PT_BEGIN(&pt)

READY: {
timeout(TIME_FOREVER);
PT_YIELD_UNTIL(&pt,
	msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBLISH, 0) || msg.is(&_mqtt._link, SIG_DISCONNECTED) || timeout());
if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBLISH, 0)) {
_topic = *((MqttIn*) msg.data)->topic();
_message = *((MqttIn*) msg.data)->message();
_messageId = _mqtt._mqttIn.messageId();
if (_mqtt._mqttIn.qos() == MQTT_QOS0_FLAG) {

	callBack();
} else if (_mqtt._mqttIn.qos() == MQTT_QOS1_FLAG) {

	callBack();
	_mqtt._mqttOut.PubAck(_mqtt._mqttIn.messageId());
	_mqtt._link.send(_mqtt._mqttOut);

} else if (_mqtt._mqttIn.qos() == MQTT_QOS2_FLAG) {
	goto QOS2_REL;

}
} else if (msg.is(&_mqtt._link, SIG_DISCONNECTED)) {
stop();
PT_YIELD(&pt);
}
goto READY;
}
QOS2_REL: {
for (_retries = 0; _retries < MAX_RETRIES; _retries++) {
sendPubRec();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(&pt,
		msg.is(&_mqtt._link, SIG_DISCONNECTED) || msg.is(&_mqtt._link,SIG_RXD ,MQTT_MSG_PUBREL,0) || timeout());
if (msg.is(&_mqtt._link, SIG_RXD, MQTT_MSG_PUBREL, 0)) {
	callBack();
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
MqttSubscription::MqttSubscription(Mqtt & mqtt) :
Handler("Subscription"), _mqtt(mqtt), _topic(SIZE_TOPIC) {
_retries = 0;
_messageId = 0;
// listen(&_mqtt);
}

void* MqttSubscription::subscribe(Str& topic) {
if (isRunning())
return 0;
_topic = topic;
start();
return this;
}

void MqttSubscription::sendSubscribe() {
_mqtt._mqttOut.Subscribe(MQTT_QOS1_FLAG, _topic, _messageId, QOS_2);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}


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
const char* device = "Stellaris-1";

/*****************************************************************************
 *   Generate next message id
 ******************************************************************************/
uint16_t gMessageId = 1;

uint16_t Mqtt::nextMessageId()
{
	return ++gMessageId;
}

bool msgToMqttIn(Msg& msg)
{
	Bytes bytes;
	Signal signal;
	msg.get((uint32_t&) signal);
	if (signal != SIG_MQTT_MESSAGE)
		return false;
	msg.get(bytes);
	mqttIn.remap(&bytes);
	bytes.offset(0);
	if (mqttIn.parse())
		return true;
	return false;
}

bool isMqttMsg(Msg& msg, uint8_t msgType, uint16_t msgId)
{
	if (msgToMqttIn(msg))
	{
		if (msgId == 0)
			return (mqttIn.type() == msgType);
		else
			return (mqttIn.type() == msgType) && (mqttIn.messageId() == msgId);
	}
	return false;
}

//____________________________________________________________________________
//
//____________________________________________________________________________

void GlobalInit()
{

	prefix << device;
	prefix << "/";
	mqttOut.prefix(prefix);
	getPrefix << "GET/" << prefix;
	putPrefix << "PUT/" << prefix;
	headPrefix << "HEAD/" << prefix;
}
//____________________________________________________________________________
//			M	Q	T	T
//  sends MQTT_CONNECTED, MQTT_DISCONNECTED
//  listens for LINK_CONNECTED, LINK_DISCONNECTED
//____________________________________________________________________________

Mqtt::Mqtt(Link & link) :
		Handler("Mqtt"), _link(link)
{
	_retries = 0;
	timeout(100);
	GlobalInit();
	_pinger = new Pinger(link);
	_publisher = new Publisher(link);
	_subscriber = new Subscriber(link);
	_subscription = new Subscription(link);
}
Mqtt::~Mqtt()
{
}

bool Mqtt::publish(Str& topic, Bytes& msg, Flags flags)
{
	return _publisher->publish(topic, msg, flags);
}

void Mqtt::sendConnect()
{
	Bytes bytes(8);
	Cbor cbor(bytes);
	cbor.add(false);
	mqttOut.Connect(MQTT_QOS2_FLAG, device, MQTT_CLEAN_SESSION, "system/online",
			bytes, "", "",
			TIME_KEEP_ALIVE / 1000);
	_link.send(mqttOut);
}

int Mqtt::ptRun(Msg& msg)
{ // setup connection and signal this
	Bytes bytes;
	MqttIn mqttIn;
	PT_BEGIN(&pt)
		while (true)
		{
			Msg::publish(SIG_MQTT_DISCONNECTED);
			_link.connect();
			listen(SIG_LINK_CONNECTED);
			PT_YIELD(&pt);				//  wait Uart connected

			sendConnect();
			listen(SIG_MQTT_MESSAGE | SIG_LINK_DISCONNECTED, TIME_WAIT_CONNECT);
			PT_YIELD(&pt);			// wait reply or timeout on connect send

			if (isMqttMsg(msg, MQTT_MSG_CONNACK, 0))
			{ // handle connack only else loop

				Msg::publish(SIG_MQTT_CONNECTED);
				listen(SIG_LINK_DISCONNECTED);
				PT_YIELD(&pt);	// wait forever on disconnect
			}

		}
	PT_END(&pt)
}

//____________________________________________________________________________
//			PINGER
//____________________________________________________________________________
Pinger::Pinger(Link& link) :
	Handler("Pinger"), _link(link)
{
_retries = 0;
}

int Pinger::ptRun(Msg& msg)
{
if (msg.sig() == SIG_MQTT_DISCONNECTED)
{
	restart();
	return 0;
}
PT_BEGIN(&pt)
	while (true)
	{
		listen(SIG_MQTT_CONNECTED);	// wait forever on  MQTT connection established
		PT_YIELD(&pt);
		_retries = 0;

		while (true)
		{ // keep on pinging
			_retries = 0;
			while (_retries < 3)
			{
				mqttOut.PingReq();
				_link.send(mqttOut);
				_retries++;
				listen(SIG_MQTT_DISCONNECTED | SIG_MQTT_MESSAGE,
				TIME_WAIT_REPLY);
				PT_YIELD_UNTIL(&pt,
						isMqttMsg(msg, MQTT_MSG_PINGRESP, 0) || timeout());
				if (msg.sig() == SIG_MQTT_MESSAGE)
				{ // successful ping

					listen(0, TIME_PING);
					PT_YIELD(&pt);
					_retries = 0;

				}
			}
			_link.disconnect();
		}
	}
PT_END(&pt)
}

//____________________________________________________________________________
//			PUBLISHER
//____________________________________________________________________________
Publisher::Publisher(Link& link) :
Handler("Publisher"), _link(link), _topic(TOPIC_MAX_SIZE), _message(
MSG_MAX_SIZE)
{
_messageId = 0;
_retries = 0;
}

int Publisher::ptRun(Msg& msg)
{
if (msg.sig() == SIG_MQTT_DISCONNECTED)
{
Msg::publish(SIG_MQTT_PUBLISH_FAILED);
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
	listen(SIG_MQTT_CONNECTED);
	PT_YIELD(&pt);
	while (true)
	{
		_state = ST_READY;
		listen(SIG_MQTT_DO_PUBLISH);
		PT_YIELD(&pt);
		if (_flags.qos == QOS_0)
		{
			sendPublish();
			Msg::publish(SIG_MQTT_PUBLISH_OK);
		}
		else if (_flags.qos == QOS_1)
		{
			_retries = 0;
			while (_retries < 4)
			{
				sendPublish();
				listen(SIG_MQTT_MESSAGE | SIG_MQTT_DISCONNECTED,
				TIME_WAIT_REPLY);
				PT_YIELD(&pt);
				if (isMqttMsg(msg, MQTT_MSG_PUBACK, _messageId))
				{
					Msg::publish(SIG_MQTT_PUBLISH_OK);
					break;
				}
			}
		}
		else if (_flags.qos == QOS_2)
		{
			_retries = 0;
			while (_retries < 4)
			{
				sendPublish();
				listen(SIG_MQTT_MESSAGE | SIG_MQTT_DISCONNECTED,
				TIME_WAIT_REPLY);
				PT_YIELD(&pt);
				if (isMqttMsg(msg, MQTT_MSG_PUBREC, _messageId))
				{
					break;
				}
			}
			if (_retries == 4)
			{
				Msg::publish(SIG_MQTT_PUBLISH_FAILED);
				break; // abandon
			}
			_retries = 0;
			while (_retries < 4)
			{
				sendPubRel();
				listen(SIG_MQTT_MESSAGE | SIG_MQTT_DISCONNECTED,
				TIME_WAIT_REPLY);
				PT_YIELD(&pt);
				if (isMqttMsg(msg, MQTT_MSG_PUBCOMP, _messageId))
				{
					Msg::publish(SIG_MQTT_PUBLISH_OK);
					break;
				}
			}
			if (_retries == 4)
			{
				Msg::publish(SIG_MQTT_PUBLISH_FAILED);
				break; // abandon
			}
		}
	}
}

PT_END(&pt)
}

bool Publisher::publish(Str& topic, Bytes& msg, Flags flags)
{
if (_state == ST_READY)
{
_retries = 0;
_topic = topic;
_message = msg;
_messageId = Mqtt::nextMessageId();
_flags = flags;
// sendPublish();
Msg::publish(SIG_MQTT_DO_PUBLISH);
_state = ST_BUSY;

return true;
}
return false;
}

void Publisher::sendPublish()
{
uint8_t header = 0;
if (_flags.qos == QOS_0)
{
_state = ST_READY;
}
else if (_flags.qos == QOS_1)
{
header += MQTT_QOS1_FLAG;
timeout(TIME_WAIT_REPLY);
}
else if (_flags.qos == QOS_2)
{
header += MQTT_QOS2_FLAG;
timeout(TIME_WAIT_REPLY);
}
if (_flags.retained)
header += MQTT_RETAIN_FLAG;
if (_retries)
{
header += MQTT_DUP_FLAG;
}
mqttOut.Publish(header, _topic, _message, _messageId);
_link.send(mqttOut);
_retries++;
}

void Publisher::sendPubRel()
{
mqttOut.PubRel(_messageId);
_link.send(mqttOut);
timeout(TIME_WAIT_REPLY);
}

//____________________________________________________________________________
//			SUBSCRIBER
//____________________________________________________________________________
Subscriber::Subscriber(Link & link) :
Handler("Subscriber"), _link(link), _topic(30), _message(MSG_MAX_SIZE)
{
_messageId = 0;
_retries = 0;
}

void Subscriber::sendPubRec()
{
mqttOut.PubRec(_messageId);
_link.send(mqttOut);
timeout(TIME_WAIT_REPLY);
}

extern PropMgr propMgr;

int Subscriber::ptRun(Msg& msg)
{
if (msg.sig() == SIG_MQTT_DISCONNECTED)
{
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
listen(SIG_MQTT_CONNECTED);
PT_YIELD(&pt);
while (true)
{
	listen(SIG_MQTT_DISCONNECTED | SIG_MQTT_MESSAGE);
	PT_YIELD(&pt);
	if (isMqttMsg(msg, MQTT_MSG_PUBLISH, 0))
	{
		if (mqttIn.qos() == MQTT_QOS0_FLAG)
		{

			propMgr.set(*mqttIn.topic(), *mqttIn.message());

		}
		else if (mqttIn.qos() == MQTT_QOS1_FLAG)
		{

			propMgr.set(*mqttIn.topic(), *mqttIn.message());
			mqttOut.PubAck(mqttIn.messageId());
			_link.send(mqttOut);

		}
		else if (mqttIn.qos() == MQTT_QOS2_FLAG)
		{

			_retries = 0;
			_topic = *mqttIn.topic();
			_message = *mqttIn.message();
			_messageId = mqttIn.messageId();
			while (_retries < 4)
			{
				sendPubRec();
				listen(SIG_MQTT_DISCONNECTED | SIG_MQTT_MESSAGE,
				TIME_WAIT_REPLY);
				PT_YIELD(&pt);
				if (isMqttMsg(msg, MQTT_MSG_PUBREL, _messageId))
				{
					propMgr.set(_topic, _message);
					mqttOut.PubComp(mqttIn.messageId());
					_link.send(mqttOut);
					break;
				}
			}

		}
	}
}
}
PT_END(&pt);
}
/*
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
 }*/
//____________________________________________________________________________
//			SUBSCRIPTION
//____________________________________________________________________________
Subscription::Subscription(Link & link) :
Handler("Subscription"), _link(link)
{
_retries = 0;
_state = ST_DISCONNECTED;
_messageId = 0;
}

int Subscription::ptRun(Msg& msg)
{
if (msg.sig() == SIG_MQTT_DISCONNECTED)
{
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
listen(SIG_MQTT_CONNECTED);	// wait forever on  MQTT connection established
PT_YIELD(&pt);

_retries = 0;
_messageId = Mqtt::nextMessageId();
while (_retries < 4)
{
sendSubscribePut();
listen(SIG_MQTT_DISCONNECTED | SIG_MQTT_MESSAGE, TIME_WAIT_REPLY);
PT_YIELD(&pt);
if (isMqttMsg(msg, MQTT_MSG_SUBACK, _messageId))
{ // successful subscribed
	break;
}
else if (msg.sig() == SIG_TIMEOUT && _retries == 3)
{
	_link.disconnect();
	restart();
	PT_YIELD(&pt);
}
}

_retries = 0;
_messageId = Mqtt::nextMessageId();
while (_retries < 4)
{
sendSubscribeGet();
listen(SIG_MQTT_DISCONNECTED | SIG_MQTT_MESSAGE, TIME_WAIT_REPLY);
PT_YIELD(&pt);
if (isMqttMsg(msg, MQTT_MSG_SUBACK, _messageId))
{ // successful subscribed
	break;
}
else if (msg.sig() == SIG_TIMEOUT && _retries == 3)
{
	_link.disconnect();
	restart();
	PT_YIELD(&pt);
}
}
listen(SIG_MQTT_DISCONNECTED);	// wait forever on  MQTT disconnection
PT_YIELD(&pt);

}
PT_END(&pt);
}

void Subscription::sendSubscribePut()
{
Str topic(100);
topic << putPrefix << "#";
mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_link.send(mqttOut);
_retries++;
}

void Subscription::sendSubscribeGet()
{
Str topic(100);
topic << getPrefix << "#";
mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_link.send(mqttOut);
_retries++;
}


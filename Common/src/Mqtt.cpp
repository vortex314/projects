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

uint16_t Mqtt::nextMessageId()
{
	return ++gMessageId;
}
/*
 bool Mqtt::msgToMqttIn(Msg& msg)
 {
 Bytes bytes;
 Signal signal;
 void* src;
 msg.rewind();
 msg.get((uint32_t&) signal);
 msg.get((uint32_t&) src);
 if (!msg.is(SIG_RXD, &_link))
 return false;
 msg.get(bytes);
 _mqttIn.remap(&bytes);
 bytes.offset(0);
 if (_mqttIn.parse())
 return true;
 return false;
 }

 bool Mqtt::isMqttMsg(Msg& msg, uint8_t msgType, uint16_t msgId)
 {
 if (msgToMqttIn(msg))
 {
 if (msgId == 0)
 return (_mqttIn.type() == msgType);
 else
 return (_mqttIn.type() == msgType) && (_mqttIn.messageId() == msgId);
 }
 return false;
 }
 */
//____________________________________________________________________________
//
//____________________________________________________________________________
void Mqtt::setPrefix(const char* prefix)
{

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
		MSG_MAX_SIZE)
{
	_retries = 0;
	timeout(100);
	setPrefix("");
	_pinger = new Pinger(*this);
	reg(_pinger);
//	_publisher = new MqttPublisher(*this);
//	_subscriber = new MqttSubscriber(*this);
//	_subscription = new Subscription(*this);
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
	Str str(8);
	str << "false";
	_mqttOut.Connect(MQTT_QOS2_FLAG, _prefix.c_str(), MQTT_CLEAN_SESSION,
			"system/online", str, "", "",
			TIME_KEEP_ALIVE / 1000);
	_link.send(_mqttOut);
}

//________________________________________________________________________________________________
//
//
//________________________________________________________________________________________________

int Mqtt::dispatch(Msg& msg)
{

	PT_BEGIN(&pt)
		while (true) // DISCONNECTED STATE
		{
			MsgQueue::publish(this, SIG_DISCONNECTED, 0, 0);

			_link.connect();
			timeout(TIME_FOREVER);
			PT_YIELD_UNTIL(&pt,
					msg.is(&_link, SIG_CONNECTED, 0, 0) || timeout()); //  wait Uart connected

			while (true) // LINK_CONNECTED
			{
				sendConnect();
				timeout(TIME_WAIT_CONNECT);
				PT_YIELD_UNTIL(&pt,
						msg.is(&_link, SIG_RXD | SIG_DISCONNECTED)
								|| timeout()); // wait reply or timeout on connect send
				if (msg.is(&_link, SIG_DISCONNECTED))
					break;
				else if (msg.is(&_link, SIG_RXD, MQTT_MSG_CONNACK, 0))
				{
					MsgQueue::publish(this, SIG_CONNECTED);
					_pinger->restart();

					while (true) // MQTT_CONNECTED
					{

						timeout( TIME_PING);

						PT_YIELD_UNTIL(&pt,
								msg.is(&_link, SIG_DISCONNECTED | SIG_RXD)
										|| timeout()); // wait for disconnect or message
						dispatchToChilds(msg);
						if (msg.is(0, SIG_DISCONNECTED))
							break;
					}
				}
				if (msg.is(0, SIG_DISCONNECTED))
					break;
			}
		}

	PT_END(&pt)
}

//____________________________________________________________________________
//			PINGER
//____________________________________________________________________________
Pinger::Pinger(Mqtt& mqtt) :
	Handler("Pinger"), _mqtt(mqtt)
{
_retries = 0;
}

int Pinger::dispatch(Msg& msg)
{
PT_BEGIN(&pt)
	while (true) // keep on pinging
	{
		_retries = 0;
		while (_retries < 3)
		{
			_mqtt._mqttOut.PingReq();
			_mqtt._link.send(_mqtt._mqttOut);
			_retries++;
			timeout(TIME_WAIT_REPLY);
			PT_YIELD_UNTIL(&pt,
					(msg.is(&_mqtt._link ,SIG_RXD,MQTT_MSG_PINGRESP, 0) || timeout()));
			if (msg.signal == SIG_RXD)
			{
				// successful ping

				timeout( TIME_PING);
				PT_YIELD_UNTIL(&pt, timeout());
				_retries = 0;

			}
			else
			{
				_retries++;
			}
		}
		_mqtt._link.disconnect();
	}
PT_END(&pt)
}
//____________________________________________________________________________
//			PUBLISHER
//____________________________________________________________________________
MqttPublisher::MqttPublisher(Mqtt& mqtt) :
Handler("MqttPublisher"), _mqtt(mqtt), _topic(TOPIC_MAX_SIZE), _message(
MSG_MAX_SIZE)
{
_messageId = 0;
_retries = 0;
}

int MqttPublisher::dispatch(Msg& msg)
{
if (msg.signal == SIG_DISCONNECTED)
{
MsgQueue::publish(&_mqtt, SIG_ERC);
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
	PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_CONNECTED));
	while (true)
	{
		_state = ST_READY;
		PT_YIELD_UNTIL(&pt, msg.is(this, SIG_START));
		if (_flags.qos == QOS_0)
		{
			sendPublish();
			MsgQueue::publish(this, SIG_SUCCESS);
		}
		else if (_flags.qos == QOS_1)
		{
			_retries = 0;
			while (_retries < 4)
			{
				sendPublish();
				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD ,MQTT_MSG_PUBACK,0));
				if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBACK, 0))
				{
					MsgQueue::publish(this, SIG_SUCCESS); //TODO check _messageId
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
				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD ,MQTT_MSG_PUBREC,0));
				if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBREC, 0))
				{
					break;
				}
			}
			if (_retries == 4)
			{
				MsgQueue::publish(this, SIG_FAIL);
				break; // abandon
			}
			_retries = 0;
			while (_retries < 4)
			{
				sendPubRel();
				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD ,MQTT_MSG_PUBCOMP,0));
				if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBCOMP, 0))
				{
					MsgQueue::publish(this, SIG_SUCCESS);
					break;
				}
			}
			if (_retries == 4)
			{
				MsgQueue::publish(this, SIG_FAIL);
				break; // abandon
			}
		}
	}
}

PT_END(&pt)
}

bool MqttPublisher::publish(Str& topic, Bytes& msg, Flags flags)
{
if (_state == ST_READY)
{
_retries = 0;
_topic = topic;
_message = msg;
_messageId = Mqtt::nextMessageId();
_flags = flags;
// sendPublish();
MsgQueue::publish(this, SIG_START);
_state = ST_BUSY;

return true;
}
return false;
}

void MqttPublisher::sendPublish()
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
_mqtt._mqttOut.Publish(header, _topic, _message, _messageId);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}

void MqttPublisher::sendPubRel()
{
_mqtt._mqttOut.PubRel(_messageId);
_mqtt._link.send(_mqtt._mqttOut);
timeout(TIME_WAIT_REPLY);
}

//____________________________________________________________________________
//			SUBSCRIBER
//____________________________________________________________________________
MqttSubscriber::MqttSubscriber(Mqtt &mqtt) :
Handler("Subscriber"), _mqtt(mqtt), _topic(30), _message(MSG_MAX_SIZE)
{
_messageId = 0;
_retries = 0;
}

void MqttSubscriber::sendPubRec()
{
_mqtt._mqttOut.PubRec(_messageId);
_mqtt._link.send(_mqtt._mqttOut);
timeout(TIME_WAIT_REPLY);
}

#define PT_WAIT_FOR( ___pt, ___signals, ___timeout ) listen(___signals,___timeout);PT_YIELD(___pt);

int MqttSubscriber::dispatch(Msg& msg)
{
if (msg.signal == SIG_DISCONNECTED)
{
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_CONNECTED));
while (true)
{
	PT_YIELD_UNTIL(&pt,
			msg.is(&_mqtt,SIG_RXD | SIG_DISCONNECTED ,MQTT_MSG_PUBLISH,0));
	if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBLISH, 0))
	{
		if (_mqtt._mqttIn.qos() == MQTT_QOS0_FLAG)
		{
			MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn);
		}
		else if (_mqtt._mqttIn.qos() == MQTT_QOS1_FLAG)
		{

			MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn);
			_mqtt._mqttOut.PubAck(_mqtt._mqttIn.messageId());
			_mqtt._link.send(_mqtt._mqttOut);

		}
		else if (_mqtt._mqttIn.qos() == MQTT_QOS2_FLAG)
		{

			_retries = 0;
			_topic = *_mqtt._mqttIn.topic();
			_message = *_mqtt._mqttIn.message();
			_messageId = _mqtt._mqttIn.messageId();
			while (_retries < 4)
			{
				sendPubRec();
				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL(&pt, msg.is(&_mqtt,SIG_RXD ,MQTT_MSG_PUBREL,0));
				if (msg.is(&_mqtt, SIG_RXD, MQTT_MSG_PUBREL, 0))
				{
					MsgQueue::publish(&_mqtt, SIG_RXD, 0, &_mqtt._mqttIn); //TODO
					_mqtt._mqttOut.PubComp(_mqtt._mqttIn.messageId());
					_mqtt._link.send(_mqtt._mqttOut);
					break;
				}
			}

		}
	}
}
}
PT_END(&pt);
}

//____________________________________________________________________________
//			SUBSCRIPTION
//____________________________________________________________________________
Subscription::Subscription(Mqtt &mqtt) :
Handler("Subscription"), _mqtt(mqtt)
{
_retries = 0;
_state = ST_DISCONNECTED;
_messageId = 0;
// listen(&_mqtt);
}

int Subscription::dispatch(Msg& msg)
{
if (msg.signal == SIG_DISCONNECTED)
{
restart();
return 0;
}
PT_BEGIN(&pt)
while (true)
{
// listen(SIG_CONNECTED);	// wait forever on  MQTT connection established
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_CONNECTED));

_retries = 0;
_messageId = Mqtt::nextMessageId();
while (_retries < 4)
{
sendSubscribePut();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_DISCONNECTED | SIG_RXD) || timeout());
if (_mqtt.isMqttMsg(msg, MQTT_MSG_SUBACK, _messageId))
{
// successful subscribed
	break;
}
else if (msg.signal == SIG_TIMEOUT && _retries == 3)
{
	_mqtt._link.disconnect();
	restart();
	PT_YIELD(&pt);
}
}

_retries = 0;
_messageId = Mqtt::nextMessageId();
while (_retries < 4)
{
sendSubscribeGet();
timeout(TIME_WAIT_REPLY);
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_DISCONNECTED | SIG_RXD) || timeout());
if (_mqtt.isMqttMsg(msg, MQTT_MSG_SUBACK, _messageId))
{
// successful subscribed
	break;
}
else if (msg.signal == SIG_TIMEOUT && _retries == 3)
{
	_mqtt._link.disconnect();
	restart();
	PT_YIELD(&pt);
}
}
 // wait forever on  MQTT disconnection
PT_YIELD_UNTIL(&pt, msg.is(&_mqtt, SIG_DISCONNECTED));

}
PT_END(&pt);
}

void Subscription::sendSubscribePut()
{
Str topic(100);
topic << _mqtt._putPrefix << "#";
_mqtt._mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}

void Subscription::sendSubscribeGet()
{
Str topic(100);
topic << _mqtt._getPrefix << "#";
_mqtt._mqttOut.Subscribe(MQTT_QOS1_FLAG, topic, _messageId, QOS_2);
_mqtt._link.send(_mqtt._mqttOut);
_retries++;
}


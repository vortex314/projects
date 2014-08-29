/*
 * MqttSeq.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Mqtt.h"

//*************************************************** GLOBAL VARS *************

Log log;
MqttIn mqttIn (MAX_MSG_SIZE);
MqttOut mqttOut (MAX_MSG_SIZE);
Str putPrefix (20);
Str getPrefix (20);
Str prefix (20);
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
class MqttPing : public Sequence
{
private:
  struct pt t;
  uint16_t _retryCount;
  Mqtt& _mqtt;
public:

  MqttPing (Mqtt& mqtt);

  int
  handler (Event* event);
};

/*****************************************************************************
 *   HANDLE MQTT send publish message with qos=2 and 1
 ******************************************************************************/

class MqttPub : public Sequence
{
private:
  struct pt t;
  uint16_t _messageId;
  Flags _flags;
  Str _topic;
  Str _message;
  uint16_t _id;
  bool _isReady;
  uint8_t _retryCount;
  Mqtt& _mqtt;
public:
  static const int PUBLISH_OK, PUBLISH_FAILED;
  MqttPub (Mqtt& mqtt);
  void
  send (Flags flags, uint16_t id, Str& topic, Str& message);
  void
  Publish ();
  bool
  isReady ();
  int
  handler (Event* event);

};

/*****************************************************************************
 *   HANDLE Property scan for changes
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE MQTT received publish message with qos=2
 ******************************************************************************/
class MqttSubQos0 : public Sequence
{
private:
  struct pt t;
  Mqtt& _mqtt;
public:
  MqttSubQos0 (Mqtt& mqtt);
  int
  handler (Event* event);
};

class MqttSubQos1 : public Sequence
{
private:
  struct pt t;
  Mqtt& _mqtt;
public:
  MqttSubQos1 (Mqtt& mqtt);
  int
  handler (Event* event);

};

class MqttSubQos2 : public Sequence
{
private:
  struct pt t;
  uint16_t _messageId;
  Mqtt& _mqtt;
  MqttIn save;
  bool _isReady;

public:
  MqttSubQos2 (Mqtt& mqtt);
  int
  handler (Event* event);
};

/*****************************************************************************
 *   Scan for topic changes and publish with right qos and retain
 ******************************************************************************/

#include "Usb.h"
/*****************************************************************************
 *   Generate next message id
 ******************************************************************************/

uint16_t
nextMessageId ()
{
  return gMessageId++;
}

void
GlobalInit ()
{

  prefix << "Stellaris" << "/";
  mqttOut.prefix (prefix);
  getPrefix << "GET/" << prefix;
  putPrefix << "PUT/" << prefix;
}
;
/*****************************************************************************
 *   MQTT EVENT filter
 ******************************************************************************/

bool
Mqtt::isEvent (Event* event, uint8_t type, uint16_t messageId, uint8_t qos)
{
  if (event->id () != Link::MESSAGE)
    return false;
  MqttIn* mqttIn;
  mqttIn = recv ();
  if (mqttIn == 0)
    return false;
  mqttIn->parse ();
  if (type)
    if (mqttIn->type () != type)
      return false;
  if (messageId)
    if (mqttIn->messageId () != messageId)
      return false;
  if (qos)
    if (mqttIn->qos () != qos)
      return false;
  return true;
}

bool
Mqtt::Publish (Flags flags, uint16_t id, Str& topic, Strpack& strp)
{
  return _mqttPub->send (flags, id, topic, strp);
}

Mqtt::Mqtt (Link& link) :
    str (30), msg (10), _link (link)
{
  mqttPing = new MqttPing (*this);
  mqttSubQos0 = new MqttSubQos0 (*this);
  mqttSubQos1 = new MqttSubQos1 (*this);
  mqttSubQos2 = new MqttSubQos2 (*this);
  _mqttPub = new MqttPub (*this);

  PT_INIT(&t);
  _messageId = 2000;
  _isConnected = false;
  new MqttPing (*this);
  GlobalInit ();
}

int
Mqtt::handler (Event* event)
{
  PT_BEGIN(&t)
    ;
    while (1)
      {
	_messageId = nextMessageId ();

	PT_YIELD_UNTIL(&t, _link.isConnected ());
	mqttOut.Connect (MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
			 "system/online", "false", "", "",
			 TIME_KEEP_ALIVE / 1000);
	_link.send (mqttOut);

	timeout (5000);
	PT_YIELD_UNTIL(&t,
		       event->is(Link::MESSAGE, MQTT_MSG_CONNACK) || timeout());
	if (timeout ())
	  {
	    publish (DISCONNECTED);
	    continue;
	  }

	str.clear () << putPrefix << "#";
	mqttOut.Subscribe (0, str, ++_messageId, MQTT_QOS1_FLAG);
	_link.send (mqttOut);

	str.clear () << getPrefix << "#";
	mqttOut.Subscribe (0, str, ++_messageId, MQTT_QOS1_FLAG);
	_link.send (mqttOut);

	str = "system/online";
	msg = "true";
	mqttOut.Publish (MQTT_QOS1_FLAG, str, msg, ++_messageId);
	_link.send (mqttOut);

	timeout (TIME_WAIT_REPLY);
	PT_YIELD_UNTIL(&t,
		       event->is(Link::MESSAGE, MQTT_MSG_PUBACK) || timeout());
	// isEvent(event, MQTT_MSG_PUBACK, _messageId, 0) || timeout());
	if (timeout ())
	  {
	    publish (DISCONNECTED);
	    continue;
	  }
	publish (CONNECTED);
	_isConnected = true;
	PT_YIELD_UNTIL(&t, event->is (Link::DISCONNECTED));

	publish (DISCONNECTED);
	_isConnected = false;
      }
  PT_END(&t);
return 0;
}

Erc
Mqtt::send (Bytes& pb)
{
if (_link.isConnected ())
  return _link.send (pb);
else
  return E_AGAIN;
}

MqttIn*
Mqtt::recv ()
{
return _link.recv ();
}

bool
Mqtt::isConnected ()
{
return _isConnected;
}

Erc
Mqtt::disconnect ()
{
return E_OK;
}

const int Mqtt::CONNECTED = Event::nextEventId ("MQTT_CONNECTED");
const int Mqtt::DISCONNECTED = Event::nextEventId ("MQTT_DISCONNECTED");
const int Mqtt::MESSAGE = Event::nextEventId ("Mqtt::MESSAGE");

/*****************************************************************************
 *   HANDLE PING keep alive
 ******************************************************************************/

MqttPing::MqttPing (Mqtt& mqtt) :
  _mqtt (mqtt)
{
PT_INIT(&t);
}

int
MqttPing::handler (Event* event)
{
PT_BEGIN(&t)
  ; //
  while (true)
    {
      _retryCount = 0;
      while (_mqtt.isConnected ())
	{
	  mqttOut.PingReq ();
	  _mqtt.send (mqttOut);

	  timeout (TIME_WAIT_REPLY);
	  PT_YIELD_UNTIL(
	      &t, timeout() || _mqtt.isEvent(event, MQTT_MSG_PINGRESP, 0, 0));
	  if (timeout ())
	    {
	      _retryCount++;
	      if (_retryCount >= 3)
		{
		  _mqtt.disconnect ();
		  PT_RESTART(&t);
		}
	    }
	  else
	    _retryCount = 0;
	  timeout (TIME_PING);
	  PT_YIELD_UNTIL(&t, timeout ());
	  // sleep between pings
	}
      PT_YIELD(&t);
    }
PT_END(&t);
}

/*****************************************************************************
 *   HANDLE MQTT send publish message with qos=2 and 1
 ******************************************************************************/

MqttPub::MqttPub (Mqtt& mqtt) :
_topic (100), _message (100), _mqtt (mqtt)
{
PT_INIT(&t);
_messageId = 1000;
_isReady = true;
_retryCount = 0;
}

void
MqttPub::send (Flags flags, uint16_t id, Str& topic, Str& message)
{
_messageId = nextMessageId ();
_topic = topic;
_message = message;
_flags = flags;
_id = id;
_isReady = false;
}

void
MqttPub::Publish ()
{
uint8_t header = 0;
if (_flags.qos == QOS_1)
{
  header += MQTT_QOS1_FLAG;
}
else if (_flags.qos == QOS_2)
{
  header += MQTT_QOS2_FLAG;
}
if (_flags.retained)
header += MQTT_RETAIN_FLAG;
if (_retryCount)
header += MQTT_DUP_FLAG;
mqttOut.Publish (header, _topic, _message, _messageId);
_mqtt.send (mqttOut);
}

bool
MqttPub::isReady ()
{
return _isReady;
}
/*
 *
 */

int
MqttPub::handler (Event* event)
{

PT_BEGIN(&t)
;
while (true)
  {
    PT_YIELD_UNTIL(&t, _isReady == false);
    if (_flags.qos == QOS_0)
      {
	_retryCount = 0;
	Publish ();
      }
    else if (_flags.qos == QOS_1)
      {
	_retryCount = 0;
	while (_retryCount < 3)
	  {
	    Publish ();
	    PT_YIELD_UNTIL(
		&t,
		timeout() || _mqtt.isEvent(event, MQTT_MSG_PUBACK, _messageId, 0));
	    if (_mqtt.isEvent (event, MQTT_MSG_PUBACK, _messageId, 0))
	      break;
	  }
	if (_retryCount == 3)
	  publish (PUBLISH_FAILED, _id);
	else
	  publish (PUBLISH_OK, _id);

      }
    else if (_flags.qos == QOS_2)
      {
	_retryCount = 0;
	while (_retryCount < 3)
	  {
	    Publish ();
	    PT_YIELD_UNTIL(
		&t,
		timeout() || _mqtt.isEvent(event, MQTT_MSG_PUBREC, _messageId, 0));
	    if (_mqtt.isEvent (event, MQTT_MSG_PUBREC, _messageId, 0))
	      break;
	    else
	      _retryCount++;
	  }
	if (_retryCount == 3)
	  {
	    publish (PUBLISH_FAILED, _id);
	    break;
	  }
	_retryCount = 0;
	while (_retryCount < 3)
	  {
	    mqttOut.PubRel (_messageId);
	    _mqtt.send (mqttOut);

	    timeout (TIME_WAIT_REPLY);
	    PT_YIELD_UNTIL(
		&t,
		timeout() || _mqtt.isEvent(event, MQTT_MSG_PUBCOMP, _messageId, 0));
	    if (timeout ())
	      _retryCount++;
	    else
	      break;
	  };
	if (_retryCount == 3)
	  {
	    publish (PUBLISH_FAILED, _id);
	    break;
	  }
      }

  }
PT_END(&t)
;
}

/*****************************************************************************
 *   HANDLE Property scan for changes
 ******************************************************************************/

/*****************************************************************************
 *   HANDLE MQTT received publish message with qos=2
 ******************************************************************************/

MqttSubQos0::MqttSubQos0 (Mqtt& mqtt) :
_mqtt (mqtt)
{
PT_INIT(&t);
}
int
MqttSubQos0::handler (Event* event)
{
PT_BEGIN(&t)
;
while (true)
{
  PT_YIELD_UNTIL(&t, _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, 0));
  MqttIn* mqttIn = _mqtt.recv ();
  if (mqttIn->qos () == 0)
    Prop::set (*mqttIn->topic (), *mqttIn->message (), mqttIn->qos ());
}
PT_END(&t);
}

MqttSubQos1::MqttSubQos1 (Mqtt& mqtt) :
_mqtt (mqtt)
{
PT_INIT(&t);
}
int
MqttSubQos1::handler (Event* event)
{
PT_BEGIN(&t)
;
while (true)
{
PT_YIELD_UNTIL(&t, _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, MQTT_QOS1_FLAG));
MqttIn* mqttIn = _mqtt.recv ();
mqttOut.PubAck (mqttIn->messageId ()); // send PUBACK
_mqtt.send (mqttOut);
Prop::set (*mqttIn->topic (), *mqttIn->message (), mqttIn->qos ());

}
PT_END(&t);
}

MqttSubQos2::MqttSubQos2 (Mqtt& mqtt) :
_mqtt (mqtt), save (MAX_MSG_SIZE)
{
_messageId = 0;
_isReady = true;
PT_INIT(&t);
}
int
MqttSubQos2::handler (Event* event)
{
MqttIn* mqttIn;
PT_BEGIN(&t)
;
while (true)
{
PT_YIELD_UNTIL(&t, _mqtt.isEvent(event, MQTT_MSG_PUBLISH, 0, MQTT_QOS2_FLAG));
_isReady = false;
mqttIn = _mqtt.recv ();
_messageId = mqttIn->messageId ();

mqttOut.PubRec (_messageId);
_mqtt.send (mqttOut);

save.clone (*_mqtt.recv ()); // clone the message
timeout (TIME_WAIT_REPLY);
PT_YIELD_UNTIL(
  &t, timeout() || _mqtt.isEvent(event, MQTT_MSG_PUBREL, _messageId, 0));
if (timeout ())
{
  _isReady = true;
  continue; // abandon and go for destruction
}

mqttOut.PubComp (_messageId);
_mqtt.send (mqttOut);
Prop::set (*save.topic (), *save.message (), save.qos ());
_isReady = true;
}
PT_END(&t);
}


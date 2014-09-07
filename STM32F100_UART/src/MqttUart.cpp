/*

 * MqttStream.cpp

 *

 *  Created on: 10-okt.-2013

 *      Author: lieven2

 */

#include "MqttUart.h"
#include "Property.h"
#include "Led.h"
#include "pt.h"

#define CONNECTION_ID "STM32F100_UART"
#define PREFIX "/A/"
#define DOMAIN "Test/"
#define DEVICE "STM32/"
#define SUBSCRIBE_PREFIX  PREFIX "_set_/" DOMAIN DEVICE
#define SUBSCRIBE SUBSCRIBE_PREFIX "#"
#define PUBLISH  PREFIX "_get_/" DOMAIN DEVICE
#define META  PREFIX "_info_/" DOMAIN DEVICE
#define ALIVE "system/alive"
extern Led ledGreen;

MqttUart::MqttUart(Uart* uart) :

		_timer(this), _timerUpdateAll(this), _uartIn(256), _uartOut(256) {
	_timer.interval(1000);
	_timerUpdateAll.interval(1000);
	_timerUpdateAll.reload(true);
	_timerUpdateAll.start();
	_uart = uart;
	_uart->upStream(this);
	_state = ST_CONNECT;
	_index = 0;
	_retries = 0;
	_messageId = 0;
	_messagesRxd = 0;
	_messagesTxd = 0;
	_listener = NULL;
	Property::updatedAll();
	_lastUpdateAll = Sys::upTime();
}

MqttUart::~MqttUart() {
	ASSERT(false);

}

Property *uartProperty;
class Now: public PackerInterface {

	Erc fromPack(Strpack& str) {
		uint64_t val;
		if (str.parse(&val))
			return E_INVAL;
		Sys::_bootTime = val - Sys::_upTime;
		return E_OK;
	}

	Erc toPack(Strpack& str) {
		uint64_t val = Sys::_bootTime + Sys::_upTime;
		str.clear();
		str.append(val);
		return E_OK;
	}
};

Now localTime;

bool alive = true;
extern Uart uart1;
extern MqttUart mqtt;

const PropertyConst p[] = { //
				{ { &alive }, "system/alive", "", T_BOOL, M_READ, I_ADDRESS }, //
				{ { &Sys::_upTime }, "system/upTime", "", T_UINT64, M_READ,
						I_ADDRESS }, //
				{ { &localTime }, "system/now", "", T_UINT64, M_RW, I_INTERFACE }, //
				{ { &Sys::_bootTime }, "system/bootTime", "", T_UINT64, M_RW,
						I_ADDRESS }, //
				{ { &Sys::_log }, "system/log", "", T_BYTES, M_READ, I_ADDRESS }, //
				{ { &uart1._bytesTxd }, "uart/1/bytesTxd", "", T_UINT32, M_READ,
						I_ADDRESS }, //
				{ { &uart1._bytesRxd }, "uart/1/bytesRxd", "", T_UINT32, M_READ,
						I_ADDRESS }, //
				{ { &uart1._commErrors }, "uart/1/commErrors", "", T_UINT32,
						M_READ, I_ADDRESS }, //
				{ { &mqtt._messagesRxd }, "mqtt/messagesRxd", "", T_UINT32,
						M_READ, I_ADDRESS }, //
				{ { &mqtt._messagesTxd }, "mqtt/messagesTxd", "", T_UINT32,
						M_READ, I_ADDRESS } //
		};
void MqttUart::init() {
	uint32_t i;
	PT_INIT(&pt);
	_timer.start();
	_state = ST_CONNECT;
	ledGreen.blink(5);
	for (i = 0; i < (sizeof(p) / sizeof(PropertyConst)); i++)
		new Property(&p[i]);
}

#define SES(__status,__event)  (( event.clsType()== __event ) && _state==__status)
#define SE(__status,__event)  ( (__event & 0xFFFF0000) + __status)

char MqttUart::thread(Event& event) {
	_timer.start(1000);
	PT_BEGIN(&pt)
		while (true) {
			while (true) {
				sendMsg();
				PT_YIELD(&pt);
				if (event.is(Timer::EXPIRED)) {
					_retries++;
					if (_retries > MAX_RETRIES) {
						ledGreen.blink(5);
						_state = ST_CONNECT;
						_retries = 0;
					}
				}
				if (event.src() == this)
					if (validatedReply(event))
						if (!next())
							break;
			}
			PT_WAIT_UNTIL(&pt, event.is(Timer::EXPIRED));
		}
	PT_END(&pt);
return 0;
}

Str topic(100);
Strpack msg(256);

void MqttUart::sendMsg() {
switch (_state) {
case ST_DISCONNECT: {
	_uartOut.Disconnect();
	break;
}
case ST_CONNECT: {
	_uartOut.Connect(0,
	CONNECTION_ID,
	MQTT_WILL_QOS1 | MQTT_WILL_FLAG | MQTT_WILL_RETAIN | MQTT_CLEAN_SESSION,
	PUBLISH ALIVE, (const char*) "false", (const char*) "", (const char*) "",
	KEEP_ALIVE);
	break;
}
case ST_SUBSCRIBE: {
	_uartOut.Subscribe(0, SUBSCRIBE, _messageId, MQTT_QOS1_FLAG);
	break;
}
case ST_PUB_META: {
	topic.clear();
	msg.clear();
	Property *prop = Property::get(_index);
	prop->getMeta(msg);
	topic.append(META);
	topic.append(prop->name());
	uint8_t hdr = MQTT_QOS1_FLAG + MQTT_RETAIN_FLAG;
	if (_retries > 0)
		hdr |= MQTT_DUP_FLAG;
	_uartOut.Publish(hdr, topic, msg, _messageId);
	break;
}
case ST_PUB_DATA: {
	topic.clear();
	msg.clear();
	Property *prop = Property::findNextUpdated();
//	Property *prop = Property::get(_index);
	if (prop == NULL)
		break;
	prop->toPack(msg);
	topic.append(PUBLISH);
	topic.append(prop->name());
	uint8_t hdr = MQTT_QOS1_FLAG + MQTT_RETAIN_FLAG;
	if (_retries > 0)
		hdr |= MQTT_DUP_FLAG;
	_uartOut.Publish(hdr, topic, msg, _messageId);
	prop->published();
	break;
}
case ST_SLEEP: {
	break;
}
}
send();
}

bool MqttUart::validatedReply(Event& event) {
switch (_state) {
case ST_SLEEP:
case ST_DISCONNECT: {
	return true;
}
case ST_CONNECT: {
	if (event.is(EV_CONNACK)) {
		ledGreen.blink(1);
		return true;
	}
	break;
}
case ST_SUBSCRIBE: {
	if (event.is(EV_SUBACK) && event.detail() == _messageId)
		return true;
	break;
}
case ST_PUB_DATA:
case ST_PUB_META: {
	if (event.is(EV_PUBACK) && event.detail() == _messageId)
		return true;
	break;
}
}
return false;
}

bool MqttUart::next() {
switch (_state) {
case ST_DISCONNECT: {
	_state = ST_CONNECT;
	ledGreen.blink(1);
	break;
}
case ST_CONNECT: { // remains in same state and index
	_index = 0;
	_state = ST_SUBSCRIBE;
	break;
}
case ST_SUBSCRIBE: {
	_index++;
	if (_index > Property::count()) {
		_state = ST_PUB_META;
		_index = 0;
	}
	break;
}
case ST_PUB_META: {
	_index++;
	if (_index >= Property::count()) {
		_index = 0;
		_state = ST_PUB_DATA;
	}
	break;
}
case ST_PUB_DATA: { // take next updated property or send "system/alive"
	if (Property::findNextUpdated())
		break;
	_state = ST_SLEEP;
	_index = 0;
	return false;
	break;
}
case ST_SLEEP: {
	_state = ST_PUB_DATA;
	_index = 0;
	break;
}
}
_retries = 0;
return true;
}

Erc MqttUart::event(Event& event) {
if (event.src() == &_timerUpdateAll) {
	Property::updatedAll();
	return E_OK;
}
if (event.clsType() == Uart::RXD) {
	uartRxd();
	return E_OK;
}
if (event.clsType() == MqttUart::EV_MESSAGE) {
	_messagesRxd++;
	if (_uartIn.type() == MQTT_MSG_CONNACK && _uartIn._returnCode == 0)
		queue(this, EV_CONNACK);
	else if (_uartIn.type() == MQTT_MSG_SUBACK)
		queue(this, EV_SUBACK + _uartIn.messageId());
	else if (_uartIn.type() == MQTT_MSG_PUBACK)
		queue(this, EV_PUBACK + _uartIn.messageId());
	else if (_uartIn.type() == MQTT_MSG_PINGRESP) {
		// ignore
	} else if (_uartIn.type() == MQTT_MSG_PUBLISH) {
		updateProperty(_uartIn);
	} else
		Sys::log(__FILE__ "__LINE__ : invalid Message Type ");
	_uartIn.reset();
	return E_OK;
}
thread(event);
return E_OK;
}

bool MqttUart::retriesExceeded() {
return _retries > MAX_RETRIES;
}

bool MqttUart::connect() {
_uartOut.Connect(0,
CONNECTION_ID,
MQTT_WILL_QOS1 | MQTT_WILL_FLAG | MQTT_WILL_RETAIN | MQTT_CLEAN_SESSION,
PUBLISH ALIVE, (const char*) "false", (const char*) "", (const char*) "",
KEEP_ALIVE);
send();
return true;
}

bool MqttUart::publishData() {

// publish _index property
uint8_t nm[100];
uint8_t val[100];
Str name(nm, 100);
Strpack value(val, 100);
value.clear();
name.clear();
Property *prop = Property::get(_index);
prop->toPack(value);
name.append(PUBLISH);
name.append(prop->name());
_uartOut.Publish(MQTT_QOS1_FLAG + MQTT_RETAIN_FLAG, name, value, _messageId);
send();
return true;
}

bool MqttUart::updateProperty(MqttIn& msg) {
Property* prop = Property::find(SUBSCRIBE_PREFIX, _uartIn._topic);

if (prop) {
	prop->fromPack(_uartIn._message);
	prop->updated();
	return true;
};
if (_uartIn._header & MQTT_QOS_MASK) {
	_uartOut.PubAck(_uartIn._messageId);
	send();
}
if (prop)
	return true;
return false;
}

bool MqttUart::publishMeta() {

// publish _index property
uint8_t nm[100];
uint8_t val[100];
Str name(nm, 100);
Str meta(val, 100);
meta.clear();
name.clear();
Property *prop = Property::get(_index);
prop->getMeta(meta);
name.append(META);
name.append(prop->name());
_uartOut.Publish(MQTT_QOS1_FLAG + MQTT_RETAIN_FLAG, name, meta, _messageId);
send();
return true;
}

bool MqttUart::subscribe() { // subscribe _index property
_uartOut.Subscribe(0, SUBSCRIBE, _messageId, MQTT_QOS1_FLAG);
send();
return true;
}

bool MqttUart::send() {
_messagesTxd++;
_uartOut.AddCrc();
_uartOut.Encode();
_uartOut.Frame();
_uart->write(_uartOut);
_uartOut.clear();
return true;
}

void MqttUart::uartRxd() { // check completeness via SOF / CRC / mqttMessage
uint8_t ch;
while (_uart->hasData() && _uartIn.complete() == false) {
	ch = _uart->read();
	if (ch == SOF) {
		if (_uartIn.length() > 0) {
			_uartIn.Decode();
			if (_uartIn.isGoodCrc()) {
				_uartIn.RemoveCrc();
				_uartIn.offset(0);
				_uartIn.parse();
				_uartIn.complete(true);
				queue(this, MqttUart::EV_MESSAGE);
				return;
//                if (_uart->hasData()) queue(this, Uart::RXD); // more data to read
			} else
				_uartIn.reset();
		}
	} else // add byte
	{
		if (_uartIn.hasSpace()) {
			_uartIn.write(ch);
		}
	}
}
}

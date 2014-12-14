/*
 * MqttIn.cpp
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#include "MqttIn.h"
//#include "Property.h"
#include <cstring>

#define LOG(x) std::cout << Sys::upTime() << " | MQTT  IN " << x << std::endl
const char* const MqttNames[] = { "UNKNOWN", "CONNECT", "CONNACK", "PUBLISH",
                                  "PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
                                  "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT"
                                };
const char* const QosNames[] = { "QOS0", "QOS1", "QOS2" };
MqttIn::MqttIn(int size) :
    Bytes(size), _topic(0), _message(0)   //+++ len=0
{
    _remainingLength = 0;
    reset();
}

MqttIn::MqttIn(MqttIn& src) :
    Bytes(src.capacity()), _topic(0), _message(0)   //+++ len=0
{
    memcpy(this, &src, sizeof(MqttIn));
    this->_start = new uint8_t[src._capacity];
    memcpy(_start, src._start, src._limit);
    _topic = src._topic;
}

MqttIn::~MqttIn()
{

}
uint8_t MqttIn::type()
{
    return _header & MQTT_TYPE_MASK;
}

uint8_t MqttIn::qos()
{
    return _header & MQTT_QOS_MASK;
}

uint16_t MqttIn::messageId()
{
    return _messageId;
}

Str* MqttIn::topic()
{
    return &_topic;
}

Bytes* MqttIn::message()
{
    return &_message;
}

void MqttIn::reset()
{
    _recvState = ST_HEADER;
    clear();

}

void MqttIn::add(uint8_t data)
{
    write(data);
    if (_recvState == ST_HEADER)
    {
        _header = data;
        _recvState = ST_LENGTH;
        _remainingLength = 0;
    }
    else if (_recvState == ST_LENGTH)
    {
        if (addRemainingLength(data) == false)   // last byte read for length
        {

            _recvState = ST_PAYLOAD;
            _lengthToRead = _remainingLength;
            if (_remainingLength == 0)
                _recvState = ST_COMPLETE;
        }
    }
    else if (_recvState == ST_PAYLOAD)
    {
        _lengthToRead--;
        if (_lengthToRead == 0)
        {
            _recvState = ST_COMPLETE;
        }
    }
    else if (_recvState == ST_COMPLETE)
        while (1)
            ;
}

bool MqttIn::complete()
{
    return (_recvState == ST_COMPLETE);
}

void MqttIn::complete(bool b)
{
    if (b)
        _recvState = ST_COMPLETE;
}

void MqttIn::readUint16(uint16_t* pi)
{
    *pi = read() << 8;
    *pi += read();
}

void MqttIn::readUtf(Str* str)
{
    uint16_t length;
    int i;
    str->clear();
    readUint16(&length);
    for (i = 0; i < length; i++)
    {
        str->write(read());
    }
}

void MqttIn::readBytes(Bytes* b, int length)
{
    int i;
    b->clear();
    for (i = 0; i < length; i++)
    {
        b->write(read());
    }
}

bool MqttIn::addRemainingLength(uint8_t data)
{
    _remainingLength <<= 7;
    _remainingLength += (data & 0x7F);
    return (data & 0x80);
}

void MqttIn::toString(Str& str)
{
    str.append("mqttIn : { type : ").append(MqttNames[type() >> 4]);
    str.append(", qos : ").append(QosNames[(_header & MQTT_QOS_MASK) >> 1]);
    str.append(", retain : ").append((_header & 0x1) > 0);
    str << " messageId : ";
    str << _messageId;

    if (type() == MQTT_MSG_PUBLISH)
    {
        str << (const char*)", topic : ";
        str << _topic;
        str << ", message : ";
        Cbor cbor(_message);
        cbor.toString(str);
    }
    else if (type() == MQTT_MSG_SUBSCRIBE)
    {
        str << ", topic : " ;
        str << _topic;
    }
    else if (type() == MQTT_MSG_CONNECT)
    {
        str << ", willTopic : " << _topic;
        str << ", willMessage : " ;
        _message.toString(str);
    }
    str.append(" }");
}

void MqttIn::parse()
{
    ASSERT(length() >= 2); //+++
    offset(0);
    _header = read();
    _remainingLength = 0;
    while (addRemainingLength(read()))
        ;
    switch (_header & 0xF0)
    {
    case MQTT_MSG_CONNECT:
    {
        Str protocol(8);
        readUtf(&protocol);
        read(); // version
        uint8_t  connectFlags=read();
        uint16_t keepAliveTimer;
        readUint16(&keepAliveTimer);
        Str clientId(23);
        readUtf(&clientId);
        if ( connectFlags & MQTT_WILL_FLAG)
        {
            uint16_t length;
            readUint16(&length);
            _topic.map(data() + offset(), length);
            readUint16(&length);
            _message.map(data() + offset(), length);
        }
        Str userName(100);
        Str password(100);
        if ( connectFlags & MQTT_USERNAME_FLAG ) readUtf(&userName);
        if ( connectFlags & MQTT_PASSWORD_FLAG ) readUtf(&password);

        break;
    }
    case MQTT_MSG_CONNACK:
    {
        read();
        _returnCode = read();
        break;
    }
    case MQTT_MSG_PUBLISH:
    {
        uint16_t length;
        readUint16(&length);
        _topic.map(data() + offset(), length); // map topic Str to this part of payload
        move(length);							// skip topic length
        int rest = _remainingLength - length - 2;
        if (_header & MQTT_QOS_MASK)   // if QOS > 0 load messageID from payload
        {
            readUint16(&_messageId);
            rest -= 2;
        }
        _message.map(data() + offset(), rest); // map message to rest of payload
        break;
    }
    case MQTT_MSG_SUBSCRIBE:
    {
        readUint16(&_messageId);
        uint16_t length;
        readUint16(&length);
        _topic.map(data()+offset(),length);
        break;
    }
    case MQTT_MSG_SUBACK:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PUBACK:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PUBREC:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PUBREL:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PUBCOMP:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PINGREQ:
    case MQTT_MSG_PINGRESP:
    {
        break;
    }
    default:
    {
    	Sys::warn(EINVAL,"MQTTIN_TYPE");
        break;// ignore bad package
    }
    }
}

// put Active Objects global
// check malloc used after init ?
// stack or global ?
// test MqttPub
// all Stellaris dependent in one file
// publish CPU,FLASH,RAM
// Usb recv can be Bytes instead of MqttIn / drop parse
// try Usb::disconnect() => UsbCDCTerm()
//


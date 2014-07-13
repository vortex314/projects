/*
 * MqttIn.cpp
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#include "MqttIn.h"
#include "Property.h"

MqttIn::MqttIn(int size) :
    Bytes(size)
{
    _remainingLength = 0;
    reset();
}

MqttIn::MqttIn(MqttIn& src)
{
    memcpy(this,&src,sizeof(MqttIn));
    memcpy(_start,src._start,src._limit);
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
 Str* MqttIn::topic(){
    return &_topic;
}

 Strpack* MqttIn::message(){
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
        while (1);
}

bool MqttIn::complete()
{
    return (_recvState == ST_COMPLETE);
}

void MqttIn::complete(bool b)
{
    if (b)_recvState = ST_COMPLETE;
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
    return ( data & 0x80);
}

void MqttIn::parse()
{
    offset(0);
    _header = read();
    _remainingLength = 0;
    while (addRemainingLength(read()));
    switch (_header & 0xF0)
    {
    case MQTT_MSG_CONNECT:
    {
        // don't do , just for forwarding based on type
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
        _topic.sub(this, length);
        move(length);
        int rest = _remainingLength - length - 2;
        if (_header & MQTT_QOS_MASK)
        {
            readUint16(&_messageId);
            rest -= 2;
        }
        _message.sub(this, rest);
        //           ASSERT((_topic.length()+4+_message.length())==_remainingLength);
        break;
    }
    case MQTT_MSG_SUBACK:
    case MQTT_MSG_PUBACK:
    case MQTT_MSG_PUBREC:
    case MQTT_MSG_PUBREL:
    case MQTT_MSG_PUBCOMP:
    {
        readUint16(&_messageId);
        break;
    }
    case MQTT_MSG_PINGRESP:
    {
        break;
    }
    default:
    {
         ASSERT(false); // invalid message type, ignore noise
    }
    }
}

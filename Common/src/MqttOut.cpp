/*
 * MqttOut.cpp
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#include "MqttOut.h"
#include <iostream>
#define LOG(x) std::cout << Sys::upTime() << " | MQTT OUT " << x << std::endl

MqttOut::MqttOut(int size) :
Bytes(size) {
    _prefix = (char*) "";
}

void MqttOut::prefix(const char *prefix) {
    _prefix = (char*) prefix;
}

void MqttOut::add(uint8_t value) {
    write(value);
}

void MqttOut::addHeader(int hdr) {
    clear();
    write(hdr);
}

void MqttOut::addRemainingLength(uint32_t length) {
    do {
        uint8_t digit = length & 0x7F;
        length /= 128;
        if (length > 0) {
            digit |= 0x80;
        }
        write(digit);
    } while (length > 0);
}

void MqttOut::addUint16(uint16_t value) {
    write(value >> 8);
    write(value & 0xFF);
}

void MqttOut::addString(const char *str) {
    addUint16(strlen(str));
    addBytes((uint8_t*) str, strlen(str));
}

void MqttOut::addStr(Str* str) {
    addUint16(str->length());
    addBytes(str);
}

void MqttOut::addComposedString(char *prefix, char *str) {
    addUint16(strlen(prefix) + strlen(str));
    addBytes((uint8_t*) prefix, strlen(prefix));
    addBytes((uint8_t*) str, strlen(str));
}

void MqttOut::addMessage(uint8_t* src, uint32_t length) {
    addUint16(length);
    addBytes(src, length);
}

void MqttOut::addBytes(Bytes* bytes) {
    bytes->offset(0);
    for (int i = 0; i < bytes->length(); i++)
        write(bytes->read());
}

void MqttOut::addBytes(uint8_t* bytes, uint32_t length) {
    for (uint32_t i = 0; i < length; i++)
        write(*bytes++);
}

void MqttOut::Connect(uint8_t hdr, const char *clientId, uint8_t connectFlag,
        const char *willTopic, const char *willMsg, const char *username, const char* password,
        uint16_t keepAlive) {
            LOG("CONNECT");
    uint8_t connectFlags = connectFlag;

    uint16_t clientidlen = strlen(clientId);
    uint16_t usernamelen = strlen(username);
    uint16_t passwordlen = strlen(password);
    uint16_t payload_len = clientidlen + 2;
    uint16_t willTopicLen = strlen(willTopic) + strlen(_prefix);
    uint16_t willMsgLen = strlen(willMsg);

    // Preparing the connectFlags
    if (usernamelen) {
        payload_len += usernamelen + 2;
        connectFlags |= MQTT_USERNAME_FLAG;
    }
    if (passwordlen) {
        payload_len += passwordlen + 2;
        connectFlags |= MQTT_PASSWORD_FLAG;
    }
    if (willTopicLen) {
        payload_len += willTopicLen + 2;
        payload_len += willMsgLen + 2;
        connectFlags |= MQTT_WILL_FLAG;
    }

    // Variable header
    uint8_t var_header[] = {0x00, 0x06, 0x4d, 0x51, 0x49, 0x73, 0x64, 0x70, // Protocol name: MQIsdp
        0x03, // Protocol version
        connectFlags, // Connect connectFlags
        (uint8_t) (keepAlive >> 8), (uint8_t) (keepAlive & 0xFF), // Keep alive
    };

    // Fixed header
    uint8_t fixedHeaderSize = 2; // Default size = one byte Message Type + one byte Remaining Length
    uint8_t remainLen = sizeof (var_header) + payload_len;
    if (remainLen > 127) {
        fixedHeaderSize++; // add an additional byte for Remaining Length
    }

    // Message Type
    addHeader(MQTT_MSG_CONNECT | hdr);
    addRemainingLength(remainLen);
    addBytes(var_header, sizeof (var_header));
    // Client ID - UTF encoded
    addString(clientId);
    if (willTopicLen) {
        addComposedString((char*)_prefix,(char*)willTopic);
        addString(willMsg);
    }
    if (usernamelen) { // Username - UTF encoded
        addString(username);
    }
    if (passwordlen) { // Password - UTF encoded
        addString(password);
    }
}

void MqttOut::Publish(uint8_t hdr,  char* topic, Bytes* msg,
        uint16_t messageId) {
            LOG("PUBLISH");
    addHeader(MQTT_MSG_PUBLISH + hdr);
    bool addMessageId = (hdr & MQTT_QOS_MASK) ? true : false;
    int remLen = strlen(topic) + strlen(_prefix) + 2 + msg->length();
    if (addMessageId)
        remLen += 2;
    addRemainingLength(remLen);
    addComposedString(_prefix,topic);
    /*    addUint16(strlen(_prefix) + strlen(topic));
        addBytes((uint8_t*) _prefix, strlen(_prefix));
        addBytes((uint8_t*) topic, strlen(topic));*/
    if (addMessageId)
        addUint16(messageId);
    addBytes(msg);
}

void MqttOut::ConnAck(uint8_t erc) {
    LOG("CONNACK");
    addHeader(MQTT_MSG_CONNACK);
    addRemainingLength(2);
    add(0);
    add(erc);
}

void MqttOut::Disconnect() {
    LOG("DISCONNECT");
    addHeader(MQTT_MSG_DISCONNECT);
    addRemainingLength(0);
}

void MqttOut::PubRel(uint16_t messageId) {
    LOG("PUBREL");
    addHeader(MQTT_MSG_PUBREL | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void MqttOut::PubAck(uint16_t messageId) {
    LOG("PUBACK");
    addHeader(MQTT_MSG_PUBACK | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void MqttOut::PubRec(uint16_t messageId) {
    LOG("PUBREC");
    addHeader(MQTT_MSG_PUBREC | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void MqttOut::PubComp(uint16_t messageId) {
    LOG("PUBCOMP");
    addHeader(MQTT_MSG_PUBCOMP | MQTT_QOS1_FLAG);
    addRemainingLength(2);
    addUint16(messageId);
}

void MqttOut::Subscribe(uint8_t hdr, const char *topic, uint16_t messageId,
        uint8_t requestedQos) {
            LOG("SUBSCRIBE");
    addHeader(hdr | MQTT_MSG_SUBSCRIBE);
    addRemainingLength(strlen(topic) + strlen(_prefix) + 2 + 2 + 1);
    addUint16(messageId);
    addComposedString(_prefix,(char*)topic);
    add(requestedQos);
}

void MqttOut::PingReq() {
    LOG("PINGREQ");
    addHeader(MQTT_MSG_PINGREQ); // Message Type, DUP flag, QoS level, Retain
    addRemainingLength(0); // Remaining length
}

void MqttOut::PingResp() {
    LOG("PINGRESP");
    addHeader(MQTT_MSG_PINGRESP); // Message Type, DUP flag, QoS level, Retain
    addRemainingLength(0); // Remaining length
}



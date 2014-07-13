/*
 * MqttIn.h
 *
 *  Created on: 22-jun.-2013
 *      Author: lieven2
 */

#ifndef MQTTIN_H_
#define MQTTIN_H_
#include "MqttConstants.h"
#include "Str.h"
#include "Strpack.h"
#define TOPIC_LEN  100
#define MSG_LEN    256

class MqttIn : public Bytes
{
public:
    uint8_t _header;
    uint32_t _remainingLength;
    uint32_t _lengthToRead;
    uint32_t _offsetVarHeader;
    uint8_t _returnCode;
    uint16_t _messageId;
    Str _topic;
    Strpack _message;

    enum RecvState
    {
        ST_HEADER, ST_LENGTH, ST_PAYLOAD, ST_COMPLETE
    } _recvState;
public:
    MqttIn(int size);
    MqttIn(MqttIn& src);

    uint16_t messageId(); // if < 0 then not found
    uint8_t type();
    uint8_t qos();
    bool complete();
    void complete(bool st);
    void reset();
    void add(uint8_t data);
    bool addRemainingLength(uint8_t data);
    void parse();
    void readUint16(uint16_t * pi);
    void readUtf(Str* str);
    void readBytes(Bytes* b, int length);
     Str* topic();
     Strpack* message();
};

#endif /* MQTTIN_H_ */

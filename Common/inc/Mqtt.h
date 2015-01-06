/*
 * MqttSeq.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef MQTT_H_
#define MQTT_H_

#include "Event.h"
#include "Sequence.h"
#include "Link.h"
#include "MqttIn.h"
#include "CircBuf.h"
#include "pt.h"
#include "MqttConstants.h"
#include "Log.h"
#include "Prop.h"
#include "MqttOut.h"

#include "Handler.h"
#include "Event.h"
#include "Msg.h"

//************************************** CONSTANTS ****************************
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 2000
#define TIME_WAIT_CONNECT 5000
#define	TIME_PING ( TIME_KEEP_ALIVE /3 )
#define TOPIC_MAX_SIZE	40
#define MSG_MAX_SIZE	256



class Subscriber: public Handler
{
public:
    Subscriber(Mqtt& mqtt);
    int ptRun(Msg& msg);
    void sendPubRec();
    // will invoke
private:
    Mqtt& _mqtt;
    Str _topic;
    Bytes _message;
    Flags _flags;
    uint16_t _messageId;
    uint16_t _retries;
};

class Publisher: public Handler
{
public:
    Publisher(Mqtt& mqtt);
    int ptRun(Msg& msg);
    bool publish(Str& topic, Bytes& msg, Flags flags);
    // will send PUB_OK,PUB_FAIL
private:
    void sendPublish();
    void sendPubRel();
    Mqtt& _mqtt;
    enum State
    {
        ST_READY,
        ST_BUSY,
    } _state;
    Str _topic;
    Bytes _message;
    uint16_t _messageId;
    Flags _flags;
    uint16_t _retries;
};

class Subscription: public Handler
{
public:
    Subscription(Mqtt& mqtt);
    int ptRun(Msg& msg);
    void sendSubscribePut();
    void sendSubscribeGet();
private:
    Mqtt& _mqtt;
    enum State
    {
        ST_DISCONNECTED,
        ST_WAIT_SUBACK_PUT,
        ST_WAIT_SUBACK_GET,
        ST_SLEEP
    } _state;
    uint16_t _retries;
    uint16_t _messageId;
};

class Pinger: public Handler
{
public:
    Pinger(Mqtt& mqtt);
    int ptRun(Msg& msg);
private:
    Mqtt& _mqtt;
    uint32_t _retries;
};

class Mqtt: public Handler
{
public :
    Link& _link;

    Str _prefix;
    Str _putPrefix;    //PUT/<device>/
    Str _getPrefix;
    Str _headPrefix; // HEAD/<device>/
    MqttIn _mqttIn; // temp storage in one event call
    MqttOut _mqttOut; // "

private:

    Pinger* _pinger;
    Publisher* _publisher;
    Subscriber* _subscriber;
    Subscription* _subscription;
    uint32_t _retries;

public:
    Mqtt(Link& link);
    ~Mqtt();
    void sendConnect();
    int ptRun(Msg& msg);
    static uint16_t nextMessageId();
    void getPrefix(Str& prefix);
    void setPrefix(const char * prefix);
    bool publish(Str& topic, Bytes& msg, Flags flags);
    bool isConnected();
    bool msgToMqttIn(Msg& msg);
    bool isMqttMsg(Msg& msg, uint8_t msgType, uint16_t msgId);
//	Erc disconnect();
private:
    void sendSubscribe(uint8_t flags);
};

#endif /* MQTT_H_ */

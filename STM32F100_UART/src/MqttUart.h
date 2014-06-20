/*
 * MqttStream.h
 *
 *  Created on: 10-okt.-2013
 *      Author: lieven2
 */

#ifndef MQTTUART_H_
#define MQTTUART_H_
#include "Stream.h"
#include "Event.h"
#include "MqttIn.h"
#include "MqttOut.h"
#include "Uart.h"
#include "Property.h"

#define MQTT_UART	'm'
#define MAX_RETRIES 3
#define KEEP_ALIVE 20
#define SLEEP_TIME 1000
#define WAIT_CONNACK_TIME	2000
#define WAIT_SUBACK_TIME	2000

#define SUB_MSGID_OFFSET	0x100
#define PUB_DATA_MSGID_OFFSET	0x200
#define PUB_META_MSGID_OFFSET 0x300

class MqttListener {
	virtual void onMessage(MqttIn& in,MqttOut& out)=0;
	virtual ~MqttListener()=0;
};

class MqttUart: public Stream {
public:
	enum MqttUartEvents {
		EV_MESSAGE = EVENT(MQTT_UART, 'M'),
		EV_CONNACK = EVENT(MQTT_UART, 'C'),
		EV_SUBACK = EVENT(MQTT_UART, 'S'),
		EV_PUBACK = EVENT(MQTT_UART, 'P')
	};
	enum State {
		ST_DISCONNECT,
		ST_CONNECT,
		ST_SUBSCRIBE,
		ST_PUB_META,
		ST_PUB_DATA,
		ST_SLEEP
	};
	MqttUart(Uart* uart);
	virtual ~MqttUart();
	Erc event(Event& event);
	void uartRxd();
	void init();
	void setListener(MqttListener& listener);

	bool connect();
	bool subscribe();
	bool publishData();
	bool updateProperty(MqttIn& msg);
	bool publishMeta();
	bool retriesExceeded();
	bool send();
	void sendMsg();
	bool validatedReply(Event& event);
	bool next();
	char thread(Event& event);
	char thread1(Event& event);
	Erc getStats(Strpack& str);
	Erc toPack(Strpack& str);
	Erc fromPack(Strpack& str) {
		return E_INVAL;
	}
	uint32_t _messagesRxd;
	uint32_t _messagesTxd;

private:
	Timer _timer;
	Timer _timerUpdateAll;
	Uart* _uart;
	MqttIn _uartIn;
	MqttOut _uartOut;
	State _state;
	uint16_t _index;
	uint8_t _retries;
	uint16_t _messageId;
	MqttListener* _listener;
	uint64_t _lastUpdateAll;
	struct pt pt;
};

#endif /* MQTTUART_H_ */

/*
 * Usb.h
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#ifndef USB_H_
#define USB_H_

#include "Event.h"
#include "Sequence.h"
#include "Link.h"
#include "MqttIn.h"
#include "CircBuf.h"
#include "main.h"
#include "Fsm.h"

#define MAX_BUFFER 2

class Usb: public Fsm, public Link {
private:
	MqttIn mqttIn;
	bool _isComplete;

public:
	CircBuf in;
//	static const int CONNECTED, DISCONNECTED, RXD, MQTT_MESSAGE, FREE;

	Usb();
	static void init();
	void reset();
	Erc connect();
	Erc disconnect();
	Erc send(Bytes& bytes);
	MqttIn* recv();
	uint8_t read();
	uint32_t hasData();
	int handler(Event* event);
	void dispatch(Event& event) {
		handler(&event);
	}
};

#endif /* USB_H_ */

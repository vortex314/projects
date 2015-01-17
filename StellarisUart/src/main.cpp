/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Board.h"
#include "Mqtt.h"
#include "Timer.h"
#include "Prop.h"
#include "Cbor.h"
#include "Msg.h"

#include <malloc.h>
#include "Handler.h"

class TempTopic: public Prop
{
public:
	TempTopic() :
			Prop("system/temp", (Flags
					)
					{ T_FLOAT, M_READ, T_1SEC, QOS_0, NO_RETAIN })
	{
	}
	void toBytes(Bytes& bytes)
	{
		Cbor msg(bytes);
		msg.add(Board::getTemp());
	}
};

TempTopic tt;

class HardwareTopic: public Prop
{
public:
	HardwareTopic() :
			Prop("system/hardware", (Flags
					)
					{ T_OBJECT, M_READ, T_1KSEC, QOS_1, NO_RETAIN })
	{
	}

	void toBytes(Bytes& message)
	{
		Cbor msg(message);
		msg.addMap(4);
		msg.add("cpuRevision");
		Bytes b(8);
		Board::processorRevision(b);
		msg.add(b);
		msg.add("cpu");
		msg.add("LM4F120H5QR");
		msg.add("board");
		msg.add("EK-LM4F120XL");
		msg.add("bool");
		msg.add(false);
	}
};

HardwareTopic hardware;

class UptimeTopic: public Prop
{
public:
	UptimeTopic() :
			Prop("system/uptime", (Flags
					)
					{ T_UINT64, M_READ, T_1SEC, QOS_0, NO_RETAIN })
	{
	}

	void toBytes(Bytes& message)
	{
		Cbor msg(message);
		msg.add(Sys::upTime());
	}
};

UptimeTopic uptime;
uint64_t bootTime;

class RealTimeTopic: public Prop
{
public:
	RealTimeTopic() :
			Prop("system/now", (Flags
					)
					{ T_UINT64, M_RW, T_1SEC, QOS_0, NO_RETAIN })
	{
	}

	void toBytes(Bytes& message)
	{
		Cbor msg(message);
		msg.add(bootTime + Sys::upTime());
	}
	void fromBytes(Bytes& message)
	{
		Cbor msg(message);
		uint64_t now;
		if (msg.get(now))
		{
			bootTime = now - Sys::upTime();
		}
	}
};

RealTimeTopic now;

class SystemOnlineTopic: public Prop
{
public:
	SystemOnlineTopic() :
			Prop("system/online", (Flags
					)
					{ T_BOOL, M_READ, T_10SEC, QOS_1, NO_RETAIN })
	{
	}

	void toBytes(Bytes& message)
	{
		Cbor msg(message);
		msg.add(true);
	}
};
SystemOnlineTopic systemOnline;

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/lm4f120h5qr.h"
#include "inc/hw_gpio.h"

#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"

class GpioOutTopic: public Prop
{
	uint32_t _gpio_port;
	uint8_t _gpio_pin;
	bool _value;
public:
	GpioOutTopic(const char *name, uint32_t base, uint8_t pin) :
			Prop(name, (Flags
					)
					{ T_BOOL, M_RW, T_10SEC, QOS_2, NO_RETAIN })
	{
		_gpio_port = base;
		_gpio_pin = pin;
		_value = false;
//		GPIOPinTypeGPIOOutput(_gpio_port, _gpio_pin); // Enable the GPIO pins
//		GPIOPinWrite(_gpio_port, _gpio_pin, 0); // DON'T INITIALIZE PIN in STATIC , PORT IS NOT INIT YET
	}

	void fromBytes(Bytes& message)
	{
		Cbor msg(message);
		bool bl;
		if (msg.get(bl))
		{
			_value = bl;
			if (bl)
				GPIOPinWrite(_gpio_port, _gpio_pin, _gpio_pin);
			else
				GPIOPinWrite(_gpio_port, _gpio_pin, 0);
		}
	}
	void toBytes(Bytes& message)
	{
		Cbor msg(message);
		if (GPIOPinRead(_gpio_port, _gpio_pin))
			msg.add((bool) true);
		else
			msg.add((bool) false);
	}
};

GpioOutTopic gpioF2("GPIO/F2", GPIO_PORTF_BASE, GPIO_PIN_2);
GpioOutTopic gpioF1("GPIO/F1", GPIO_PORTF_BASE, GPIO_PIN_1);
GpioOutTopic gpioF3("GPIO/F3", GPIO_PORTF_BASE, GPIO_PIN_3);

// #include "Fsm.h"

class LedBlink: public Handler
{
	bool _isOn;
	uint32_t _msecInterval;
	Link* _link;
public:
	LedBlink(Link* link) :
			Handler("LedBlink")
	{
		_isOn = false;
		_msecInterval = 500;
		_link = link;

	}

	virtual ~LedBlink()
	{
	}

	int dispatch(Msg& msg)
	{
		PT_BEGIN(&pt)
			while (true)
			{
				timeout(_msecInterval);
				PT_YIELD_UNTIL(&pt,
						msg.is(_link, SIG_CONNECTED | SIG_DISCONNECTED)
								|| timeout());
				switch (msg.signal)
				{
				case SIG_TIMEOUT:
				{
					Board::setLedOn(Board::LED_GREEN, _isOn);
					_isOn = !_isOn;
					break;
				}
				case SIG_CONNECTED:
				{
					_msecInterval = 100;
					break;
				}
				case SIG_DISCONNECTED:
				{
					_msecInterval = 500;
					break;
				}
				default:
				{
				}
				}

			}
		PT_END(&pt);
}

};

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
#include "BipBuffer.h"
#include "Msg.h"
#include "Uart.h"
#include "Cbor.h"
#include "I2C.h"

#define L3GD20_ADDR  0b1101010

extern Uart *gUart0;
PropMgr propMgr;
// I2C i2c1(1);

class Main: public Handler
{

};

Main mainH;

int main(void)
{
	Board::init();	// initialize usb
	Uart::init();
	Bytes bytes(10);
	bytes.write(0x12);
//	i2c1.init();
//	i2c1.send(L3GD20_ADDR,bytes);
//	Usb usb;	// usb active object
//	Uart uart0;

	Mqtt mqtt(*gUart0);	// mqtt active object
//	MqttPublisher mqttPublisher(mqtt);
//	MqttSubscriber mqttSubscriber(mqtt);
	propMgr.mqtt(mqtt);
	LedBlink ledBlink(gUart0);

	mainH.reg(&mqtt);
	mainH.reg(gUart0);
	mainH.reg(&ledBlink);
	mainH.reg(&propMgr);

	uint64_t clock = Sys::upTime() + 100;
	MsgQueue::publish(0, SIG_INIT, 0, 0);				// kickoff all engines
	Msg msg;

	while (1)
	{
		if (Sys::upTime() > clock)
		{
			clock += 10;		// 10 msec timer tick
			MsgQueue::publish(0, SIG_TICK, 0, 0); // check timeouts every 10 msec
		}
		if (gUart0->hasData())	// if UART has received data alert uart receiver
			MsgQueue::publish(gUart0, SIG_START);
		// _________________________________________________________________handle all queued messages
		while (MsgQueue::get(msg))
		{
			mainH.dispatchToChilds(msg);
			if (msg.data != 0)
				delete (MqttIn*) (msg.data);
		}

	}
}

//*****************************************************************************
//
// usb_dev_serial.c - Main routines for the USB CDC serial example.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"
#include "usb_serial_structs.h"
//#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Serial Device (usb_dev_serial)</h1>
//!
//! This example application turns the evaluation kit into a virtual serial
//! port when connected to the USB host system.  The application supports the
//! USB Communication Device Class, Abstract Control Model to redirect UART0
//! traffic to and from the USB host system.
//!
//! Assuming you installed StellarisWare in the default directory, a
//! driver information (INF) file for use with Windows XP, Windows Vista and
//! Windows7 can be found in C:/StellarisWare/windows_drivers. For Windows
//! 2000, the required INF file is in C:/StellarisWare/windows_drivers/win2K.
//
//*****************************************************************************

//*****************************************************************************
//
// Note:
//
// This example is intended to run on Stellaris evaluation kit hardware
// where the UARTs are wired solely for TX and RX, and do not have GPIOs
// connected to act as handshake signals.  As a result, this example mimics
// the case where communication is always possible.  It reports DSR, DCD
// and CTS as high to ensure that the USB host recognizes that data can be
// sent and merely ignores the host's requested DTR and RTS states.
// comments in the code indicate where code would be required to add support
// for real handshakes.
//
//*****************************************************************************
#include "MqttIn.h"
#include "MqttOut.h"
#include "CircBuf.h"
#include "Queue.h"
#include "Event.h"
#include "Sequence.h"
#include "Timer.h"
#include "Log.h"
#include "pt.h"
#include "Board.h"

//*******************************************************************************

#define MAX_MSG_SIZE 100
#define TIME_KEEP_ALIVE 10000
#define TIME_WAIT_REPLY 1000

//*******************************************************************************

Log log;
uint16_t gMessageId = 1;
MqttIn mqttIn(MAX_MSG_SIZE);
MqttOut mqttOut(MAX_MSG_SIZE);
Str putPrefix(20);
Str getPrefix(20);
Str prefix(20);
#define MAX_BUFFER 2

class Usb: public Sequence {
private:
	bool _isConnected;
	int _index;
	MqttIn* mqttIn[MAX_BUFFER];

public:
	CircBuf in;
	static const int CONNECTED, DISCONNECTED, RXD, MQTT_MESSAGE;

	Usb() :
			in(256) {
		_index = 0;
		for (int i = 0; i < MAX_BUFFER; i++)
			mqttIn[i] = new MqttIn(MAX_MSG_SIZE);
		_index = 0;
	}
	void nextIndex() {
		_index++;
		if (_index == MAX_BUFFER)
			_index = 0;
	}
	bool isConnected() {
		return _isConnected;
	}
	void isConnected(bool bConnected) {
		_isConnected = bConnected;
	}
	void reset() {
		in.clear();
	}
	Erc send(Bytes& bytes) {
		bytes.AddCrc();
		bytes.Encode();
		bytes.Frame();
		bytes.offset(0);
		while (USBBufferSpaceAvailable((tUSBBuffer *) &g_sTxBuffer)
				&& bytes.hasData()) {
			uint8_t b;
			b = bytes.read();
			USBBufferWrite((tUSBBuffer *) &g_sTxBuffer, (unsigned char *) &b,
					1);
		}
		if (bytes.hasData() == false)
			return E_OK;
		return EAGAIN;
	}

	MqttIn* getMessage(int idx) {
		return mqttIn[idx];
	}

	int handler(Event* event) {
		if (event->is(Usb::CONNECTED))
			isConnected(true);
		else if (event->is(Usb::DISCONNECTED))
			isConnected(false);
		else if (event->is(Usb::RXD)) {
			uint8_t b;
			while (USBBufferRead((tUSBBuffer *) &g_sRxBuffer, &b, 1)) {
				mqttIn[_index]->add(b);
				if (mqttIn[_index]->complete()) {
					publish(Usb::MQTT_MESSAGE, _index);
					nextIndex();
					mqttIn[_index]->clear();
				}
			}

		}
		return 0;
	}
};
const int Usb::CONNECTED = Event::nextEventId("Usb::CONNECTED");
const int Usb::DISCONNECTED = Event::nextEventId("Usb::DISCONNECTED");
const int Usb::RXD = Event::nextEventId("Usb::RXD");
const int Usb::MQTT_MESSAGE = Event::nextEventId("Usb::MQTT_MESSAGE");

Usb usb;

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 1000
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//*****************************************************************************
volatile unsigned long g_ulUARTTxCount = 0;
volatile unsigned long g_ulUARTRxCount = 0;
#ifdef DEBUG
unsigned long g_ulUARTRxErrors = 0;
#endif

//*****************************************************************************
//
// The base address, peripheral ID and interrupt ID of the UART that is to
// be redirected.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines required to redirect UART0 via USB.
//
//*****************************************************************************
#define USB_UART_BASE           UART0_BASE
#define USB_UART_PERIPH         SYSCTL_PERIPH_UART0
#define USB_UART_INT            INT_UART0

//*****************************************************************************
//
// Default line coding settings for the redirected UART.
//
//*****************************************************************************
#define DEFAULT_BIT_RATE        115200
#define DEFAULT_UART_CONFIG     (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | \
                                 UART_CONFIG_STOP_ONE)

//*****************************************************************************
//
// GPIO peripherals and pins muxed with the redirected UART.  These will depend
// upon the IC in use and the UART selected in USB_UART_BASE.  Be careful that
// these settings all agree with the hardware you are using.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines required to redirect UART0 via USB.
//
//*****************************************************************************
#define TX_GPIO_BASE            GPIO_PORTA_BASE
#define TX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
#define TX_GPIO_PIN             GPIO_PIN_1

#define RX_GPIO_BASE            GPIO_PORTA_BASE
#define RX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
#define RX_GPIO_PIN             GPIO_PIN_0

//*****************************************************************************
//
// Global system tick counter
//
//*****************************************************************************
volatile uint64_t g_ulSysTickCount = 0;

//*****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//*****************************************************************************
#define COMMAND_PACKET_RECEIVED 0x00000001
#define COMMAND_STATUS_UPDATE   0x00000002

volatile unsigned long g_ulFlags = 0;
char *g_pcStatus;

//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
static volatile tBoolean g_bUSBConfigured = false;

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static void SetControlLineState(unsigned short usState);
static tBoolean SetLineCoding(tLineCoding *psLineCoding);
static void GetLineCoding(tLineCoding *psLineCoding);
static void SendBreak(tBoolean bSend);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
	while(1)
	{
	}
}
#endif

//*****************************************************************************
//
// Set the state of the RS232 RTS and DTR signals.
//
//*****************************************************************************
static void SetControlLineState(unsigned short usState) {
	//
	// If configured with GPIOs controlling the handshake lines,
	// set them appropriately depending upon the flags passed in the wValue
	// field of the request structure passed.
	//
}

//*****************************************************************************
//
// Set the communication parameters to use on the UART.
//
//*****************************************************************************
static tBoolean SetLineCoding(tLineCoding *psLineCoding) {
	return true;
}

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void GetLineCoding(tLineCoding *psLineCoding) {
	psLineCoding->ucDatabits = 8;
	psLineCoding->ucParity = USB_CDC_PARITY_NONE;
	psLineCoding->ucStop = USB_CDC_STOP_BITS_1;
	psLineCoding->ulRate = 115000;
}

//*****************************************************************************
//
// This function sets or clears a break condition on the redirected UART RX
// line.  A break is started when the function is called with \e bSend set to
// \b true and persists until the function is called again with \e bSend set
// to \b false.
//
//*****************************************************************************
static void SendBreak(tBoolean bSend) {
}

//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long ControlHandler(void *pvCBData, unsigned long ulEvent,
		unsigned long ulMsgValue, void *pvMsgData) {
	//
	// Which event are we being asked to process?
	//
	switch (ulEvent) {
	//
	// We are connected to a host and communication is now possible.
	//
	case USB_EVENT_CONNECTED:
		g_bUSBConfigured = true;

		//
		// Flush our buffers.
		//
		USBBufferFlush(&g_sTxBuffer);
		USBBufferFlush(&g_sRxBuffer);
		Sequence::publish(Usb::CONNECTED);
		usb.reset();
		break;

		//
		// The host has disconnected.
		//
	case USB_EVENT_DISCONNECTED:
		g_bUSBConfigured = false;
		Sequence::publish(Usb::DISCONNECTED);
		break;

		//
		// Return the current serial communication parameters.
		//
	case USBD_CDC_EVENT_GET_LINE_CODING:
		GetLineCoding((tLineCoding*) pvMsgData);
		break;

		//
		// Set the current serial communication parameters.
		//
	case USBD_CDC_EVENT_SET_LINE_CODING:
		SetLineCoding((tLineCoding*) pvMsgData);
		break;

		//
		// Set the current serial communication parameters.
		//
	case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
		SetControlLineState((unsigned short) ulMsgValue);
		break;

		//
		// Send a break condition on the serial line.
		//
	case USBD_CDC_EVENT_SEND_BREAK:
		SendBreak(true);
		break;

		//
		// Clear the break condition on the serial line.
		//
	case USBD_CDC_EVENT_CLEAR_BREAK:
		SendBreak(false);
		break;

		//
		// Ignore SUSPEND and RESUME for now.
		//
	case USB_EVENT_SUSPEND:
	case USB_EVENT_RESUME:
		break;

		//
		// We don't expect to receive any other events.  Ignore any that show
		// up in a release build or hang in a debug build.
		//
	default:
#ifdef DEBUG
		while(1);
#else
		break;
#endif

	}

	return (0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long TxHandler(void *pvCBData, unsigned long ulEvent,
		unsigned long ulMsgValue, void *pvMsgData) {
	//
	// Which event have we been sent?
	//
	switch (ulEvent) {
	case USB_EVENT_TX_COMPLETE:
		//
		// Since we are using the USBBuffer, we don't need to do anything
		// here.
		//
		/*	if (usbOut.hasData()) {
		 uint8_t b;
		 b = usbOut.read();
		 USBBufferWrite((tUSBBuffer *) &g_sTxBuffer, (unsigned char *) &b,
		 1);
		 }*/
		break;

		//
		// We don't expect to receive any other events.  Ignore any that show
		// up in a release build or hang in a debug build.
		//
	default:
#ifdef DEBUG
		while(1);
#else
		break;
#endif

	}
	return (0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long RxHandler(void *pvCBData, unsigned long ulEvent,
		unsigned long ulMsgValue, void *pvMsgData) {

	// Which event are we being sent?
	//
	switch (ulEvent) {
	//
	// A new packet has been received.
	//
	case USB_EVENT_RX_AVAILABLE: {
		/*		uint8_t b;
		 while (USBBufferRead((tUSBBuffer *) &g_sRxBuffer, &b, 1))
		 usb.in.write(b);*/
		Sequence::publish(Usb::RXD);
		break;
	}

		//
		// We are being asked how much unprocessed data we have still to
		// process. We return 0 if the UART is currently idle or 1 if it is
		// in the process of transmitting something. The actual number of
		// bytes in the UART FIFO is not important here, merely whether or
		// not everything previously sent to us has been transmitted.
		//
	case USB_EVENT_DATA_REMAINING: {
		//
		// Get the number of bytes in the buffer and add 1 if some data
		// still has to clear the transmitter.
		//
		return (0);
	}

		//
		// We are being asked to provide a buffer into which the next packet
		// can be read. We do not support this mode of receiving data so let
		// the driver know by returning 0. The CDC driver should not be sending
		// this message but this is included just for illustration and
		// completeness.
		//
	case USB_EVENT_REQUEST_BUFFER: {
		return (0);
	}

		//
		// We don't expect to receive any other events.  Ignore any that show
		// up in a release build or hang in a debug build.
		//
	default:
#ifdef DEBUG
		while(1);
#else
		break;
#endif
	}

	return (0);
}

uint16_t nextMessageId() {
	return gMessageId++;
}

/*****************************************************************************
 *   MQTT EVENT filter
 ******************************************************************************/

bool eventIsMqtt(Event* event, uint8_t type, uint16_t messageId, uint8_t qos) {
	if (event->id() != Usb::MQTT_MESSAGE)
		return false;
	MqttIn* mqttIn;
	mqttIn = usb.getMessage(event->detail());
	if (type)
		if (mqttIn->type() != type)
			return false;
	if (messageId)
		if (mqttIn->messageId() != messageId)
			return false;
	if (qos)
		if (mqttIn->qos() != qos)
			return false;
	return true;
}

typedef enum {
		LED_BLUE = 1, LED_GREEN, LED_RED
	} Led;

#define LED_INTERVAL 100
class LedBlink: public Sequence {
private:
	struct pt t;

public:
	LedBlink() {
		PT_INIT( &t);
	}
	int handler(Event* event) {
		PT_BEGIN ( &t )
			while (true) {
				timeout(99);
				PT_YIELD_UNTIL( &t, timeout());
				Board::setLedOn(LED_GREEN,true);
				timeout(1);
				PT_YIELD_UNTIL( &t, timeout());
				Board::setLedOn(LED_GREEN,false);
			}
		PT_END ( &t );
}
};

class Mqtt: public Sequence {
private:
	struct pt t;
	uint16_t _messageId;
	bool _isConnected;
	Str str;
	Str msg;

public:
	const static int DISCONNECTED, CONNECTED, IN;

	Mqtt() :
			str(30), msg(10) {
		PT_INIT( &t);
		_messageId = 2000;
		_isConnected = false;
	}

	int handler(Event* event) {
		PT_BEGIN ( &t )
			;
			while (1) {
				_messageId = nextMessageId();

				PT_YIELD_UNTIL( &t, usb.isConnected());
				mqttOut.Connect(MQTT_QOS2_FLAG, "clientId", MQTT_CLEAN_SESSION,
						"system/online", "false", "", "",
						TIME_KEEP_ALIVE / 1000);
				usb.send(mqttOut);

				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL( &t,
						eventIsMqtt ( event, MQTT_MSG_CONNACK, 0, 0 ) || timeout());
				if (timeout()) {
					publish(DISCONNECTED);
					continue;
				}

				str.clear() << putPrefix << "#";
				mqttOut.Subscribe(0, str, ++_messageId, MQTT_QOS1_FLAG);
				usb.send(mqttOut);

				str.clear() << getPrefix << "#";
				mqttOut.Subscribe(0, str, ++_messageId, MQTT_QOS1_FLAG);
				usb.send(mqttOut);

				str = "system/online";
				msg = "true";
				mqttOut.Publish(MQTT_QOS1_FLAG, str, msg, ++_messageId);
				usb.send(mqttOut);

				timeout(TIME_WAIT_REPLY);
				PT_YIELD_UNTIL( &t,
						eventIsMqtt ( event, MQTT_MSG_PUBACK, _messageId, 0 ) || timeout());
				if (timeout()) {
					publish(DISCONNECTED);
					continue;
				}
				publish(CONNECTED);
				_isConnected = true;
				PT_YIELD_UNTIL( &t, event->is (Usb::DISCONNECTED ));

				publish(DISCONNECTED);
				_isConnected = false;
			}
		PT_END ( &t );
}

Erc send(Bytes& pb) {
	if (usb.isConnected())
		return usb.send(pb);
	else
		return E_AGAIN;
}
bool isConnected() {
	return _isConnected;
}
;
Erc disconnect() {
	return E_OK;
}
};
const int Mqtt::CONNECTED = Event::nextEventId("MQTT_CONNECTED");
const int Mqtt::DISCONNECTED = Event::nextEventId("MQTT_DISCONNECTED");

void eventPump() {
	Event event;
	Erc erc;
	erc = Sequence::get(event); // dispatch eventually IDLE message
	if (erc == E_LACK_RESOURCE)
		return;
	if (event.id() != Timer::TICK) {
		log << " EVENT : ";
		event.toString(log);
		log.flush();
	}

	int i;
	for (i = 0; i < MAX_SEQ; i++)
		if (Sequence::activeSequence[i]) {
			if (Sequence::activeSequence[i]->handler(&event) == PT_ENDED) {
				Sequence* seq = Sequence::activeSequence[i];
				seq->unreg();
				delete seq;
			};
		}

}
;

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int main(void) {
	prefix << "Stellaris" << "/";
	mqttOut.prefix(prefix);
	getPrefix << "GET/" << prefix;
	putPrefix << "PUT/" << prefix;
	//
	// Enable lazy stacking for interrupt handlers.  This allows floating-point
	// instructions to be used within interrupt handlers, but at the expense of
	// extra stack usage.
	//
	ROM_FPULazyStackingEnable();
	//
	// Set the clocking to run from the PLL at 50MHz
	//
	ROM_SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_16MHZ);
	//
	// Configure the required pins for USB operation.
	//
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);
	//
	// Not configured initially.
	//
	g_bUSBConfigured = false;
	//
	// Enable the system tick.
	//
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();
	//
	// Initialize the transmit and receive buffers.
	//
	USBBufferInit((tUSBBuffer *) &g_sTxBuffer);
	USBBufferInit((tUSBBuffer *) &g_sRxBuffer);
	//
	// Set the USB stack mode to Device mode with VBUS monitoring.
	//
	USBStackModeSet(0, USB_MODE_DEVICE, 0);
	//
	// Pass our device information to the USB library and place the device
	// on the bus.
	//
	USBDCDCInit(0, (tUSBDCDCDevice *) &g_sCDCDevice);
	//
	// Main application loop.
	//
	Mqtt mqtt;
	LedBlink ledBlink;
	Board::init();
	uint64_t clock = Sys::upTime() + 100;
	while (1) {
		eventPump();
		if (Sys::upTime() > clock) {
			clock += 100;
			Sequence::publish(Timer::TICK);
		}
		/*		while (usbIn.hasData()) {
		 uint8_t b=usbIn.read();
		 usbOut.write( b );
		 }

		 while (USBBufferSpaceAvailable((tUSBBuffer *) &g_sTxBuffer)
		 && usbOut.hasData()) {
		 uint8_t b;
		 b = usb.out.read();
		 USBBufferWrite((tUSBBuffer *) &g_sTxBuffer, (unsigned char *) &b,1);
		 }*/
	}
}

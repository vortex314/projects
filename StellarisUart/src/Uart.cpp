/*
 * Uart.cpp
 *
 *  Created on: 24-okt.-2014
 *      Author: lieven2
 */

#include "Uart.h"
#include "Msg.h"

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
#include "driverlib/uart.h"

Uart* gUart0 = new Uart();

Uart::Uart() :
		_in(100), _out(256), _mqttIn(256) {
	gUart0 = this;
	_overrunErrors=0;
	_crcErrors=0;
}
// initialize UART at 1MB 8N1
void Uart::init() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE));
	// Enable the UART interrupt.
	//
	UARTFIFOEnable(UART0_BASE);
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX1_8);
	UARTTxIntModeSet(UART0_BASE, UART_TXINT_MODE_FIFO);
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_TX);
}

Uart::~Uart() {
}

uint8_t Uart::read() {
	return gUart0->_in.read();
}
uint32_t Uart::hasData() {
	return gUart0->_in.hasData();
}

bool UARTFIFOFull(uint32_t uart_base) {
	uint32_t fr = *((uint32_t*) (UART0_BASE + UART_O_FR));
	return (fr & UART_FR_TXFF) != 0;
}

void Uart::toFifo() {
	while (!UARTFIFOFull(UART0_BASE) && _out.hasData()) {
		if (UARTCharPutNonBlocking(UART0_BASE, _out.read()) == false)
			break;
	}
}

Erc Uart::send(Bytes& bytes) {
	bytes.AddCrc();
	bytes.Encode();
	bytes.Frame();
	bytes.offset(0);
	if (_out.space() < bytes.length()) { // not enough space in circbuf
		_overrunErrors++;
		toFifo();
		return E_AGAIN;
	}

	while (_out.hasSpace() && bytes.hasData()) {
		_out.write(bytes.read());
	}
	toFifo();
	return E_OK;
}

Erc Uart::connect() {
	Msg::publish(SIG_LINK_CONNECTED);
	isConnected(true);
	return E_OK;
}

Erc Uart::disconnect() {
	Msg::publish(SIG_LINK_DISCONNECTED);
//	isConnected(false);
	return E_OK;
}

void Uart::dispatch(Msg& event) {
	switch (event.sig()) {

	case SIG_LINK_CONNECTED: {
		_in.clear();
		isConnected(true);
//      publish(Link::CONNECTED);
		break;
	}
	case SIG_LINK_DISCONNECTED: {
//		isConnected(false);
//      publish(Link::DISCONNECTED);
		break;
	}
	case (SIG_LINK_RXD):

	{
		uint8_t b;
		while (_in.hasData()) {
			b = _in.read();
			if (_mqttIn.Feed(b)) {
				_mqttIn.Decode();
				if (_mqttIn.isGoodCrc()) {
					_mqttIn.RemoveCrc();
					_mqttIn.parse();
					Msg m;
					m.create(100).sig(SIG_MQTT_MESSAGE).add(_mqttIn).send();
				} else {
					_crcErrors++;
				}
				_mqttIn.clear();
			}
		}
		break;
	}
	default: {
		if (_in.hasData())
			Msg::publish(SIG_LINK_RXD);
	}
	}
}
uint32_t circbufOverflow = 0;
extern "C" void UART0IntHandler(void) {
	unsigned long ulStatus;

	ulStatus = UARTIntStatus(UART0_BASE, true); // Get the interrrupt status.
	UARTIntClear(UART0_BASE, ulStatus); // Clear the asserted interrupts.
	if (ulStatus & UART_INT_RX) {
		while (UARTCharsAvail(UART0_BASE)) { // Loop while there are characters in the receive FIFO.
			if (gUart0) {
				if (gUart0->_in.hasSpace())
					gUart0->_in.write(UARTCharGetNonBlocking(UART0_BASE)); // Read the next character from the UART
				else
					gUart0->_overrunErrors++;
			}
		}
//LMR		Msg::publish(SIG_LINK_RXD);
	}
	if (ulStatus & UART_INT_TX) {
		gUart0->toFifo();
	}
}


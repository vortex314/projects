/*
 * Uart.cpp
 *
 *  Created on: 24-okt.-2014
 *      Author: lieven2
 */

#include <Uart.h>
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

Uart* gUart0;

Uart::Uart() :
		_in(100), _out(100) {
	gUart0 = this;
}
// initialize UART at 1MB 8N1
void Uart::init() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 1000000,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE));
	// Enable the UART interrupt.
	//
	ROM_IntEnable(INT_UART0);
	ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}

Uart::~Uart() {

}

uint8_t Uart::read() {
	return gUart0->_in.read();
}
uint32_t Uart::hasData() {
	return gUart0->_in.hasData();
}

Erc Uart::send(Bytes& bytes) {
	while (_out.hasSpace() && bytes.hasData()) {
		_out.write(bytes.read());
	}
	while (UARTSpaceAvail(UART0_BASE) && _out.hasData()) {
		UARTCharPut(UART0_BASE, _out.read());
	}
	// init TXD
	if (bytes.hasData())
		return E_AGAIN;
	return E_OK;
}

Erc Uart::connect() {
	return E_OK;
}

Erc Uart::disconnect() {
	return E_OK;
}

extern "C" void UART0IntHandler(void) {
	unsigned long ulStatus;

	ulStatus = ROM_UARTIntStatus(UART0_BASE, true); // Get the interrrupt status.
	ROM_UARTIntClear(UART0_BASE, ulStatus); // Clear the asserted interrupts.
	while (ROM_UARTCharsAvail(UART0_BASE)) { // Loop while there are characters in the receive FIFO.
		gUart0->_in.write(ROM_UARTCharGetNonBlocking(UART0_BASE)); // Read the next character from the UART
	}
}

//*****************************************************************************
//
// This example demonstrates how to send a string of data to the UART.
//
//*****************************************************************************
/*int main(void) {
 //
 // Enable lazy stacking for interrupt handlers.  This allows floating-point
 // instructions to be used within interrupt handlers, but at the expense of
 // extra stack usage.
 //
 ROM_FPUEnable();
 ROM_FPULazyStackingEnable();

 //
 // Set the clocking to run directly from the crystal.
 //
 ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
 SYSCTL_XTAL_16MHZ);

 //
 // Enable the GPIO port that is used for the on-board LED.
 //
 ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

 //
 // Enable the GPIO pins for the LED (PF2).
 //
 ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

 //
 // Enable the peripherals used by this example.
 //
 ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
 ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

 //
 // Enable processor interrupts.
 //
 ROM_IntMasterEnable();

 //
 // Set GPIO A0 and A1 as UART pins.
 //
 GPIOPinConfigure(GPIO_PA0_U0RX);
 GPIOPinConfigure(GPIO_PA1_U0TX);
 ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

 //
 // Configure the UART for 115,200, 8-N-1 operation.
 //
 ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
 UART_CONFIG_PAR_NONE));

 //
 // Enable the UART interrupt.
 //
 ROM_IntEnable(INT_UART0);
 ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

 //
 // Prompt for text to be entered.
 //
 UARTSend((unsigned char *) "\033[2JEnter text: ", 16);

 //
 // Loop forever echoing data through the UART.
 //
 while (1) {
 }
 }*/

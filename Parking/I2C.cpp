/*
 * I2C.cpp
 *
 *  Created on: 22-dec.-2014
 *      Author: lieven2
 */

#include <I2C.h>

#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/i2c.h"
#include "utils/softi2c.h"
#include "utils/uartstdio.h"

I2C::I2C(uint32_t port) {
	_port = port;
}

I2C::~I2C() {
}

void I2C::init() {
	if (_port == 0) {  // The I2C0 peripheral must be enabled before use.
		SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
		GPIOPinConfigure(GPIO_PB2_I2C0SCL);
		GPIOPinConfigure(GPIO_PB3_I2C0SDA);
		GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
		I2CMasterInitExpClk(I2C0_MASTER_BASE, SysCtlClockGet(), false); // true=400kbps, false=100kbps
	} else if (_port == 1) {
		SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
		GPIOPinConfigure(GPIO_PA6_I2C1SCL);
		GPIOPinConfigure(GPIO_PA7_I2C1SDA);
		GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
		I2CMasterInitExpClk(I2C1_MASTER_BASE, SysCtlClockGet(), false);
	} else if (_port == 2) {
		SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
		GPIOPinConfigure(GPIO_PE4_I2C2SCL);
		GPIOPinConfigure(GPIO_PE5_I2C2SDA);
		GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);
		I2CMasterInitExpClk(I2C1_MASTER_BASE, SysCtlClockGet(), false);
	}
}
#define I2C_READ true
#define I2C_WRITE false
void I2C::send(uint8_t addr, Bytes& b) {
	//
	// Tell the master module what address it will place on the bus when
	// communicating with the slave.  Set the address to SLAVE_ADDRESS
	// (as set in the slave module).  The receive parameter is set to false
	// which indicates the I2C Master is initiating a writes to the slave.  If
	// true, that would indicate that the I2C Master is initiating reads from
	// the slave.
	//
	I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, addr, I2C_WRITE);
	//
	// Place the data to be sent in the data register
	//
	I2CMasterDataPut(I2C0_MASTER_BASE, b.read());
	//
	// Initiate send of data from the master.  Since the loopback
	// mode is enabled, the master and slave units are connected
	// allowing us to receive the same data that we sent out.
	//
	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
	//
	// Wait until the slave has received and acknowledged the data.
	//
	while (!(I2CSlaveStatus(I2C0_SLAVE_BASE) & I2C_SCSR_RREQ)) {
	}

	//
	// Read the data from the slave.
	//
	uint8_t b2 = I2CSlaveDataGet(I2C0_SLAVE_BASE);

	//
	// Wait until master module is done transferring.
	//
	while (I2CMasterBusy(I2C0_MASTER_BASE)) {
	}
}

extern "C" void I2C0IntHandler(void) {
	unsigned long ulStatus;
	ulStatus = I2CMasterIntStatus(I2C0_MASTER_BASE, true); // Get the interrrupt status.
	I2CMasterIntClear(I2C0_MASTER_BASE);
}
extern "C" void I2C1IntHandler(void) {
	unsigned long ulStatus;
	ulStatus = I2CMasterIntStatus(I2C1_MASTER_BASE, true); // Get the interrrupt status.
	I2CMasterIntClear(I2C1_MASTER_BASE);
}
extern "C" void I2C2IntHandler(void) {
	unsigned long ulStatus;
	ulStatus = I2CMasterIntStatus(I2C2_MASTER_BASE, true); // Get the interrrupt status.
	I2CMasterIntClear(I2C2_MASTER_BASE);
}
extern "C" void I2C3IntHandler(void) {
	unsigned long ulStatus;
	ulStatus = I2CMasterIntStatus(I2C3_MASTER_BASE, true); // Get the interrrupt status.
	I2CMasterIntClear(I2C3_MASTER_BASE);
}

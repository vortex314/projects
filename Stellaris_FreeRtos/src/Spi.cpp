/*
 * Spi.cpp
 *
 *  Created on: 26-mei-2014
 *      Author: lieven2
 */

#include "Spi.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/interrupt.h"

Spi* spi[4] = { NULL, NULL, NULL, NULL };

extern "C" void Spi0Handler() {
	if (spi[0])
		spi[0]->intHandler();
	else
		SSIIntDisable(SSI0_BASE, SSI_TXFF | SSI_RXFF | SSI_RXTO);
}
extern "C" void Spi1Handler() {
	if (spi[1])
		spi[1]->intHandler();
	else
		SSIIntDisable(SSI1_BASE, SSI_TXFF | SSI_RXFF | SSI_RXTO);
}
extern "C" void Spi2Handler() {
	if (spi[2])
		spi[2]->intHandler();
	else
		SSIIntDisable(SSI2_BASE, SSI_TXFF | SSI_RXFF | SSI_RXTO);
}
extern "C" void Spi3Handler() {
	if (spi[3])
		spi[3]->intHandler();
	else
		SSIIntDisable(SSI3_BASE, SSI_TXFF | SSI_RXFF | SSI_RXTO);
}

const uint32_t SSI_BASE_ADDRESS[] =
		{ SSI0_BASE, SSI1_BASE, SSI2_BASE, SSI3_BASE };
const unsigned long SSI_INT[] = { INT_SSI0, INT_SSI1, INT_SSI2, INT_SSI3 };
const unsigned long SSI_PERIPH[] = { SYSCTL_PERIPH_SSI0, SYSCTL_PERIPH_SSI1,
		SYSCTL_PERIPH_SSI2, SYSCTL_PERIPH_SSI3 };

#define SSI (uint32_t)(_port)

void Spi::intHandler(void) {
	unsigned long ulStatus, ulData;
	ulStatus = SSIIntStatus(SSI, true);

	SSIIntClear(SSI, ulStatus);
	if (ulStatus & (SSI_RXFF)) // See if the receive interrupt is being asserted.
			{
		while (SSIDataGetNonBlocking(SSI, &ulData)) {
			_in.write(ulData & 0x00FF);
			_count--;
			;
		}
		if (_count == 0)
			post(new Event( upStream(),this, Spi::RXD));

		//		SSIIntDisable(SSI0_BASE, SSI_RXFF);
	}
	if (ulStatus & SSI_TXFF) {
		while (_out.hasData()) {
			if (SSIDataPutNonBlocking(SSI, _out.peek()))
				_out.read();
		}
		if (_out.hasData() == 0)
			SSIIntDisable(SSI, SSI_TXFF);
	}
}

Spi::Spi(uint32_t id) :
		_out(10), _in(10) {
	if (id > 3)
		return;
	_clock = 1000000; //default value 1MHz
	_port = (void*) SSI_BASE_ADDRESS[id];
	spi[id] = this;

	SysCtlPeripheralReset(SSI_PERIPH[id]);
	SSIConfigSetExpClk(SSI, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, _clock, 8);
	SSIEnable(SSI);
	SSIIntEnable(SSI, SSI_TXFF | SSI_RXFF);
	IntEnable(SSI_INT[id]);

//	HWREG(SSI + SSI_O_CR1) |= SSI_CR1_LBM; //enable SSI (internal) loopback mode, testing purpose
}

bool Spi::hasData(){
	return _out.hasData();
}

uint8_t Spi::read(){
	return _out.read();
}

Spi::~Spi() {

}

#include "FreeRTOS.h"
#include "task.h"

Erc Spi::write(uint8_t b) {
	return _out.write(b);
}

Erc Spi::flush(){
	SSIIntEnable(SSI, SSI_TXFF); // interrupt will be generated if fifo is empty
}

Erc Spi::send(Bytes& b) {
	b.offset(0);
	while (b.hasData() & _out.hasSpace())
		_out.write(b.read());
	_count = _out.length();
	_out.offset(0);
	SSIIntEnable(SSI, SSI_TXFF | SSI_RXFF); // interrupt will be generated if fifo is empty
	while (_out.hasData()) {
		if (SSIDataPutNonBlocking(SSI, _out.peek()))
			_out.read();
	};
	/*	int i;
	unsigned long ulData;
	b.clear();
	for (i = 0; i < _count; i++) {
		while (SSIDataGetNonBlocking(SSI, &ulData) == 0)
			taskYIELD();
		b.write(ulData);
	}*/
	/*	while (SSIDataGetNonBlocking(SSI, &ulData))
	 ;
	 b.offset(0);
	 while (b.hasData()) {
	 SSIDataPutNonBlocking(SSI, b.read());
	 }*/
	return E_OK;
}

Erc Spi::setClock(uint32_t hz) {
	_clock = hz;
	SSIConfigSetExpClk(SSI, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, _clock, 8);
	return E_OK;
}

uint32_t Spi::getClock() {

	return _clock;
}

Erc Spi::event(Event* event) {
	return E_OK;
}

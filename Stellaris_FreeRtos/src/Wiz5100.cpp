/*
 * File:   Wiz5100.cpp
 * Author: lieven
 *
 * Created on August 25, 2013, 11:18 AM
 */

#include "Wiz5100.h"
#include "Bytes.h"
#include "Property.h"
#include "Timer.h"

#ifdef PIC32

#define PPSU() {SYSKEY=0x0;SYSKEY=0xAA996655;SYSKEY=0x556699AA;CFGCONbits.IOLOCK=0;}
#define PPSL() {SYSKEY=0x0;SYSKEY=0xAA996655;SYSKEY=0x556699AA;CFGCONbits.IOLOCK=1;}
#endif

#define STELLARIS

#ifdef STELLARIS
#include <driverlib/rom_map.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#endif
void Wiz5100::reset() {
#ifdef PIC32
	mPORTBSetPinsDigitalOut(BIT_15);
	mPORTBClearBits(BIT_15);
	Sys::delay_ms(100);
	mPORTBSetBits(BIT_15);
#endif
#ifdef STELLARIS

	MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PB0_U1RX, 0);
	Sys::delay_ms(100);
	MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PB0_U1RX, GPIO_PIN_0);
	Sys::delay_ms(100);
#endif
}

Erc Wiz5100::loadCommon(uint8_t mac[6], uint8_t ipAddress[4],
		uint8_t gtwAddr[4], uint8_t netmask[4]) {
	int i;
	uint8_t reg;
	int erc = 0;
	if (erc)
		return erc;
	for (i = 0; i < 4; i++) {
		erc += write(W5100_GAR + i, gtwAddr[i]); // set up the gateway address
		read(W5100_GAR + i,&reg);
	}
	Sys::delay_ms(1);
	for (i = 0; i < 6; i++)
		erc += write(W5100_SHAR + i, mac[i]); // set up the MAC address
	Sys::delay_ms(1);
	for (i = 0; i < 4; i++)
		erc += write(W5100_SUBR + i, netmask[i]); // set up the subnet mask
	Sys::delay_ms(1);
	for (i = 0; i < 4; i++)
		erc += write(W5100_SIPR + i, ipAddress[i]); // set up the source IP address
	Sys::delay_ms(1);
	erc += write(W5100_RMSR, (uint8_t) 0x55); // use default buffer sizes (2K bytes RX and TX for each socket
	erc += write(W5100_TMSR, (uint8_t) 0x55);
	return erc;
}

Wiz5100::Wiz5100(Spi* spi, uint32_t socket) {
//	_socket = socket;
//	_socketAddr = 0x0400 + (_socket * 0x0100);
	_spi = spi;
	_spi->upStream(this);
}

Wiz5100::~Wiz5100() {
}
/*
 int Wiz5100::socket() {
 return _socket;
 }
 */
int Wiz5100::init() {
	return E_OK;
}

Erc Wiz5100::event(Event* event) {
	if (event->id() == Timer::EXPIRED)
		poll(0);
	return E_OK;
}

void Wiz5100::poll(uint32_t socket) {
	/*	// poll socket
	 uint8_t reg;
	 read(Sx_IR(socket), &reg);
	 if (IR != reg) {
	 upStream()->post(new Event(upStream(), this, Wiz5100::INTERRUPT + reg));
	 IR = reg;
	 };
	 write(Sx_IR(socket), reg); //clear IR flags
	 read(Sx_SR(socket), &reg);
	 if (SR != reg) {
	 upStream()->post(
	 new Event(upStream(), this, Wiz5100::STATUS_CHANGE + reg));
	 SR = reg;
	 };*/
}

#include "task.h"

Erc Wiz5100::write(uint16_t address, uint8_t data) {
	union {
		uint8_t rxd[4];
		uint32_t iRxd;
	};
	uint8_t txd[] = { W5100_WRITE_OPCODE, address >> 8, address, data };
	Erc erc = _spi->exchange(txd, rxd, 4);
	if (erc)
		return erc;
	if (iRxd != 0x03020100)
		return E_AGAIN;
	return E_OK;
}

Erc Wiz5100::read(uint16_t address, uint8_t* data) {
	union {
		uint8_t rxd[4];
		uint32_t iRxd;
	};
	uint8_t txd[] = { W5100_READ_OPCODE, address >> 8, address, 0 };
	Erc erc = _spi->exchange(txd, rxd, 4);
	if (erc)
		return erc;
	if ((iRxd & 0x00FFFFFF) != 0x00020100)
		return E_AGAIN;
	*data = rxd[3];
	return E_OK;
}

Erc Wiz5100::read16(uint16_t address, uint16_t* data) {
	int erc;
	uint8_t res1, res2;
	erc = read(address, &res1);
	if (erc)
		return erc;
	erc = read(address + 1, &res2);
	if (erc)
		return erc;
	*data = (res1 << 8) + res2;
	return E_OK;
}

Erc Wiz5100::write16(uint16_t address, uint16_t data) {
	int erc;
	erc = write(address, (uint8_t) (data >> 8));
	if (erc)
		return erc;
	erc = write(address + 1, (uint8_t) (data & 0xFF));
	if (erc)
		return erc;
	return E_OK;
}

int Wiz5100::socketCmd(uint32_t socket, uint8_t cmd) {
	int erc = write(Sx_CR(socket), cmd); // put socket in listen state
	if (erc)
		return erc;
	while (true) {
		uint8_t cmdStatus;
		erc = read(Sx_CR(socket), &cmdStatus);
		if (erc)
			return erc;
		if (cmdStatus == 0)
			break;
	}; // block until command is accepted
	return 0;
}

/*
 int Wiz5100::socketOpen() {
 int erc;
 uint8_t status;
 erc += socketCmd(W5100_SKT_CR_OPEN);
 erc += getStatus(&status);
 if (status != W5100_SKT_SR_INIT) {
 erc = socketCmd(W5100_SKT_CR_CLOSE); // tell chip to close the socket
 return E_CONN_LOSS;
 }
 return 0;
 }

 int Wiz5100::socketClose() {
 int erc;
 uint8_t status;
 erc = getStatus(&status);
 if (erc) return erc;
 if (status != W5100_SKT_SR_CLOSED) // Make sure we close the socket first
 {
 socketCmd(W5100_SKT_CR_CLOSE); // tell chip to close the socket
 while (true) { // loop until socket is closed (blocks!!)
 erc = getStatus(&status);
 if (erc) return erc;
 if (status == W5100_SKT_SR_CLOSED) break;
 }
 }
 return 0;
 }
 */
void Wiz5100::setCR(uint32_t socket, uint8_t cmd) {
	write(Sx_CR(socket), cmd);
}

uint8_t Wiz5100::getCR(uint32_t socket) {
	uint8_t cmd;
	read(Sx_CR(socket), &cmd);
	return cmd;
}

/*
 void Wiz5100::setIR(uint8_t cmd) {
 write(Sx_IR(_socket), cmd);
 }*/

/*
 int Wiz5100::socketConnect() {
 int erc = 0;
 uint8_t status;
 erc += socketCmd(W5100_SKT_CR_CONNECT); // connect to server
 _delay_ms(1000);
 erc += getStatus(&status);
 uint8_t ir;
 erc += read(Sx_IR(_socket), &ir);
 if (erc) return ENOTCONN;
 if (status != W5100_SKT_SR_ESTABLISHED)
 return ENOTCONN;
 return 0;
 }

 int Wiz5100::socketDisconnect() {
 int erc = 0;
 uint8_t status;
 erc += getStatus(&status);
 if (erc) return erc;
 if (status == W5100_SKT_SR_CLOSED) return 0; // alreay disconnected
 erc += socketCmd(W5100_SKT_CR_DISCON); // disconnect from server
 while (true) {
 erc += getStatus(&status);
 uint8_t ir;
 erc += read(Sx_IR(_socket), &ir);
 if (erc) return EAGAIN;
 if (status == W5100_SKT_SR_CLOSED)
 break;
 }
 return 0;
 }*/

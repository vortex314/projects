/*
 * Wiz5100.cpp
 *
 *  Created on: 20-dec.-2013
 *      Author: lieven2
 */

#include "Wiz5100.h"
#include <driverlib/rom_map.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"

Wiz5100::Wiz5100(uint32_t socket) :
		_spi(0), _timer(this) {
	socket = socket;
	_socketAddr = 0x0400 + (_socket * 0x0100);
}

Wiz5100::~Wiz5100() {
ASSERT(false);
}

/*
 * 		/RESET		PB0				OUT
 * 		/INT		PB6				IN
 * 		/CS			PA3	SSI0FSS		OUT
 * 		MISO		PA4 SSI0RX		IN
 * 		MOSI		PA6 SSI0TX		OUT
 * 		CLK			PA2	SSI0CLK		OUT
 *
 */
// toggle PB0 to low during 10 msec
void Wiz5100::reset() {
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
	Sys::delay_ms(1);
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
	Sys::delay_ms(10);
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
	Sys::delay_ms(1);
}

/*
 * File:   Wiz5100.cpp
 * Author: lieven
 *
 * Created on August 25, 2013, 11:18 AM
 */

#include "Wiz5100.h"
#include "Bytes.h"
#include "Timer.h"

Erc Wiz5100::loadCommon(uint8_t mac[6], uint8_t ipAddress[4],
		uint8_t gtwAddr[4], uint8_t netmask[4]) {
	int i;
	int erc = 0;
	if (erc)
		return erc;
	for (i = 0; i < 4; i++)
		erc += write(W5100_GAR + i, gtwAddr[i]); // set up the gateway address
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

int Wiz5100::socket() {
	return _socket;
}

int Wiz5100::init() {
	_spi.init();
	_timer.reload(true);
	_timer.start(10);
	return E_OK;
}

Erc Wiz5100::event(Event& event) {
	if (event.clsType() == Timer::EXPIRED)
		poll();
	return E_OK;
}

void Wiz5100::poll() {
	// poll socket
	uint8_t reg;
	read(Sx_IR(_socket), &reg);
	if (IR != reg) {
		upQueue(Wiz5100::INTERRUPT + reg);
		IR = reg;
	};
	write(Sx_IR(_socket), reg); //clear IR flags
	read(Sx_SR(_socket), &reg);
	if (SR != reg) {
		upQueue(Wiz5100::STATUS_CHANGE + reg);
		SR = reg;
	};
}



int Wiz5100::write(uint16_t address, uint8_t data) {
	uint32_t rxd=0;
	_spi.exchange(rxd,&rxd);
	int erc = _spi.exchange((W5100_WRITE_OPCODE << 24) + (address << 8) + data,
			&rxd);
	if (erc)
		return erc;
	if (rxd != 0x00010203)
		return E_AGAIN;
	return 0;
}

int Wiz5100::read(uint16_t address, uint8_t* data) {
	uint32_t rxd;
	int erc;
	if ((erc = _spi.exchange(
			(W5100_READ_OPCODE << 24) + (address << 8) + 0,
			&rxd)))
		return erc;
	if ((rxd & 0xFFFFFF00) != 0x00010200)
		return E_AGAIN;
	*data = rxd; // take LSB 1 byte
	return 0;
}

int Wiz5100::read16(uint16_t address, uint16_t* data) {
	int erc;
	uint8_t res1, res2;
	erc = read(address, &res1);
	if (erc)
		return erc;
	erc = read(address + 1, &res2);
	if (erc)
		return erc;
	*data = (res1 << 8) + res2;
	return 0;
}

int Wiz5100::write16(uint16_t address, uint16_t data) {
	int erc;
	erc = write(address, (uint8_t) (data >> 8));
	if (erc)
		return erc;
	erc = write(address + 1, (uint8_t) (data & 0xFF));
	if (erc)
		return erc;
	return 0;
}

int Wiz5100::socketCmd(uint8_t cmd) {
	int erc = write(Sx_CR(_socket), cmd); // put socket in listen state
	if (erc)
		return erc;
	while (true) {
		uint8_t cmdStatus;
		erc = read(Sx_CR(_socket), &cmdStatus);
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
void Wiz5100::setCR(uint8_t cmd) {
	write(Sx_CR(_socket), cmd);
}

uint8_t Wiz5100::getCR() {
	uint8_t cmd;
	read(Sx_CR(_socket), &cmd);
	return cmd;
}

uint8_t Wiz5100::getIR() {
	return IR;
}
/*
 void Wiz5100::setIR(uint8_t cmd) {
 write(Sx_IR(_socket), cmd);
 }*/

uint8_t Wiz5100::getSR() {
	return SR;
}

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

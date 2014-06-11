/*
 * File:   Tcp.cpp
 * Author: lieven
 *
 * Created on August 24, 2013, 9:39 PM
 */

#include "Tcp.h"
#include "Wiz5100.h"
#include "Sys.h"
#include "Led.h"
#include "Property.h"
#include "printf.h"

/*
 TCP : CONNECTED- CONNECTING - DISCONNECTED

 MQTT : CONNECTED - PUBLISH_STATIC - SUBSCRIBING - READY
 MQTT : READY : PING_WAIT_RESP, PING_SLEEP ( timeout : disconnect, ping_received :
 TREE : WAIT - SYNCING - UPDATING
 LED : BLINKING - STATIC
 TIMER : IDLE - COUNTING - EXPIRED
 */

// volatile uint8_t ir;
Led* led;
extern Led ledBlue;

Tcp::Tcp(Wiz5100* wiz, uint32_t socket) :
		_rxd(256), _txd(256) {
	led = &ledBlue;
	_wiz = wiz;
	_wiz->upStream(this);
	_state = ST_DISCONNECTED;
	_socket = socket;
	_srcPort = 2000;
	_bytesRxd = 0;
	_bytesTxd = 0;
	_connects = 0;
	_disconnects = 0;
	if (_socket == 0) {
		new Property(&_bytesRxd, T_UINT32, M_READ, "tcp/0/bytesRxd", "");
		new Property(&_bytesTxd, T_UINT32, M_READ, "tcp/0/bytesTxd", "");
		new Property(&_connects, T_UINT32, M_READ, "tcp/0/connects", "");
		new Property(&_disconnects, T_UINT32, M_READ, "tcp/0/disconnects", "");
	}
	if (_socket == 1) {
		new Property(&_bytesRxd, T_UINT32, M_READ, "tcp/1/bytesRxd", "");
		new Property(&_bytesTxd, T_UINT32, M_READ, "tcp/1/bytesTxd", "");
		new Property(&_connects, T_UINT32, M_READ, "tcp/1/connects", "");
		new Property(&_disconnects, T_UINT32, M_READ, "tcp/1/disconnects", "");
	}
}

Tcp::~Tcp() {

	while (1)
		;
}

int Tcp::init() {
	int i;
	int erc = 0;

	erc = _wiz->write(Sx_MR(_socket), W5100_SKT_MR_TCP); // set protocol for this socket
	incSrcPort();
	for (i = 0; i < 4; i++)
		erc += _wiz->write(Sx_DIPR(_socket) + i, _dstIp[i]);
	erc = _wiz->write(Sx_DPORT(_socket), _dstPort >> 8);
	erc = _wiz->write(Sx_DPORT(_socket) + 1, _dstPort & 0xFF);
	return 0;
}

Erc Tcp::incSrcPort() {

	Erc erc = 0;
	_srcPort++;
	erc = _wiz->write(Sx_PORT(_socket), ((_srcPort & 0xFF00) >> 8)); // set source port for this socket (MSB)
	erc = _wiz->write(Sx_PORT(_socket) + 1, (_srcPort & 0x00FF)); // set source port for this socket (LSB)
	return erc;
}

void Tcp::setDstIp(uint8_t * dstIp) {
	memcpy(_dstIp, dstIp, 4);
}

void Tcp::setDstPort(uint16_t port) {
	_dstPort = port;
}

void Tcp::state(State state) {
	if (_state == state)
		return;
	if (state == ST_DISCONNECTED) {
		_state = state;
		if (_socket == 0)
			led->blink(5);
		_txd.clear();
		_rxd.clear();
		upQueue(DISCONNECTED);
		_disconnects++;
	}
	if (state == ST_CONNECTED) {
		_state = state;
		_connects++;
		if (_socket == 0)
			led->blink(1);
		if (_txd.hasData())
			flush();
		upQueue(CONNECTED);
	}
	if (state == ST_CONNECTING) {

		_state = state;
	}
}

bool Tcp::isConnected() {

	return _state == ST_CONNECTED;
}

bool Tcp::isConnecting() {

	return _state == ST_CONNECTING;
}

Erc Tcp::event(Event* event) {
	switch (event->id()) {
	case Wiz5100::INTERRUPT: {
		if (event->detail() & W5100_SKT_IR_DISCON)
			state(ST_DISCONNECTED);
		if (event->detail() & W5100_SKT_IR_RECV)
			recvLoop();
		if (event->detail() & W5100_SKT_IR_SEND_OK)
			upQueue(Tcp::TXD);
		if (event->detail() & W5100_SKT_IR_CON)
			state(ST_CONNECTED);
		if (event->detail() & W5100_SKT_IR_TIMEOUT)
			state(ST_DISCONNECTED);
		break;
	}
	case Tcp::CMD_SEND: {
		flush();

		break;
	}
	}
	return E_OK;
}

Erc Tcp::connect() {
	incSrcPort();
	_wiz->write(Sx_IR(_socket), 1);
	_wiz->socketCmd(_socket, W5100_SKT_CR_OPEN);
	_wiz->socketCmd(_socket, W5100_SKT_CR_CONNECT);
	_rxd.clear();
	_txd.clear();
	uint8_t SR;
	uint8_t IR;
	while (true) {
		vTaskDelay(100);
		_wiz->read(Sx_IR(_socket), &IR);
		_wiz->write(Sx_IR(_socket), 1);
		if (IR & W5100_SKT_IR_CON)
			return E_OK;
		else if (IR & W5100_SKT_IR_DISCON) {
//			disconnect();
			return E_CONN_LOSS;
		} else if (IR & W5100_SKT_IR_TIMEOUT) {
//			disconnect();
			_wiz->read(Sx_SR(_socket), &SR);
			return E_CONN_LOSS;
		}
		taskYIELD();
	};
	return E_OK;
}

void Tcp::disconnect() {

	_wiz->socketCmd(_socket, W5100_SKT_CR_DISCON);
	_wiz->socketCmd(_socket, W5100_SKT_CR_CLOSE);
	_rxd.clear();
	_txd.clear();
}

int Tcp::recv(Bytes& msg) {
	if (_state != ST_CONNECTED)
		return 0;
	int count = 0;
	while (_rxd.hasData() && msg.hasSpace()) {

		msg.write(_rxd.read());
		count++;
	}
	return count;
}

Erc Tcp::recvLoop() {
	uint16_t offaddr;
	uint16_t size;
	uint16_t realaddr;
	uint8_t data;
	Erc erc = 0;
	erc |= _wiz->read16(Sx_RX_RD(_socket), &offaddr);
	erc |= _wiz->read16(Sx_RX_RSR(_socket), &size);
	//    ASSERT(size > 0);
	if (size == 0)
		return erc;
	ASSERT(_rxd.hasSpace());
	while (_rxd.hasSpace() && size > 0) {
		realaddr = Sx_RX_BASE(_socket) + (offaddr & W5100_RX_BUF_MASK);
		erc = _wiz->read(realaddr, &data);
		_rxd.write(data);
		size--;
		offaddr++;
		_bytesRxd++;
	}
	erc |= _wiz->write16(Sx_RX_RD(_socket), offaddr + size); // Increase the S0_RX_RD value, so it point to the next receive
	erc |= _wiz->write(Sx_CR(_socket), W5100_SKT_CR_RECV); // issue the receive command
	upQueue(Tcp::RXD);

	return erc;
}

bool Tcp::hasData() {

	return _rxd.hasData();
}

uint8_t Tcp::read() {

	return _rxd.read();
}

Erc Tcp::write(Bytes* msg) {
	int count = 0;
	msg->offset(0);
	while (_txd.hasSpace() && msg->hasData()) {
		_txd.write(msg->read());
		count++;
	}
	return E_OK;
}

Erc Tcp::flush() {
	uint16_t offaddr;
	uint16_t size;
	uint16_t realaddr;
	int erc = E_OK;
	if (_txd.hasData() == false)
		return E_LACK_RESOURCE;
	while (_txd.hasData()) {
		erc = _wiz->read16(Sx_TX_FSR(_socket), &size); // make sure the TX free-size reg is available
		if (size == 0)
			return E_NOT_FOUND;
		erc = _wiz->read16(Sx_TX_WR(_socket), &offaddr); // Read the Tx Write Pointer
		while (_txd.hasData() && size > 0) {
			realaddr = Sx_TX_BASE(_socket) + (offaddr & W5100_TX_BUF_MASK); // calc W5100 physical buffer addr for this socket
			_wiz->write(realaddr, _txd.read()); // send a byte of application data to TX buffer
			offaddr++; // next TX buffer addr
			size--;
			_bytesTxd++;
		}
		erc = _wiz->write16(Sx_TX_WR(_socket), offaddr);
		_wiz->socketCmd(_socket, W5100_SKT_CR_SEND);
		if (erc)
			return erc;
	}
	return erc;
}

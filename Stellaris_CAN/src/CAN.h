/*
 * CAN.h
 *
 *  Created on: 30-nov.-2013
 *      Author: lieven2
 */

#ifndef CAN_H_
#define CAN_H_
#include <stdint.h>
#include <Stream.h>
#include "Erc.h"
#include "Sys.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "driverlib/can.h"
class CANListener {
public:
	virtual void recv(uint32_t id, uint32_t length, uint8_t* pdata)=0;
	virtual void sendDone(uint32_t id, Erc erc)=0;
};
#define MAX_MSG_OBJECT	32
class CAN: public Stream {
public:
	enum {
		RXD, TXD, ERR
	} CANEvent;
	typedef enum {
		SEND,RECV
	} Mode;
	CAN(uint32_t port);
	virtual ~CAN();
	Erc event(Event& event);
	void init();

	uint32_t getMailbox(CANListener* listener, uint32_t src, uint32_t srcMask);
	void send(uint32_t mailbox, uint32_t dstAddress, uint8_t *data,
			uint32_t length);
	uint32_t _countRxd;
	uint32_t _countTxd;
	uint32_t _countErr;
	struct {
		tCANMsgObject* msgObject;
		CANListener* listener;
		Mode mode;
		uint32_t src;
		uint32_t srcMask;
	} _table[MAX_MSG_OBJECT];
	uint32_t _tableIdx;
private:
	uint32_t _port;
	uint32_t _CAN_BASE;
	uint32_t _CAN_INT;
	uint32_t _srcAddress;
};

#endif /* CAN_H_ */

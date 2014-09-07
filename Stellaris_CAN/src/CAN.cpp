/*
 * CAN.cpp
 *
 *  Created on: 30-nov.-2013
 *      Author: lieven2
 */

#include "CAN.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

CAN can0(0);

CAN::CAN(uint32_t port) {
	_port = port;
	if (port == 0) {
		_CAN_BASE = CAN0_BASE;
		_CAN_INT = INT_CAN0;
	}
	_countErr = 0;
	_countRxd = 0;
	_countTxd = 0;
	_tableIdx = 0;
}

CAN::~CAN() {

}
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
void CAN::init() {
	// Initialize the CAN controller
	CANInit(_CAN_BASE);
	CANBitRateSet(_CAN_BASE, SysCtlClockGet(), 125000);
	CANIntEnable(_CAN_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
	IntEnable(_CAN_INT);
	HWREG(CAN0_BASE + CAN_O_CTL) |= CAN_CTL_TEST;
	HWREG(CAN0_BASE + CAN_O_TST) = CAN_TST_LBACK | CAN_TST_SILENT;
	CANEnable(_CAN_BASE);
	// Configure LoopBack mode
}

uint32_t CAN::getMailbox(CANListener* listener, uint32_t src,
		uint32_t srcMask) {
	ASSERT(_tableIdx<32);
	_table[_tableIdx].listener = listener;
	_table[_tableIdx].src = src;
	_table[_tableIdx].mode = RECV;
	_table[_tableIdx].srcMask = srcMask;
	_table[_tableIdx].msgObject = (tCANMsgObject*) Sys::malloc(
			sizeof(tCANMsgObject));
	_table[_tableIdx].msgObject->pucMsgData = (unsigned char*) Sys::malloc(8);
	_table[_tableIdx].msgObject->ulMsgID = src;
	_table[_tableIdx].msgObject->ulMsgIDMask = srcMask;
	_table[_tableIdx].msgObject->ulMsgLen = 8;
	_table[_tableIdx].msgObject->ulFlags = MSG_OBJ_RX_INT_ENABLE
			| MSG_OBJ_USE_ID_FILTER;
	CANMessageSet(_CAN_BASE, _tableIdx + 1, _table[_tableIdx].msgObject,
			MSG_OBJ_TYPE_RX);
//	CANIntEnable(_CAN_BASE, _tableIdx + 1);
	_tableIdx++;
	_table[_tableIdx].msgObject = (tCANMsgObject*) Sys::malloc(
			sizeof(tCANMsgObject));
	_table[_tableIdx].listener = listener;
	_table[_tableIdx].mode = SEND;
	_tableIdx++;
	return _tableIdx - 2;
}

Erc CAN::event(Event& event) {
	switch (event.detail()) {
	case RXD: {
		break;
	}
	case TXD: {
		break;
	}
	}
	return E_OK;
}

void CAN::send(uint32_t mailbox, uint32_t dstAddress, uint8_t* data,
		uint32_t length) {
	ASSERT(length<9);
	uint32_t idx = mailbox + 1; // +1 for transmission
	tCANMsgObject* mo = _table[idx].msgObject;
	_table[idx].mode = SEND;

	mo->ulMsgID = dstAddress; // CAN message ID
	mo->ulMsgIDMask = 0; // no mask needed for TX
	mo->ulFlags = MSG_OBJ_TX_INT_ENABLE + MSG_OBJ_FIFO; // enable interrupt on TX
	mo->ulMsgLen = length; // size of message is length
	mo->pucMsgData = data; // ptr to message content
	CANIntEnable(_CAN_BASE, idx + 1);
	CANMessageSet(_CAN_BASE, idx + 1, mo, MSG_OBJ_TYPE_TX);
	/*	uint32_t st;
	 while ((st = CANStatusGet(_CAN_BASE, CAN_STS_TXREQUEST)) == 0) {

	 }
	 CANMessageGet(_CAN_BASE, st, mo, true);
	 while (CANStatusGet(_CAN_BASE, CAN_STS_TXREQUEST) == 0) {

	 }
	 st = CANStatusGet(_CAN_BASE, CAN_STS_TXREQUEST);*/
}

//*****************************************************************************
//
// This function provides a 1 second delay using a simple polling method.
//
//*****************************************************************************
void SimpleDelay(void) {
//
// Delay cycles for 1 second
//
	MAP_SysCtlDelay(16000000 / 3);
}

//*****************************************************************************
//
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt, and maintains a count of all messages that
// have been transmitted.
//
//*****************************************************************************
extern "C" void CAN0IntHandler(void) {
	unsigned long ulStatus;
// Read the CAN interrupt status to find the cause of the interrupt
	ulStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

//
// If the cause is a controller status interrupt, then get the status
//
	if (ulStatus == CAN_INT_INTID_STATUS) {
		//
		// Read the controller status.  This will return a field of status
		// error bits that can indicate various errors.  Error processing
		// is not done in this example for simplicity.  Refer to the
		// API documentation for details about the error status bits.
		// The act of reading this status will clear the interrupt.  If the
		// CAN peripheral is not connected to a CAN bus with other CAN devices
		// present, then errors will occur and will be indicated in the
		// controller status.
		//
		ulStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
		if (ulStatus == CAN_STATUS_TXOK) {
		} else if (ulStatus == CAN_STATUS_RXOK) {
		} else
			/*
			 CAN_STS_CONTROL - the main controller status
			 CAN_STS_TXREQUEST - bit mask of objects pending transmissio
			 CAN_STS_NEWDAT - bit mask of objects with new data
			 CAN_STS_MSGVAL - bit mask of objects with valid configuration

			 CAN_STATUS_BUS_OFF - controller is in bus-off condition
			 CAN_STATUS_EWARN - an error counter has reached a limit of at least 96
			 CAN_STATUS_EPASS - CAN controller is in the error passive state
			 CAN_STATUS_RXOK - a message was received successfully (independent of any mes-sage filtering).
			 CAN_STATUS_TXOK - a message was successfully transmitted
			 CAN_STATUS_LEC_MSK - mask of last error code bits (3 bits)
			 CAN_STATUS_LEC_NONE - no error
			 CAN_STATUS_LEC_STUFF - stuffing error detected
			 CAN_STATUS_LEC_FORM - a format error occurred in the fixed format part of amessage
			 CAN_STATUS_LEC_ACK - a transmitted message was not acknowledged
			 CAN_STATUS_LEC_BIT1 - dominant level detected when trying to send in recessive
			 mode
			 CAN_STATUS_LEC_BIT0 - recessive level detected when trying to send in dominant
			 mode
			 CAN_STATUS_LEC_CRC - CRC error in received message
			 */
			can0._countErr++;
	}

//
// Check if the cause is message object 1, which what we are using for
// sending messages.
//
	else if (ulStatus < can0._tableIdx + 1 && ulStatus > 0) {
		uint32_t idx = ulStatus - 1;
		if (can0._table[idx].mode == CAN::RECV) {
			tCANMsgObject* msgObj = (can0._table[idx].msgObject);
			CANIntClear(CAN0_BASE, idx + 1);
			CANMessageGet(CAN0_BASE, idx + 1, msgObj, true);
			can0._countRxd++;
			CANListener* listener = can0._table[idx].listener;
			listener->recv(msgObj->ulMsgID, msgObj->ulMsgLen,
					msgObj->pucMsgData);
			CANMessageSet(CAN0_BASE, ulStatus, msgObj, MSG_OBJ_TYPE_RX);
//			CANMessageSet(CAN0_BASE, ulStatus, msgObj, MSG_OBJ_TYPE_TX); // reload
		} else if (can0._table[idx].mode == CAN::SEND) {
			CANListener* listener = can0._table[idx].listener;
			tCANMsgObject* msgObj = (can0._table[idx].msgObject);
			CANIntClear(CAN0_BASE, idx + 1);
			can0._countTxd++;
			listener->sendDone(msgObj->ulMsgID, E_OK);
		}
	}

	else {
		//
		// Spurious interrupt handling can go here.
		//
		CANIntClear(CAN0_BASE, ulStatus);
	}
}


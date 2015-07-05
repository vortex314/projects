/*
 * Usb.cpp
 *
 *  Created on: 5-jul.-2015
 *      Author: lieven2
 */

#include "Usb.h"



Usb usb;
Usb::Usb() :_txData(100),_rxData(100) {
	_isConnected =false;
}

Usb::~Usb() {
	// TODO Auto-generated destructor stub
}

extern "C" {
	void onUsbConnected(){
		usb._isConnected=true;
	}
	void onUsbDisconnected(){
		usb._isConnected=false;
	}
	void onUsbRxData(uint8_t* pData,uint32_t length){
		int i;
		for(i=0;i<length;i++)
			usb._rxData.writeFromIsr(pData[i]);
	}
	uint32_t usbTxDataSize() {
		if ( !usb._isConnected ) return 0;
 		return usb._txData.size();
	}
	uint32_t usbGetTxData(uint8_t* pData,uint32_t length){
		uint32_t offset=0;
		while ( usbTxDataSize() && offset<length)
			pData[offset++]=usb._txData.read();
		return offset;
	}
}

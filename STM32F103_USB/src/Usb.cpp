/*
 * Usb.cpp
 *
 *  Created on: 5-jul.-2015
 *      Author: lieven2
 */

#include "Usb.h"


Usb::Usb() {
	// TODO Auto-generated constructor stub

}

Usb::~Usb() {
	// TODO Auto-generated destructor stub
}

extern "C" {
	void onUsbConnected();
	void onUsbDisconnected();
	void onUsbData(uint8_t* pData,uint16_t length);
}

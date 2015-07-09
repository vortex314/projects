/*
 * Usb.h
 *
 *  Created on: 5-jul.-2015
 *      Author: lieven2
 */

#ifndef SRC_USB_H_
#define SRC_USB_H_
#include <stdint.h>
#ifdef __cplusplus
class Usb {
public:
	Usb();
	virtual ~Usb();
};
#endif
extern "C" {
	void onUsbConnected();
	void onUsbDisconnected();
	void onUsbData(uint8_t* pData,uint16_t length);
}


#endif /* SRC_USB_H_ */

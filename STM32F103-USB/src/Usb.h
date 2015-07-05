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
#include <CircBuf.h>
#include <Handler.h>
class Usb {
public:
	Usb();
	virtual ~Usb();
	bool _isConnected;
	CircBuf _txData;
	CircBuf _rxData;

};
#endif
#ifdef __cplusplus
extern "C" {
#endif
	void onUsbConnected();
	void onUsbDisconnected();
	void onUsbRxData(uint8_t* pData,uint32_t length);
	uint32_t usbTxDataSize();
	uint32_t usbGetTxData(uint8_t* pData,uint32_t length);
#ifdef __cplusplus
}
#endif

#endif /* SRC_USB_H_ */


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
#include <Link.h>
#include <MqttIn.h>
class Usb : public Link {
public:
	Usb();
	virtual ~Usb();
	MqttIn _mqttIn;
	CircBuf _out;
	CircBuf _in;
	Handler* _device;
	static void init();
	void reset();
	Erc connect();
	Erc disconnect();
	Erc send(Bytes& bytes);
	Erc recv(Bytes& bytes);
	uint8_t read();
	uint32_t hasData();
	bool dispatch(Msg& event) ;
};
#endif
// calleable from C code and interrupt routines
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

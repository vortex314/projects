
/*
 * Usb.cpp
 *
 *  Created on: 5-jul.-2015
 *      Author: lieven2
 */

#include "Usb.h"



Usb usb;
Usb::Usb() :_mqttIn(100),_out(100),_in(100) {
	_isConnected =false;
	_device = (Handler*)malloc(1);
}

Usb::~Usb() {

}

void Usb::reset() {
	_mqttIn.reset();
	_out.clear();
	_in.clear();
}

Erc Usb::connect() {
	return E_OK;
}
Erc Usb::disconnect() {
	return E_OK;
}

Erc Usb::send(Bytes& bytes) {
	bytes.AddCrc();
	bytes.Encode();
	bytes.Frame();
	bytes.offset(0);
	while (bytes.hasData() && _out.hasSpace()) {
	 _out.write(bytes.read());
	 };
	if (bytes.hasData() == false)
		return E_OK;
	return EAGAIN;
}

Erc Usb::recv(Bytes& bytes) {
	return E_AGAIN;
}

uint8_t Usb::read() {
	return _in.read();
	/*	uint8_t b;
	 USBBufferRead((tUSBBuffer *) &g_sRxBuffer, &b, 1);
	 return b;*/
}

uint32_t Usb::hasData() {
	return _in.hasData();
//	return USBBufferDataAvailable((tUSBBuffer *) &g_sRxBuffer);
}

bool Usb::dispatch(Msg& event) {
	if ( event.is(_device,0))
	switch (event.sig()) {

	case SIG_CONNECTED: {
		reset();
		isConnected(true);
//      publish(Link::CONNECTED);
		break;
	}
	case SIG_DISCONNECTED: {
		isConnected(false);
//      publish(Link::DISCONNECTED);
		break;
	}
	case (SIG_RXD):

	{
		uint8_t b;
		while (hasData()) {
			b = read();
			if (_mqttIn.getBytes()->Feed(b)) {
				_mqttIn.getBytes()->Decode();
				if (_mqttIn.getBytes()->isGoodCrc()) {
					_mqttIn.getBytes()->RemoveCrc();
					_mqttIn.parse();
					MsgQueue::publish(this,SIG_RXD,0,(void*)&_mqttIn);

				} else
					_mqttIn.reset();
			}
		}
		break;
	}
	default: {
	}
		;
	}
	return true;
}

extern "C" {
	void onUsbConnected(){
		usb._isConnected=true;
	}
	void onUsbDisconnected(){
		usb._isConnected=false;
	}
	void onUsbRxData(uint8_t* pData,uint32_t length){
		uint16_t i;
		for(i=0;i<length;i++)
			usb._in.writeFromIsr(pData[i]);
		MsgQueue::publish(usb._device,SIG_RXD);
	}
	uint32_t usbTxDataSize() {
		if ( !usb._isConnected ) return 0;
 		return usb._out.size();
	}
	uint32_t usbGetTxData(uint8_t* pData,uint32_t length){
		uint32_t offset=0;
		while ( usbTxDataSize() && offset<length)
			pData[offset++]=usb._out.read();
		return offset;
	}
}

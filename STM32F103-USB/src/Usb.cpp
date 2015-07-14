/*
 * Usb.cpp
 *
 *  Created on: 5-jul.-2015
 *      Author: lieven2
 */

#include "Usb.h"

Usb usb;
class UsbDevice : public Handler {
	bool dispatch(Msg& event){
		return true;
	}

};
Usb::Usb() :
		_mqttIn(100), _out(100), _in(100) , _inBuffer(256){
	_device = new UsbDevice();
}

Usb::~Usb() {

}

void Usb::free(void* ptr){
	delete (MqttIn*)ptr;
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
}

uint32_t Usb::hasData() {
	return _in.hasData();
}

bool Usb::dispatch(Msg& event) {
	if (event.is(_device, 0))
		switch (event.sig()) {

		case SIG_CONNECTED: {
			reset();
			isConnected(true);
			MsgQueue::publish(this,SIG_CONNECTED);
			break;
		}
		case SIG_DISCONNECTED: {
			isConnected(false);
			MsgQueue::publish(this,SIG_DISCONNECTED);
			break;
		}
		case (SIG_RXD):

		{
			uint8_t b;
			while (_in.hasData())
					{
						b = _in.read();
						if (_inBuffer.Feed(b))
						{
							_inBuffer.Decode();
							if (_inBuffer.isGoodCrc())
							{
								_inBuffer.RemoveCrc();
								MqttIn* mqttIn = new MqttIn(_inBuffer.length());
								_inBuffer.offset(0);
								if (mqttIn == 0)
									while (1)
										;
								while (_inBuffer.hasData())
									mqttIn->Feed(_inBuffer.read());
								mqttIn->parse();
								MsgQueue::publish(this, SIG_RXD, mqttIn->type(), mqttIn);
							}
							else
							{
								Sys::warn(EIO, "USB_CRC");
							}
							_inBuffer.clear();
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
void onUsbConnected() {
	MsgQueue::publish(usb._device, SIG_CONNECTED);
}
void onUsbDisconnected() {
	MsgQueue::publish(usb._device, SIG_DISCONNECTED);
}
void onUsbRxData(uint8_t* pData, uint32_t length) {
	uint16_t i;
	for (i = 0; i < length; i++)
		usb._in.writeFromIsr(pData[i]);
	MsgQueue::publish(usb._device, SIG_RXD);
}
uint32_t usbTxDataSize() {
	if (!usb.isConnected())
		return 0;
	return usb._out.size();
}
uint32_t usbGetTxData(uint8_t* pData, uint32_t length) {
	uint32_t offset = 0;
	while (usbTxDataSize() && offset < length)
		pData[offset++] = usb._out.read();
	return offset;
}
}

/*
 * Spi.cpp
 *
 *  Created on: 20-dec.-2013
 *      Author: lieven2
 */

#include "Spi.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "driverlib/ssi.h"

#define SSI0_BASE    0x40008000  // SSI0
Spi::Spi(int idx) {

}

void Spi::init() {
	//
	// Configure the SSI.
	//
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, 100000, 8);
	//
	// Enable the SSI module.
	//
	SSIEnable(SSI0_BASE);

}

Erc Spi::exchange(Bytes& in, Bytes& out) {
	unsigned long w;

	out.offset(0);
	while (out.hasData()) {
		while (SSIBusy(SSI0_BASE))
			;
		for (int i = 0; i < 4; i++)
			SSIDataPut(SSI0_BASE, out.read());
		while (SSIBusy(SSI0_BASE))
			;
		for (int i = 0; i < 4; i++) {
			SSIDataGet(SSI0_BASE, &w);
			in.write(w);
		};

	}
	return E_OK;
}
#ifdef BIG_ENDIAN
typedef union {
	uint32_t i32;
	struct {
		uint8_t b3;
		uint8_t b2;
		uint8_t b1;
		uint8_t b0;
	} bytes;
} Converter;
#else
typedef union {
	uint32_t i32;
	struct {
		uint8_t b0;
		uint8_t b1;
		uint8_t b2;
		uint8_t b3;
	} bytes;
}Converter;
#endif
Erc Spi::exchange(uint32_t out,uint32_t* in) {
	Converter cvOut;
	Converter cvIn;
	cvOut.i32 = out;
	while (SSIBusy(SSI0_BASE))
		;
	SSIDataPut(SSI0_BASE, cvOut.bytes.b0);
	SSIDataPut(SSI0_BASE, cvOut.bytes.b1);
	SSIDataPut(SSI0_BASE, cvOut.bytes.b2);
	SSIDataPut(SSI0_BASE, cvOut.bytes.b3);
	while (SSIBusy(SSI0_BASE))
		;
	unsigned long l;
	SSIDataGet(SSI0_BASE, &l);
	cvIn.bytes.b0=l;
	SSIDataGet(SSI0_BASE, &l);
	cvIn.bytes.b1=l;
	SSIDataGet(SSI0_BASE, &l);
	cvIn.bytes.b2=l;
	SSIDataGet(SSI0_BASE, &l);
	cvIn.bytes.b3=l;
	*in=cvIn.i32;
	return E_OK;
}

Spi::~Spi() {

}


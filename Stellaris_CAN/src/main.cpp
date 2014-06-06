#include <stdio.h>
#include <inc/hw_can.h>
#include <inc/hw_sysctl.h>
#include "Sys.h"
#include <driverlib/systick.h>
#include "Stream.h"
#include "Event.h"
#include "Timer.h"
#include "Led.h"
#include "CAN.h"
#include "Spi.h"
#include "Wiz5100.h"
#include "Bytes.h"

class CANTest: public CANListener {
public:
	CAN& _can;
	uint32_t mb;
	CANTest(CAN& can) : _can(can) {
		_can = can;
	}

	~CANTest() {
	}
	void init() {
		_can.init();
		mb = _can.getMailbox(this, 0, 0);
		_can.send(mb,0xF,(uint8_t*)"ABCD",4);
	}
	void recv(uint32_t id, uint32_t length, uint8_t* pdata) {

	}
	void sendDone(uint32_t id, Erc erc) {

	}
};

extern CAN can0;
int main(void) {
	Event event;
	Sys::init();

	Led ledGreen(Led::LED_GREEN);
	Led ledRed(Led::LED_RED);
	Led ledBlue(Led::LED_BLUE);
	Spi spi0(0);

	ledGreen.blink(1);
	ledRed.light(false);
	ledBlue.light(false);


	Wiz5100 wiz(0);
	wiz.init();
	wiz.reset();
	Bytes out(10);
	uint8_t array[]={0x1,0x2};
	out.write(array,0,4);
	spi0.exchange(out,out);

	CANTest ct(can0);
	ct.init();

while (true) { // EVENTPUMP
	if (Stream::dequeue(&event) == 0) { // event waiting
		event.dest()->event(event);
	}
}
}

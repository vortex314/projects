#include "Sys.h"
#include "Led.h"
#include "Uart.h"
#include "Str.h"
#include "MqttUart.h"
#include "MqttIn.h"
#include "MqttOut.h"
#include "Property.h"
extern Uart uart1;

uint64_t eventCount = 0;
Led ledGreen(Led::LED_GREEN);
Led ledBlue(Led::LED_BLUE);
MqttUart mqtt(&uart1);

#include <stm32f10x.h>


extern "C" int main(void) {
	Sys::init();
	ledGreen.init();
	ledBlue.init();


	new Property(&eventCount, T_UINT32, M_RW,
			"system/eventCounter", "\"desc\":\"count total number of events processed\"");

	ledBlue.blink(1);
	uart1.init();
	mqtt.init();
	uart1.upStream(&mqtt);

	Event event;
	Sys::log("System started");

	while (true) { // EVENTPUMP
		if (Stream::dequeue(&event) == 0) { // event waiting
			eventCount++;
			event.dest()->event(event);
		}
	}
	return 0;
}
#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line){
	while(1);
}
#endif

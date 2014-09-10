/*
 * main.cpp
 *
 *  Created on: 23-aug.-2014
 *      Author: lieven2
 */

#include "Usb.h"
#include "Board.h"
#include "Mqtt.h"
#include "Timer.h"
#include "Prop.h"

#include <malloc.h>
/*
 class MeasureSeq : public Fsm
 =======

 #define LED_INTERVAL 100
 class LedSeq : public Sequence
 {
 private:
 struct pt t;

 public:
 LedSeq ()
 {
 PT_INIT(&t);
 }
 int
 handler (Event* event)
 {
 PT_BEGIN ( &t )
 while (true)
 {
 timeout (250);
 PT_YIELD_UNTIL(&t, timeout ());
 Board::setLedOn (Board::LED_GREEN, true);
 timeout (250);
 PT_YIELD_UNTIL(&t, timeout ());
 Board::setLedOn (Board::LED_GREEN, false);
 }
 PT_END ( &t );
 }
 };
 #include <malloc.h>

 class MeasureSeq : public Sequence
 >>>>>>> 709d62f69d76b208f0276c399a72d145b54fd6dd
 {
 private:
 struct pt t;
 Mqtt& _mqtt;
 public:
 MeasureSeq (Mqtt& mqtt);
 void
 measure ();
 int
 handler (Event* event);
 static void
 heapMemory (void* instance, Cmd cmd, Strpack& strp);
 };

 <<<<<<< HEAD
 MeasureSeq::MeasureSeq (Mqtt& mqtt) :
 _mqtt (mqtt)
 =======
 MeasureSeq::MeasureSeq (Mqtt& mqtt) : _mqtt(mqtt)
 >>>>>>> 709d62f69d76b208f0276c399a72d145b54fd6dd
 {
 PT_INIT(&t);
 }
 void
 MeasureSeq::measure ()
 {
 struct mallinfo m;
 m = mallinfo ();
 }

 int
 MeasureSeq::handler (Event* event)
 {
 struct mallinfo m;
 m = mallinfo ();
 PT_BEGIN ( &t )
 while (true)
 {
 timeout (5000);
 PT_YIELD_UNTIL(&t, timeout ());
 Strpack strp (100);
 <<<<<<< HEAD
 Str topic (10);
 topic.set ("system/heapUsage");
 Strpack value (10);
 value.append ((uint32_t) m.uordblks);
 Flags flags =
 { T_STR, M_WRITE, QOS_1, I_OBJECT, true };
 _mqtt.Publish (flags, 1, topic, value);
 =======
 Str  topic(10);
 topic.set("system/heapUsage");
 Strpack value(10);
 value.append((uint32_t) m.uordblks);
 Flags flags = {
 T_STR, M_WRITE, QOS_1, I_OBJECT, true
 };
 _mqtt.Publish(flags,1,topic,value);
 >>>>>>> 709d62f69d76b208f0276c399a72d145b54fd6dd
 }
 PT_END ( &t );
 }
 void
 MeasureSeq::heapMemory (void* instance, Cmd cmd, Strpack& strp)
 {
 struct mallinfo m;
 m = mallinfo ();
 static const char* const desc = " desc:'heapUsage'";
 if (cmd == CMD_PUT)
 {
 }
 else if (cmd == CMD_GET)
 {
 strp.append ((uint32_t) m.uordblks);
 }
 else if (cmd == CMD_DESC)
 {
 strp.append (desc);
 }
 <<<<<<< HEAD
 }*/

#include "Fsm.h"

class LedBlink: public Fsm {
	bool _isOn;
	uint32_t _msecInterval;
public:
	LedBlink() {
		init(static_cast<SF>(&LedBlink::blink));
		timeout(1000);
		_isOn = false;
		_msecInterval = 500;
	}
	virtual ~LedBlink(){};

	void blink(Event& event) {
		switch (event.id()) {
		case SIG_TIMER_TICK: {
			if (timeout()) {
				Board::setLedOn(Board::LED_GREEN, _isOn);
				_isOn = !_isOn;
				timeout(_msecInterval);
			}
			break;
		}
		case SIG_MQTT_CONNECTED: {
			_msecInterval = 100;
			break;
		}
		case SIG_MQTT_DISCONNECTED: {
			_msecInterval = 500;
			break;
		}
		}
	}
};
LedBlink ledBlink;

void eventPump() {
	Event event;
	while (Event::nextEvent(event) == E_OK) {
		Fsm::dispatchToAll(event);
	}
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
#include "BipBuffer.h"
BipBuffer bb;

int main(void) {

	bb.AllocateBuffer(1024);
	int size;
	bb.Reserve(10,size);
	bb.Commit(size);
	bb.GetContiguousBlock(size);

	bb.FreeBuffer();

	Board::init();	// initialize usb
	Usb::init();
	Usb usb;		// usb active object
	Mqtt mqtt(usb);	// mqtt active object

	PropertyListener propertyListener(mqtt);
	uint64_t clock = Sys::upTime() + 100;
	while (1) {
		eventPump();
		if (Sys::upTime() > clock) {
			clock += 10;
			Fsm::publish(SIG_TIMER_TICK);
		}
	}
}
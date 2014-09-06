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
/*
 #define LED_INTERVAL 100
 class LedSeq: public Sequence
 {
 private:
 struct pt t;

 public:
 LedSeq()
 {
 PT_INIT(&t);
 }
 int handler(Event* event)
 {
 PT_BEGIN ( &t )
 while (true)
 {
 timeout(250);
 PT_YIELD_UNTIL(&t, timeout());
 Board::setLedOn(Board::LED_GREEN, true);
 timeout(250);
 PT_YIELD_UNTIL(&t, timeout());
 Board::setLedOn(Board::LED_GREEN, false);
 }
 PT_END ( &t );
 }
 };*/
#include <malloc.h>
/*
class MeasureSeq : public Fsm
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

MeasureSeq::MeasureSeq (Mqtt& mqtt) :
    _mqtt (mqtt)
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
	Str topic (10);
	topic.set ("system/heapUsage");
	Strpack value (10);
	value.append ((uint32_t) m.uordblks);
	Flags flags =
	  { T_STR, M_WRITE, QOS_1, I_OBJECT, true };
	_mqtt.Publish (flags, 1, topic, value);
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
}*/

#include "Fsm.h"

class LedBlink : public Fsm
{
bool _isOn;
uint32_t _msecInterval;
public:
LedBlink ()
{
  init (static_cast<SF> (&LedBlink::blink));
  timeout (1000);
  _isOn = false;
  _msecInterval = 500;
}

void
blink (Event& event)
{
  switch (event.id ())
    {
    case SIG_TIMER_TICK:
      {
	if (timeout ())
	  {
	    Board::setLedOn (Board::LED_GREEN, _isOn);
	    _isOn = !_isOn;
	    timeout (_msecInterval);
	  }
	break;
      }
    case SIG_MQTT_CONNECTED:
      {
	_msecInterval = 100;
	break;
      }
    case SIG_MQTT_DISCONNECTED:
      {
	_msecInterval = 500;
	break;
      }
    }
}
};
LedBlink ledBlink;

void
eventPump ()
{
Event event;
while (Event::nextEvent(event) == E_OK)
  {
	Fsm::dispatchToAll(event);
  }
}


//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************

int
main (void)
{
Board::init ();	// initialize usb

Usb::init ();
Usb usb;		// usb active object
//LedSeq ledSeq; 	// blinking led AO

Mqtt mqtt (usb);	// mqtt active object

PropertyListener propertyListener (mqtt);
uint64_t clock = Sys::upTime () + 100;
while (1)
  {
    eventPump ();
    if (Sys::upTime () > clock)
      {
	clock += 10;
	Fsm::publish (SIG_TIMER_TICK);
      }
  }
}

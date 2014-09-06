//*****************************************************************************
//
// usb_dev_serial.c - Main routines for the USB CDC serial example.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"
#include "usb_serial_structs.h"

#include "MqttIn.h"
#include "MqttOut.h"
#include "CircBuf.h"
#include "Queue.h"
#include "Event.h"
#include "Sequence.h"
#include "Timer.h"
#include "Log.h"
#include "pt.h"
#include "Board.h"

#include "Property.h"

#include "Usb.h"
Usb usb;

//*******************************************************************************
//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 1000
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
static volatile tBoolean g_bUSBConfigured = false;

#define MAX_MSG_SIZE 100

Usb::Usb () :
    mqttIn (MAX_MSG_SIZE), in (256)
{
  _isComplete = false;
}

void
Usb::reset ()
{
  in.clear ();
  _isComplete = false;
}

Erc
Usb::connect ()
{
  return E_OK;
}
Erc
Usb::disconnect ()
{
  return E_OK;
}

Erc
Usb::send (Bytes& bytes)
{
  bytes.AddCrc ();
  bytes.Encode ();
  bytes.Frame ();
  bytes.offset (0);
  while (USBBufferSpaceAvailable ((tUSBBuffer *) &g_sTxBuffer)
      && bytes.hasData ())
    {
      uint8_t b;
      b = bytes.read ();
      USBBufferWrite ((tUSBBuffer *) &g_sTxBuffer, (unsigned char *) &b, 1);
    }
  if (bytes.hasData () == false)
    return E_OK;
  return EAGAIN;
}

MqttIn*
Usb::recv ()
{
  if (_isComplete)
    return &mqttIn;
  else
    return 0;
}

uint8_t
Usb::read ()
{
  uint8_t b;
  USBBufferRead ((tUSBBuffer *) &g_sRxBuffer, &b, 1);
  return b;
}

uint32_t
Usb::hasData ()
{
  return USBBufferDataAvailable ((tUSBBuffer *) &g_sRxBuffer);
}

int
Usb::handler (Event* event)
{
  if (event->is (SIG_USB_FREE))
    {
      mqttIn.clear ();
      _isComplete = false;
    }
  else if (event->is (SIG_USB_CONNECTED))
    {
      reset ();
      isConnected (true);
//      publish(Link::CONNECTED);
    }
  else if (event->is (SIG_USB_DISCONNECTED))
    {
      isConnected (false);
//      publish(Link::DISCONNECTED);
    }
  else if (event->is (SIG_USB_RXD))
    {
      uint8_t b;
      while (hasData () && !_isComplete)
	{
	  b = read ();
	  if (mqttIn.Feed (b))
	    {
	      mqttIn.Decode ();
	      if (mqttIn.isGoodCrc ())
		{
		  mqttIn.RemoveCrc ();
		  mqttIn.parse ();
		  _isComplete = true;
		  publish (SIG_MQTT_MESSAGE, mqttIn.type ());
		  publish (SIG_USB_FREE);
		}
	      else
		mqttIn.clear ();
	    }
	}

    }
  return 0;
}

void
Usb::init ()
{
  //
  // Configure the required pins for USB operation.
  //
  ROM_SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOD);
  ROM_GPIOPinTypeUSBAnalog (GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);
  //
  // Not configured initially.
  //
  g_bUSBConfigured = false;
  //
  // Enable the system tick.
  //
  ROM_SysTickPeriodSet (ROM_SysCtlClockGet () / SYSTICKS_PER_SECOND);
  ROM_SysTickIntEnable ();
  ROM_SysTickEnable ();
  //
  // Initialize the transmit and receive buffers.
  //
  USBBufferInit ((tUSBBuffer *) &g_sTxBuffer);
  USBBufferInit ((tUSBBuffer *) &g_sRxBuffer);
  //
  // Set the USB stack mode to Device mode with VBUS monitoring.
  //
  USBStackModeSet (0, USB_MODE_DEVICE, 0);
  //
  // Pass our device information to the USB library and place the device
  // on the bus.
  //
  USBDCDCInit (0, (tUSBDCDCDevice *) &g_sCDCDevice);
  //
}


//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static void
SetControlLineState (unsigned short usState);
static tBoolean
SetLineCoding (tLineCoding *psLineCoding);
static void
GetLineCoding (tLineCoding *psLineCoding);
static void
SendBreak (tBoolean bSend);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
  {
    while(1)
      {
      }
  }
#endif
//*****************************************************************************
//
// Set the state of the RS232 RTS and DTR signals.
//
//*****************************************************************************
static void
SetControlLineState (unsigned short usState)
{
  if (usState & USB_CDC_ACTIVATE_CARRIER)
    {
      Fsm::publish (SIG_USB_CONNECTED);
    }
  else
    Fsm::publish (SIG_USB_DISCONNECTED);
}

//*****************************************************************************
//
// Set the communication parameters to use on the UART.
//
//*****************************************************************************
static tBoolean
SetLineCoding (tLineCoding *psLineCoding)
{
//  Sequence::publish (Usb::CONNECTED);
  return true;
}

//*****************************************************************************
//
// Get the communication parameters in use on the UART.
//
//*****************************************************************************
static void
GetLineCoding (tLineCoding *psLineCoding)
{
  psLineCoding->ucDatabits = 8;
  psLineCoding->ucParity = USB_CDC_PARITY_NONE;
  psLineCoding->ucStop = USB_CDC_STOP_BITS_1;
  psLineCoding->ulRate = 115000;
}

//*****************************************************************************
//
// This function sets or clears a break condition on the redirected UART RX
// line.  A break is started when the function is called with \e bSend set to
// \b true and persists until the function is called again with \e bSend set
// to \b false.
//
//*****************************************************************************
static void
SendBreak (tBoolean bSend)
{
}

//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long
ControlHandler (void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
		void *pvMsgData)
{
  //
  // Which event are we being asked to process?
  //
  switch (ulEvent)
    {
    //
    // We are connected to a host and communication is now possible.
    //
    case USB_EVENT_CONNECTED:
      g_bUSBConfigured = true;

      //
      // Flush our buffers.
      //
      USBBufferFlush (&g_sTxBuffer);
      USBBufferFlush (&g_sRxBuffer);
//		Sequence::publish(Usb::CONNECTED);
//		usb.reset();
      break;

      //
      // The host has disconnected.
      //
    case USB_EVENT_DISCONNECTED:
      g_bUSBConfigured = false;
//		Sequence::publish(Usb::DISCONNECTED);
      break;

      //
      // Return the current serial communication parameters.
      //
    case USBD_CDC_EVENT_GET_LINE_CODING:
      GetLineCoding ((tLineCoding*) pvMsgData);
      break;

      //
      // Set the current serial communication parameters.
      //
    case USBD_CDC_EVENT_SET_LINE_CODING:
      SetLineCoding ((tLineCoding*) pvMsgData);
      break;

      //
      // Set the current serial communication parameters.
      //
    case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
      SetControlLineState ((unsigned short) ulMsgValue);
      break;

      //
      // Send a break condition on the serial line.
      //
    case USBD_CDC_EVENT_SEND_BREAK:
      SendBreak (true);
      break;

      //
      // Clear the break condition on the serial line.
      //
    case USBD_CDC_EVENT_CLEAR_BREAK:
      SendBreak (false);
      break;

      //
      // Ignore SUSPEND and RESUME for now.
      //
    case USB_EVENT_SUSPEND:
    case USB_EVENT_RESUME:
      break;

      //
      // We don't expect to receive any other events.  Ignore any that show
      // up in a release build or hang in a debug build.
      //
    default:
#ifdef DEBUG
      while(1);
#else
      break;
#endif

    }

  return (0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long
TxHandler (void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
	   void *pvMsgData)
{
  //
  // Which event have we been sent?
  //
  switch (ulEvent)
    {
    case USB_EVENT_TX_COMPLETE:
      //
      // Since we are using the USBBuffer, we don't need to do anything
      // here.
      //
      break;
      //
      // We don't expect to receive any other events.  Ignore any that show
      // up in a release build or hang in a debug build.
      //
    default:
#ifdef DEBUG
      while(1);
#else
      break;
#endif

    }
  return (0);
}

//*****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
extern "C" unsigned long
RxHandler (void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
	   void *pvMsgData)
{

  // Which event are we being sent?
  //
  switch (ulEvent)
    {
    //
    // A new packet has been received.
    //
    case USB_EVENT_RX_AVAILABLE:
      {
	/*		uint8_t b;
	 while (USBBufferRead((tUSBBuffer *) &g_sRxBuffer, &b, 1))
	 usb.in.write(b);*/
	Fsm::publish (SIG_USB_RXD);
	break;
      }

      //
      // We are being asked how much unprocessed data we have still to
      // process. We return 0 if the UART is currently idle or 1 if it is
      // in the process of transmitting something. The actual number of
      // bytes in the UART FIFO is not important here, merely whether or
      // not everything previously sent to us has been transmitted.
      //
    case USB_EVENT_DATA_REMAINING:
      {
	//
	// Get the number of bytes in the buffer and add 1 if some data
	// still has to clear the transmitter.
	//
	return (0);
      }

      //
      // We are being asked to provide a buffer into which the next packet
      // can be read. We do not support this mode of receiving data so let
      // the driver know by returning 0. The CDC driver should not be sending
      // this message but this is included just for illustration and
      // completeness.
      //
    case USB_EVENT_REQUEST_BUFFER:
      {
	return (0);
      }

      //
      // We don't expect to receive any other events.  Ignore any that show
      // up in a release build or hang in a debug build.
      //
    default:
#ifdef DEBUG
      while(1);
#else
      break;
#endif
    }

  return (0);
}

class EventLogger : public Sequence
{
public:
  EventLogger ()
  {

  }
  int
  handler (Event* event)
  {
    if (event->id () != Timer::TICK)
      {
	Log::log () << " EVENT : ";
	event->toString (Log::log ());
	Log::log ().flush ();
      }
    return 0;
  }

};


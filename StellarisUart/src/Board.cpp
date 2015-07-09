/*
 * Board.cpp
 *
 *  Created on: 9-dec.-2012
 *      Author: lieven2
 *      Board : LM4F120XL
 */

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
//#include "inc/lm4f112h5qc.h"
#include "inc/hw_types.h"
#include "inc/lm4f120h5qr.h"
#include "inc/hw_gpio.h"

#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/systick.h"
#include "driverlib/adc.h"
#include "Board.h"
#include <stdio.h>
#include <string.h>

char *Board::processor() {
	return (char *) "LM4F120H5QR";
}

void Board::processorRevision(Bytes& b) {
	union {
		uint32_t i32;
		uint8_t b[4];
	} v;
	v.i32 = SYSCTL_DID0_R;
	for (int i = 3; i >= 0; i--)
		b.write(v.b[i]);
	v.i32 = SYSCTL_DID1_R;
	for (int i = 3; i >= 0; i--)
		b.write(v.b[i]);
}

void Board::disableInterrupts() {
	IntMasterDisable();
}
void Board::enableInterrupts() {
	IntMasterEnable();
}

//*****************************************************************************
//
// Defines for the hardware resources used by the pushbuttons.
//
// The switches are on the following ports/pins:
//
// PF4 - Left Button
// PF0 - Right Button
//
// The switches tie the GPIO to ground, so the GPIOs need to be configured
// with pull-ups, and a value of 0 means the switch is pressed.
//
//*****************************************************************************
#define BUTTONS_GPIO_PERIPH     SYSCTL_PERIPH_GPIOF
#define BUTTONS_GPIO_BASE       GPIO_PORTF_BASE

#define NUM_BUTTONS             2
#define LEFT_BUTTON             GPIO_PIN_4
#define RIGHT_BUTTON            GPIO_PIN_0

#define ALL_BUTTONS             (LEFT_BUTTON | RIGHT_BUTTON)

//*****************************************************************************
//
// Useful macros for detecting button events.
//
//*****************************************************************************
#define BUTTON_PRESSED(button, buttons, changed)                              \
        (((button) & (changed)) && ((button) & (buttons)))

#define BUTTON_RELEASED(button, buttons, changed)                             \
        (((button) & (changed)) && !((button) & (buttons)))

//*****************************************************************************
//
// Holds the current, debounced state of each button.  A 0 in a bit indicates
// that that button is currently pressed, otherwise it is released.
// We assume that we start with all the buttons released (though if one is
// pressed when the application starts, this will be detected).
//
//*****************************************************************************
static unsigned char g_ucButtonStates = ALL_BUTTONS;
//*****************************************************************************
//
//! Initializes the GPIO pins used by the board pushbuttons.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the pushbuttons are attached.  It enables
//! the port used by the buttons and configures each button GPIO as an input
//! with a weak pull-up.
//!
//! \return None.
//
//*****************************************************************************
void ButtonsInit(void) {
	//
	// Enable the GPIO port to which the pushbuttons are connected.
	//
	SysCtlPeripheralEnable(BUTTONS_GPIO_PERIPH);
	//
	// Unlock PF0 so we can change it to a GPIO input
	// Once we have enabled (unlocked) the commit register then re-lock it
	// to prevent further changes.  PF0 is muxed with NMI thus a special case.
	//
	HWREG(BUTTONS_GPIO_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
	HWREG(BUTTONS_GPIO_BASE + GPIO_O_CR) |= 0x01;
	HWREG(BUTTONS_GPIO_BASE + GPIO_O_LOCK) = 0;
	//
	// Set each of the button GPIO pins as an input with a pull-up.
	//
	GPIODirModeSet(BUTTONS_GPIO_BASE, ALL_BUTTONS, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(BUTTONS_GPIO_BASE, ALL_BUTTONS, GPIO_STRENGTH_2MA,
	GPIO_PIN_TYPE_STD_WPU);
	//
	// Initialize the debounced button state with the current state read from
	// the GPIO bank.
	//
	g_ucButtonStates = GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);
}

//*****************************************************************************
//
//! Polls the current state of the buttons and determines which have changed.
//!
//! \param pucDelta points to a character that will be written to indicate
//! which button states changed since the last time this function was called.
//! This value is derived from the debounced state of the buttons.
//! \param pucRawState points to a location where the raw button state will
//! be stored.
//!
//! This function should be called periodically by the application to poll the
//! pushbuttons.  It determines both the current debounced state of the buttons
//! and also which buttons have changed state since the last time the function
//! was called.
//!
//! In order for button debouncing to work properly, this function should be
//! caled at a regular interval, even if the state of the buttons is not needed
//! that often.
//!
//! If button debouncing is not required, the the caller can pass a pointer
//! for the \e pucRawState parameter in order to get the raw state of the
//! buttons.  The value returned in \e pucRawState will be a bit mask where
//! a 1 indicates the buttons is pressed.
//!
//! \return Returns the current debounced state of the buttons where a 1 in the
//! button ID's position indicates that the button is pressed and a 0
//! indicates that it is released.
//
//*****************************************************************************
unsigned char ButtonsPoll(unsigned char *pucDelta, unsigned char *pucRawState) {
	unsigned long ulDelta;
	unsigned long ulData;
	static unsigned char ucSwitchClockA = 0;
	static unsigned char ucSwitchClockB = 0;

	//
	// Read the raw state of the push buttons.  Save the raw state
	// (inverting the bit sense) if the caller supplied storage for the
	// raw value.
	//
	ulData = (GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS));
	if (pucRawState) {
		*pucRawState = (unsigned char) ~ulData;
	}

	//
	// Determine the switches that are at a different state than the debounced
	// state.
	//
	ulDelta = ulData ^ g_ucButtonStates;

	//
	// Increment the clocks by one.
	//
	ucSwitchClockA ^= ucSwitchClockB;
	ucSwitchClockB = ~ucSwitchClockB;

	//
	// Reset the clocks corresponding to switches that have not changed state.
	//
	ucSwitchClockA &= ulDelta;
	ucSwitchClockB &= ulDelta;

	//
	// Get the new debounced switch state.
	//
	g_ucButtonStates &= ucSwitchClockA | ucSwitchClockB;
	g_ucButtonStates |= (~(ucSwitchClockA | ucSwitchClockB)) & ulData;

	//
	// Determine the switches that just changed debounced state.
	//
	ulDelta ^= (ucSwitchClockA | ucSwitchClockB);

	//
	// Store the bit mask for the buttons that have changed for return to
	// caller.
	//
	if (pucDelta) {
		*pucDelta = (unsigned char) ulDelta;
	}

	//
	// Return the debounced buttons states to the caller.  Invert the bit
	// sense so that a '1' indicates the button is pressed, which is a
	// sensible way to interpret the return value.
	//
	return (~g_ucButtonStates);
}

bool Board::isButtonPressed(Button button) {
	return false;
}

float fTempValueC;
uint32_t ui32TempAvg;

extern "C" void ADC0IntHandler(void) // NOT started yet !!!!
		{
	uint32_t ui32ADC0Value[4]; //used for storing data from ADC FIFO, must be as large as the FIFO for sequencer in use. Sequencer 1 has FIFO depth of 4
	//variables that cannot be optimized out by compiler

	ADCIntClear(ADC0_BASE, 1); //clear interrupt flag
	ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2]
			+ ui32ADC0Value[3] + 2) / 4; //calculate average
	ADCProcessorTrigger(ADC0_BASE, 1);
}

void AdcInit() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_125KSPS);
	ADCSequenceDisable(ADC0_BASE, 1);
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 3,
	ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
#define ADC_ON
#ifdef ADC_ON
	ADCIntEnable(ADC0_BASE, 1);
	IntEnable(INT_ADC0SS1); //DISABLED
#else
	ADCIntDisable(ADC0_BASE, 1);
	IntDisable(INT_ADC0SS1);
#endif
	ADCSequenceEnable(ADC0_BASE, 1);
	ADCProcessorTrigger(ADC0_BASE, 1);
}

void Board::init() // initialize the board specifics
{
	//
	// Enable lazy stacking for interrupt handlers.  This allows floating-point
	// instructions to be used within interrupt handlers, but at the expense of
	// extra stack usage.
	//
	FPUEnable();
	FPULazyStackingEnable();
	//
	// Set the clocking to run from the PLL at 50MHz
	//
	SysCtlClockSet(
	SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlDelay(16);	// can help to avoid FaultIsr I have read somewhere

	IntMasterEnable(); // Enable interrupts to the processor.
	// Set up the period for the SysTick timer for 1 mS.
	SysTickPeriodSet(SysCtlClockGet() / 1000);
	SysTickIntEnable(); // Enable the SysTick Interrupt.
	SysTickEnable(); // Enable SysTick.

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Enable the GPIO port that is used for the on-board LED.
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); // Enable the GPIO pins for the LED (PF2).
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

	Board::setLedOn(Board::LED_GREEN, false);
	Board::setLedOn(Board::LED_RED, false);
	Board::setLedOn(Board::LED_BLUE, false);
	AdcInit();
}

float Board::getTemp() {
	/*	float ulTempValueC;
	 unsigned long ulADC0Value[4], ulTempAvg;
	 //	ADCIntClear(ADC0_BASE, 1);
	 //	ADCIntEnable(ADC0_BASE, 1);
	 ADCProcessorTrigger(ADC0_BASE, 1);
	 while (!ADCIntStatus(ADC0_BASE, 1, false)) {
	 }
	 SysCtlDelay(100000);
	 ADCSequenceDataGet(ADC0_BASE, 1, ulADC0Value);
	 if (ulADC0Value[0] == 0)
	 ADCInit();
	 ulTempAvg = (ulADC0Value[0] + ulADC0Value[1] + ulADC0Value[2]
	 + ulADC0Value[3] + 2) / 4;
	 ulTempValueC = (1475.0 - ((2475.0 * ulTempAvg)) / 4096) / 10;*/
	if (ui32TempAvg == 0)
		AdcInit();
	fTempValueC = (1475.0 - ((2475.0 * ui32TempAvg)) / 4096) / 10; //TEMP = 147.5 – ((75 * (VREFP – VREFN) * ADCVALUE) / 4096) multiply by 10 to keep precision and then div by 10 at end
	return fTempValueC;
}

//*****************************************************************************
// The UART interrupt handler.
//*****************************************************************************

void Board::setLedOn(int32_t led, bool on) // light one of the Led's
		{
	if (led == LED_BLUE) {
		if (on)
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
		else
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
	} else if (led == LED_GREEN) {
		if (on)
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
		else
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
	} else if (led == LED_RED) {
		if (on)
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
		else
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
	}
}

bool Board::getLed(int32_t led) {
	int32_t value;
	if (led == LED_RED) {
		value = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1);
		if (value)
			return true;
	}
	if (led == LED_BLUE) {
		value = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2);
		if (value)
			return true;
	}
	if (led == LED_GREEN) {
		value = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3);
		if (value)
			return true;
	}
	return false;
}

bool Board::getButton(Button button) {
	return false;
}


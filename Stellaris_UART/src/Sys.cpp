/*
 * Sys.cpp

 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Sys.h"
#include "Property.h"
#include "Str.h"
#define PART_LM4F120H5QR
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include <inc/lm4f120h5qr.h>
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#define __disable_irq()
#define __enable_irq()

Sys::Sys() {
}

Str Sys::_log(100);
void Sys::log(const char *s) {
	_log.clear();
	_log.append(_upTime);
	_log.append(" : ");
	_log.append(s);
	Property::updated(&_log);
}

#include "PinMux.h"
void Sys::init() {

	SysCtlClockSet(
			SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_16MHZ);
	PortFunctionInit();

}
uint64_t Sys::_upTime = 0;
uint64_t Sys::_bootTime = 0L;

#include "Timer.h"

// ISR clock updates _upTime every 1 msec
/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
extern "C" void SysTick_Handler(void) {
	Sys::_upTime++;
	Timer::decAll();
}

void Sys::delay_ms(uint32_t msec) {
	uint64_t end = _upTime + msec;
	while (_upTime < end)
		;
}

uint64_t Sys::upTime() {
	return _upTime;
}

#include <stdlib.h>
#include <assert.h>

#define HEAP_SIZE 500
uint32_t myHeap[HEAP_SIZE];
uint32_t myHeapOffset = 0;

void * Sys::malloc(uint32_t size) {
	uint32_t i32Size = size / 4;
	void* ptr;
	ASSERT((myHeapOffset + i32Size) < HEAP_SIZE);
	ptr = (void*) &myHeap[myHeapOffset];
	myHeapOffset += i32Size;
	return ptr;
}

void Sys::free(void *pv) {
	ASSERT(false);
}

void Sys::interruptDisable() {
	__disable_irq();
}

void Sys::interruptEnable() {
	__enable_irq();
}


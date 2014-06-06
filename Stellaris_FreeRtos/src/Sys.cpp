/*
 * Sys.cpp

 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Sys.h"
#include "Str.h"
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
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include <inc/hw_sysctl.h>
#include <driverlib/systick.h>

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
//	Property::updated(&_log);
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
void SysTickHandler(void) {
	Sys::_upTime++;
	Timer::decAll();
}

extern "C" void PortFunctionInit();
void Sys::init() {

	MAP_SysCtlClockSet(
			SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_16MHZ);
	MAP_SysTickPeriodSet(SysCtlClockGet() / 1000);
	MAP_SysTickEnable();
	MAP_SysTickIntEnable();
	MAP_IntMasterEnable();

	PortFunctionInit();

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

#define FREERTOS

#ifndef FREERTOS

#define HEAP_SIZE 5000
uint32_t myHeap[HEAP_SIZE];
uint32_t myHeapOffset = 0;
#define BLOCK_MAX	50
struct {
	bool free;
	uint32_t size;
	void *start;
} block[BLOCK_MAX];
uint32_t blockCount = 0;
#endif

void * Sys::malloc(uint32_t size) {
#ifdef FREERTOS
	return pvPortMalloc(size);
#else
	int i;
	for (i = 0; i < blockCount; i++) { // find free of same size
		if (block[i].free)
		if (block[i].size == size) {
			block[i].free = false;
			return (void*) block[i].start;
		}
	}
	uint32_t i32Size = ( size+3) / 4;
	void* ptr;
	ASSERT((myHeapOffset + i32Size) < HEAP_SIZE);
	ptr = (void*) &myHeap[myHeapOffset];
	myHeapOffset += i32Size;
	if (blockCount < BLOCK_MAX) {
		int i = blockCount++;
		block[i].size = size;
		block[i].free = false;
		block[i].start = ptr;
	} else while(1);
	return ptr;
#endif
}

void Sys::free(void *pv) {
#ifdef FREERTOS
	vPortFree(pv);
#else
	int i;
	for (i = 0; i < blockCount; i++) {
		if (block[i].free==false)
		if (block[i].start == pv) {
			block[i].free = true;
			return;
		}
	}
	while(1);
	ASSERT(false); // trying to free something never allocated
#endif
}

void Sys::interruptDisable() {
	__disable_irq();
}

void Sys::interruptEnable() {
	__enable_irq();
}


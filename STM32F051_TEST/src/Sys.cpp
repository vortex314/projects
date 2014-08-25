/*
 * Sys.cpp

 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Sys.h"
#include "stm320518_eval.h"
#include "stm32f0xx.h"
#include "Str.h"

Sys::Sys() {
}


void Sys::init() {
	WWDG_DeInit();

	SysTick_Config(SystemCoreClock / 1000);
//	SysTick_Config(8000);
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

}



uint64_t Sys::upTime(){
	return _upTime;
}
/*
#include <stdlib.h>
#include <assert.h>
*/
/* #define HEAP_SIZE 500
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
*/

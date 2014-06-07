/*
 * Sys.cpp

 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */
#include "Sys.h"
#include "stm32f10x_conf.h"
#include "Property.h"
#include "Str.h"

Sys::Sys() {
}

Str Sys::_log(100);
void Sys::log(const char *s){
    _log.clear();
    _log.append(_upTime);
    _log.append(" : ");
    _log.append(s);
    Property::updated(&_log);
}


void Sys::init() {
	WWDG_DeInit();
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Configure all unused GPIO port pins in Analog Input mode (floating input
	 trigger OFF), this will reduce the power consumption and increase the device
	 immunity against EMI/EMC *************************************************/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
	RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
	RCC_APB2Periph_GPIOE, DISABLE);
	SysTick_Config(SystemCoreClock / 1000);
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

uint64_t Sys::upTime(){
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


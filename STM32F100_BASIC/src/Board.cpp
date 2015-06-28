/*
 * Board.cpp
 *
 *  Created on: 9-dec.-2012
 *      Author: lieven2
 *      Board : LM4F120XL
 */

#include "Board.h"
#include <stdint.h>
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stm32vl_discovery.h>

char *Board::processor() {
	return (char *) "STM32F100RB";
}

void Board::processorRevision(Bytes& b) {

}

void Board::disableInterrupts() {

}
void Board::enableInterrupts() {

}

void ButtonsInit(void) {

}

bool Board::isButtonPressed(Button button) {
	return false;
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 24000000
 *            HCLK(Hz)                       = 24000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            HSE Frequency(Hz)              = 8000000
 *            HSE PREDIV1                    = 2
 *            PLLMUL                         = 6
 *            Flash Latency(WS)              = 0
 * @param  None
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_ClkInitTypeDef clkinitstruct = { 0 };
	RCC_OscInitTypeDef oscinitstruct = { 0 };

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	oscinitstruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	oscinitstruct.HSEState = RCC_HSE_ON;
	oscinitstruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
	oscinitstruct.PLL.PLLState = RCC_PLL_ON;
	oscinitstruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	oscinitstruct.PLL.PLLMUL = RCC_PLL_MUL6;
	if (HAL_RCC_OscConfig(&oscinitstruct) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
	clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_0) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}
}

void _exit() {

}

void Board::init() // initialize the board specifics
{

	/* STM32F1xx HAL library initialization:
	 - Configure the Flash prefetch
	 - Systick timer is configured by default as source of time base, but user
	 can eventually implement his proper time base source (a general purpose
	 timer for example or other time source), keeping in mind that Time base
	 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	 handled in milliseconds basis.
	 - Set NVIC Group Priority to 4
	 - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the system clock to 24 MHz */
	SystemClock_Config();
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);

	Board::setLedOn(Board::LED_GREEN, false);
	Board::setLedOn(Board::LED_RED, false);
	Board::setLedOn(Board::LED_BLUE, false);
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

float Board::getTemp() {
	float fTemp = 37.5;
	return fTemp;
}

//*****************************************************************************
// The UART interrupt handler.
//*****************************************************************************

void Board::setLedOn(int32_t led, bool on) // light one of the Led's
		{
	if (led == LED_BLUE) {
		if (on)
			BSP_LED_On(LED4);
		else
			BSP_LED_Off(LED4);
	} else if (led == LED_GREEN) {
		if (on)
			BSP_LED_On(LED3);
		else
			BSP_LED_Off(LED3);
	} /*else if (led == LED_RED) {
	 if (on)
	 BSP_LED_On(LED_RED);
	 else
	 BSP_LED_Off(LED_RED);
	 }*/
}

bool Board::getLed(int32_t led) {
	/*	int32_t value;
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
	 }*/
	return false;
}

bool Board::getButton(Button button) {
	return false;
}


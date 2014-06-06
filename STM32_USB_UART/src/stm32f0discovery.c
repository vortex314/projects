/**
 *
 *

 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f0discovery.h"
#include "stm32_eval_legacy.h"
#include "led.h"

// 2 color led on PA9
#define LED_Pin	GPIO_Pin_9
void LedOn() {
	// GPIOA clock was already set
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = LED_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void LedOff() {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = LED_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void LedRed() {
	LedOn();
	GPIO_SetBits(GPIOA, LED_Pin );
}

void LedGreen() {
	LedOn();
	GPIO_ResetBits(GPIOA, LED_Pin );
}
/**
 * @brief  Configures COM port.
 * @param
 * @param  USART_InitStruct: pointer to a USART_InitTypeDef structure that
 *   contains the configuration information for the specified USART peripheral.
 * @retval None
 */
void STM_EVAL_COMInit(USART_InitTypeDef* USART_InitStruct) {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // enable GPIOA clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);	// enable AFIO clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);// enable USART2 clock

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; /* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; /* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_Init(USART2, USART_InitStruct);		// USART configuration
	USART_Cmd(USART2, ENABLE);					// Enable USART
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

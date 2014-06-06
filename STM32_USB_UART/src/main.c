/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @version V3.4.0
 * @date    29-June-2012
 * @brief   Virtual Com Port Demo main file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#define MAX_CLOCKS	5

int32_t clock[MAX_CLOCKS];
void (*clockFunc[MAX_CLOCKS])(void);

void setClockFunc(int32_t idx, void (*f)(void)) {
	if (idx < MAX_CLOCKS)
		clockFunc[idx] = f;
}

void resetClock(int32_t idx, int32_t value) {
	if (idx < MAX_CLOCKS)
		clock[idx] = value;
}

void MySysTickHandler(void) {
	int32_t i;
	for (i = 0; i < MAX_CLOCKS; i++) {
		if (clock[i]) {
			if (--clock[i] == 0)
				if (clockFunc[i] != NULL )
					clockFunc[i]();
		}
	}
}

/*******************************************************************************
 * set External clock to MCO this to feed the STM32F0
 *******************************************************************************/

void Set_MCO() {
	GPIO_InitTypeDef GPIO_InitStructure;
	// Put the clock configuration into RCC_APB2PeriphClockCmd
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* Output clock on MCO pin ---------------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_MCOConfig(RCC_MCO_HSE ); // Put on MCO pin the: freq. of external crystal
}
/******************************************************************************
 * PA11,PA12 are D-,D+
 * disconnect them
 *******************************************************************************/

static void DelayUs(uint32_t delay_us) {
	uint32_t nb_loop;

	nb_loop = (((24000000 / 1000000) / 4) * delay_us) + 1; /* uS (divide by 4 because each loop take about 4 cycles including nop +1 is here to avoid delay of 0 */
	asm volatile(
			"1: " "\n\t"
			" nop " "\n\t"
			" subs.w %0, %0, #1 " "\n\t"
			" bne 1b " "\n\t"
			: "=r" (nb_loop)
			: "0"(nb_loop)
			: "r3"
	);
}


void USB_Disconnect() {
	GPIO_InitTypeDef GPIO_InitStructure;
	// Put the clock configuration into RCC_APB2PeriphClockCmd
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* Output clock on MCO pin ---------------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DelayUs(1000000);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;		// USB D-
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DelayUs(1000000);
	GPIO_SetBits(GPIOA, GPIO_Pin_12 );
	GPIO_SetBits(GPIOA, GPIO_Pin_11 );
	DelayUs(1000000);
	GPIO_ResetBits(GPIOA, GPIO_Pin_12 );
	GPIO_ResetBits(GPIOA, GPIO_Pin_11 );
	DelayUs(1000000);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DelayUs(100000);
}
/*******************************************************************************
 * Function Name  : main.
 * Description    : Main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void) {
	Set_System();
	Set_MCO();
	USB_Disconnect();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();

	while (1) {
	}
}
#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

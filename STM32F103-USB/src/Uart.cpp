/*
 * Uart.cpp
 *
 *  Created on: 20-aug.-2015
 *      Author: lieven2
 */

#include "Uart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"

const struct {
	USART_TypeDef* base;
	GPIO_TypeDef* port;
	uint16_t txPin;
	uint16_t rxPin;
} portsUart[] = { { USART1,GPIOA,GPIO_Pin_9,GPIO_Pin_10 }, //
		{ USART2,GPIOA,GPIO_Pin_2,GPIO_Pin_3 } //

		};
Uart::Uart(uint32_t idx) :
		_idx(idx), _in(100), _out(256), _inBuffer(256) {
	_overrunErrors = 0;
	_crcErrors = 0;
}
Uart::~Uart() {
	// TODO Auto-generated destructor stub
}

void Uart::init() {

	/* USART configuration structure for USART1 */
	USART_InitTypeDef usart1_init_struct;
	/* Bit configuration structure for GPIOA PIN9 and PIN10 */
	GPIO_InitTypeDef gpioa_init_struct;

	/* Enalbe clock for USART1, AFIO and GPIOA */
	RCC_APB2PeriphClockCmd(
	RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

	/* GPIOA PIN9 alternative function Tx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpioa_init_struct);
	/* GPIOA PIN9 alternative function Rx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_10;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpioa_init_struct);

	/* Enable USART1 */
	USART_Cmd(USART1, ENABLE);
	/* Baud rate 9600, 8-bit data, One stop bit
	 * No parity, Do both Rx and Tx, No HW flow control
	 */
	usart1_init_struct.USART_BaudRate = 9600;
	usart1_init_struct.USART_WordLength = USART_WordLength_8b;
	usart1_init_struct.USART_StopBits = USART_StopBits_1;
	usart1_init_struct.USART_Parity = USART_Parity_No;
	usart1_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart1_init_struct.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;
	/* Configure USART1 */
	USART_Init(USART1, &usart1_init_struct);
	/* Enable RXNE interrupt */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	/* Enable USART1 global interrupt */
	NVIC_EnableIRQ(USART1_IRQn);
}

uint8_t Uart::read() {
	return _in.read();
}
uint32_t Uart::hasData() {
	return _in.hasData();
}

Erc Uart::send(Bytes& bytes) {
	bytes.offset(0);
	if (_out.space() < bytes.length()) { // not enough space in circbuf
		_overrunErrors++;
		Sys::warn(EOVERFLOW, "UART_SEND");
		return E_AGAIN;
	}
	while (_out.hasSpace() && bytes.hasData()) {
		_out.write(bytes.read());
	}
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	return E_OK;
}

extern Uart uart1;

void USART1_IRQHandler(void) {
	/* RXNE handler */
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		uart1._in.write((uint8_t) USART_ReceiveData(USART1));
	}
	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		if (uart1._out.hasData()) {
			USART_SendData(USART1, uart1._out.read());
		} else
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	}
}

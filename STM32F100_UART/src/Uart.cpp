/*
 * Uart.cpp
 *
 *  Created on: 9-okt.-2013
 *      Author: lieven2
 */

#include <Uart.h>
#include <stm32f10x_usart.h>
#include "Led.h"
extern Led ledGreen;

Uart::Uart() :
		_rxd(256), _txd(256), _timer(this) {
	_timer.interval(5);
	_timer.reload(true);
	_commErrors=0;
	_bytesRxd=0;
	_bytesTxd=0;
}

Uart::~Uart() {
	ASSERT(false);
}

Erc Uart::write(Bytes& bytes) {
	bytes.offset(0);
	while (_txd.hasSpace() && bytes.hasData()) {
		_txd.write(bytes.read());
	};
	if (_txd.hasData()) {
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	}
	return E_OK;
}

uint8_t Uart::read() {
	return _rxd.read();
}

bool Uart::hasData() {
	return _rxd.hasData();
}

void Uart::poll() { // EventQueue is not yet multithread and ISR safe
	if (_rxd.hasData())
		upQueue(Uart::RXD);
}

Uart uart1;

Erc Uart::event(Event& event) {
	if (event.src() == &_timer)
		poll();
	return E_OK;
}

//__________________________________________________________________ HARDWARE SPECIFIC

void Uart::init() {
	/***********************************************
	 * Initialize GPIOA PIN8 as push-pull output
	 ***********************************************/
	// USART1.TXD = PA9 = P1 pin 7
	// USART1.RXD = PA10 = P1 Pin 8
	GPIO_InitTypeDef gpioa_init_struct;
	USART_InitTypeDef usart_init_struct;
	/* Bit configuration structure for GPIOA PIN9 and PIN10 */

	/* Enable clock for USART1, AFIO and GPIOA */
	RCC_APB2PeriphClockCmd(
	RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

	/* GPIOA PIN9 alternative function Tx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_9;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpioa_init_struct);

	/* GPIOA PIN10 alternative function Rx */
	gpioa_init_struct.GPIO_Pin = GPIO_Pin_10;
	gpioa_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	gpioa_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpioa_init_struct);
	USART_Cmd(USART1, ENABLE);
	/* Baud rate 115200, 8-bit data, One stop bit
	 * No parity, Do both Rx and Tx, No HW flow control
	 */
	usart_init_struct.USART_BaudRate = 19200;
	usart_init_struct.USART_WordLength = USART_WordLength_8b;
	usart_init_struct.USART_StopBits = USART_StopBits_1;
	usart_init_struct.USART_Parity = USART_Parity_No;
	usart_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart_init_struct.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;

	USART_Init(USART1, &usart_init_struct);
//	USART_HalfDuplexCmd(USART1, ENABLE); // loopback

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); /* Enable RXNE and TXE interrupt */
//	USART_ITConfig(USART1, USART_IT_TXE, ENABLE); // don't enable wait to send something
	NVIC_EnableIRQ(USART1_IRQn); /* Enable USART1 global interrupt */
	_timer.start();
}

/**********************************************************
 * USART1 interrupt request handler:
 *********************************************************/
extern "C" void USART1_IRQHandler(void) {
	/* RXNE handler */
	if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {
		uart1._rxd.write(USART_ReceiveData(USART1));
		uart1._bytesRxd++;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
	if (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == SET) {
		if (uart1._txd.hasData()) {
			USART_SendData(USART1, uart1._txd.read());
			uart1._bytesTxd++;
		} else {
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
//			USART_ClearITPendingBit(USART1, USART_IT_TXE);
		}
	}
	USART_ReceiveData(USART1);
	USART_ClearITPendingBit(USART1, USART_IT_TC);
//	USART_ClearITPendingBit(USART1,
//			USART_IT_CTS | USART_IT_LBD | USART_IT_TC  | USART_IT_LBD );
//	(((IT) == USART_IT_TC) || ((IT) == USART_IT_RXNE) ||  ((IT) == USART_IT_LBD) || ((IT) == USART_IT_CTS))
	//	USART_ClearITPendingBit(USART1,0);
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE)
			| USART_GetFlagStatus(USART1, USART_FLAG_NE)
			| USART_GetFlagStatus(USART1, USART_FLAG_FE)
			| USART_GetFlagStatus(USART1, USART_FLAG_PE)) {
		USART_ReceiveData(USART1); // only way to clear flag
		uart1._commErrors++;
	}
}


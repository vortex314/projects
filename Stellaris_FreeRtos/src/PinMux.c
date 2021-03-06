//*****************************************************************************
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file was automatically generated by the Stellaris PinMux Utility
// Version: 1.0.1
//
//*****************************************************************************

#include "PinMux.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"

//*****************************************************************************
void
PortFunctionInit(void)
{
    //
    // Enable Peripheral Clocks 
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable port PF3 for GPIOOutput
    //
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Enable port PF1 for GPIOOutput
    //
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

    //
    // Enable port PF4 for GPIOInput
    //
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);

    //
    // Enable port PF2 for GPIOOutput
    //
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    //
    // Enable port PF0 for GPIOInput
    //

    //
    //First open the lock and select the bits we want to modify in the GPIO commit register.
    //
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0x1;

    //
    //Now modify the configuration of the pins that we unlocked.
    //
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);

    //
    // Enable port PB2 for I2C0 I2C0SCL
    //
    MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);

    //
    // Enable port PB3 for I2C0 I2C0SDA
    //
    MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    //
    // Enable port PA6 for I2C1 I2C1SCL
    //
    MAP_GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);

    //
    // Enable port PA7 for I2C1 I2C1SDA
    //
    MAP_GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    MAP_GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    //
    // Enable port PE5 for I2C2 I2C2SDA
    //
    MAP_GPIOPinConfigure(GPIO_PE5_I2C2SDA);
    MAP_GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_5);

    //
    // Enable port PE4 for I2C2 I2C2SCL
    //
    MAP_GPIOPinConfigure(GPIO_PE4_I2C2SCL);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);

    //
    // Enable port PA5 for SSI0 SSI0TX
    //
    MAP_GPIOPinConfigure(GPIO_PA5_SSI0TX);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5);

    //
    // Enable port PA4 for SSI0 SSI0RX
    //
    MAP_GPIOPinConfigure(GPIO_PA4_SSI0RX);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_4);

    //
    // Enable port PA3 for SSI0 SSI0FSS
    //
    MAP_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_3);

    //
    // Enable port PA2 for SSI0 SSI0CLK
    //
    MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2);

    //
    // Enable port PD3 for SSI1 SSI1TX
    //
    MAP_GPIOPinConfigure(GPIO_PD3_SSI1TX);
    MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3);

    //
    // Enable port PD0 for SSI1 SSI1CLK
    //
    MAP_GPIOPinConfigure(GPIO_PD0_SSI1CLK);
    MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0);

    //
    // Enable port PD2 for SSI1 SSI1RX
    //
    MAP_GPIOPinConfigure(GPIO_PD2_SSI1RX);
    MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_2);

    //
    // Enable port PD1 for SSI1 SSI1FSS
    //
    MAP_GPIOPinConfigure(GPIO_PD1_SSI1FSS);
    MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_1);

    //
    // Enable port PB4 for SSI2 SSI2CLK
    //
    MAP_GPIOPinConfigure(GPIO_PB4_SSI2CLK);
    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4);

    //
    // Enable port PB7 for SSI2 SSI2TX
    //
    MAP_GPIOPinConfigure(GPIO_PB7_SSI2TX);
    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_7);

    //
    // Enable port PB6 for SSI2 SSI2RX
    //
    MAP_GPIOPinConfigure(GPIO_PB6_SSI2RX);
    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_6);

    //
    // Enable port PB5 for SSI2 SSI2FSS
    //
    MAP_GPIOPinConfigure(GPIO_PB5_SSI2FSS);
    MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5);

    //
    // Enable port PA0 for UART0 U0RX
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0);

    //
    // Enable port PA1 for UART0 U0TX
    //
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1);

    //
    // Enable port PB1 for UART1 U1TX
    //
    MAP_GPIOPinConfigure(GPIO_PB1_U1TX);
    MAP_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_1);

    //
    // Enable port PB0 for UART1 U1RX
    //
    MAP_GPIOPinConfigure(GPIO_PB0_U1RX);
    MAP_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0);

    //
    // Enable port PD7 for UART2 U2TX
    // First open the lock and select the bits we want to modify in the GPIO commit register.
    //
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0x80;

    //
    // Now modify the configuration of the pins that we unlocked.
    //
    MAP_GPIOPinConfigure(GPIO_PD7_U2TX);
    MAP_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_7);

    //
    // Enable port PD6 for UART2 U2RX
    //
    MAP_GPIOPinConfigure(GPIO_PD6_U2RX);
    MAP_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6);

    //
    // Enable port PC6 for UART3 U3RX
    //
    MAP_GPIOPinConfigure(GPIO_PC6_U3RX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6);

    //
    // Enable port PC7 for UART3 U3TX
    //
    MAP_GPIOPinConfigure(GPIO_PC7_U3TX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_7);

    //
    // Enable port PC5 for UART4 U4TX
    //
    MAP_GPIOPinConfigure(GPIO_PC5_U4TX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_5);

    //
    // Enable port PC4 for UART4 U4RX
    //
    MAP_GPIOPinConfigure(GPIO_PC4_U4RX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4);
}

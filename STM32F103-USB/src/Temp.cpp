/*
 * Temp.cpp
 *
 *  Created on: 13-jul.-2015
 *      Author: lieven2
 */

#include "stm32f10x.h"
#include <stm32f10x_adc.h>
#include <stm32f10x_rcc.h>

uint16_t AD_value;
const uint16_t V25 = 1750; // when V25=1.41V at ref 3.3V
const uint16_t Avg_Slope = 5; //when avg_slope=4.3mV/C at ref 3.3V
uint16_t TemperatureC;
int measure(void) {

//enable ADC1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN, ENABLE);
	ADC_InitTypeDef ADC_InitStructure;
//ADC1 configuration
//select independent conversion mode (single)
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
//We will convert single channel only
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
//we will convert one time
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
//select no external triggering
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
//right 12-bit data alignment in ADC data register
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
//single channel conversion
	ADC_InitStructure.ADC_NbrOfChannel = 1;
//load structure values to control and status registers
	ADC_Init(ADC1, &ADC_InitStructure);
//wake up temperature sensor
	ADC_TempSensorVrefintCmd(ENABLE);
//ADC1 channel16 configuration
//we select 41.5 cycles conversion for channel16
//and rank=1 which doesn't matter in single mode
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_41Cycles5);
//Enable ADC1
	ADC_Cmd(ADC1, ENABLE);
//Enable ADC1 reset calibration register
	ADC_ResetCalibration(ADC1);
//Check the end of ADC1 reset calibration register
	while (ADC_GetResetCalibrationStatus(ADC1))
		;
//Start ADC1 calibration
	ADC_StartCalibration(ADC1);
//Check the end of ADC1 calibration
	while (ADC_GetCalibrationStatus(ADC1))
		;
//Start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
//wait for conversion complete
	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {
	}
//read ADC value
	AD_value = ADC_GetConversionValue(ADC1);
//clear EOC flag
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
//	printf("\r\n ADC value: %d \r\n", AD_value);
	TemperatureC = (uint16_t) ((V25 - AD_value) / Avg_Slope + 25);
//	printf("Temperature: %d%cC\r\n", TemperatureC, 176);
	return TemperatureC;
}



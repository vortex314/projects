/*
 * board.h
 *
 *  Created on: 28-jun.-2015
 *      Author: lieven2
 */

#ifndef BOARD_H_
#define BOARD_H_
#include "core_cm3.h"
#define __CLZ                             __clz
typedef enum
{
  LED1 = 0,
  LED2 = 1,
  LED3 = 2,
  LED4 = 3,

  LED_GREEN  = LED1,
  LED_ORANGE = LED2,
  LED_RED    = LED3,
  LED_BLUE   = LED4

} Led_TypeDef;

typedef enum
{
  BUTTON_WAKEUP = 0,
  BUTTON_TAMPER = 1,
  BUTTON_KEY    = 2,
  BUTTON_SEL    = 3,
  BUTTON_LEFT   = 4,
  BUTTON_RIGHT  = 5,
  BUTTON_DOWN   = 6,
  BUTTON_UP     = 7,

} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1,
  BUTTON_MODE_EVT  = 2

} ButtonMode_TypeDef;

/**
 * @brief JOYSTICK Types Definition
 */
typedef enum
{
  JOY_SEL   = 0,
  JOY_LEFT  = 1,
  JOY_RIGHT = 2,
  JOY_DOWN  = 3,
  JOY_UP    = 4,
  JOY_NONE  = 5

}JOYState_TypeDef;

typedef enum
{
  JOY_MODE_GPIO = 0,
  JOY_MODE_EXTI = 1

}JOYMode_TypeDef;

typedef struct
{
  __IO uint16_t REG;
  __IO uint16_t RAM;

}LCD_CONTROLLER_TypeDef;



/**
  * @brief  This method returns the STM3210E EVAL BSP Driver revision
  * @retval version : 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t BSP_GetVersion(void);

/**
  * @brief  Configures LED GPIO.
  * @param  Led: Specifies the Led to be configured.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  *     @arg LED4
  * @retval None
  */
void BSP_LED_Init(Led_TypeDef Led);
/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  *     @arg LED4
  * @retval None
  */
void BSP_LED_On(Led_TypeDef Led);
/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  *     @arg LED4
  * @retval None
  */
void BSP_LED_Off(Led_TypeDef Led);

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  *     @arg LED4
  * @retval None
  */
void BSP_LED_Toggle(Led_TypeDef Led);

/**
  * @brief  Configures push button GPIO and EXTI Line.
  * @param  Button: Button to be configured.
  *   This parameter can be one of the following values:
  *     @arg BUTTON_TAMPER: Key/Tamper Push Button
  *     @arg BUTTON_SEL   : Sel Push Button on Joystick
  *     @arg BUTTON_LEFT  : Left Push Button on Joystick
  *     @arg BUTTON_RIGHT : Right Push Button on Joystick
  *     @arg BUTTON_DOWN  : Down Push Button on Joystick
  *     @arg BUTTON_UP    : Up Push Button on Joystick
  * @param  Button_Mode: Button mode requested.
  *   This parameter can be one of the following values:
  *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
  *     @arg BUTTON_MODE_EVT : Button will be connected to EXTI line
  *                            with event generation capability
  *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line
  *                            with interrupt generation capability
  * @retval None
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
/**
  * @brief  Returns the selected button state.
  * @param  Button: Button to be checked.
  *   This parameter can be one of the following values:
  *     @arg BUTTON_TAMPER: Key/Tamper Push Button
  * @retval Button state
  */
uint32_t BSP_PB_GetState(Button_TypeDef Button);

/**
  * @brief  Configures all button of the joystick in GPIO or EXTI modes.
  * @param  Joy_Mode: Joystick mode.
  *    This parameter can be one of the following values:
  *     @arg  JOY_MODE_GPIO: Joystick pins will be used as simple IOs
  *     @arg  JOY_MODE_EXTI: Joystick pins will be connected to EXTI line
  *                                 with interrupt generation capability
  * @retval HAL_OK: if all initializations are OK. Other value if error.
  */
uint8_t BSP_JOY_Init(JOYMode_TypeDef Joy_Mode);

/**
  * @brief  Returns the current joystick status.
  * @retval Code of the joystick key pressed
  *          This code can be one of the following values:
  *            @arg  JOY_SEL
  *            @arg  JOY_DOWN
  *            @arg  JOY_LEFT
  *            @arg  JOY_RIGHT
  *            @arg  JOY_UP
  *            @arg  JOY_NONE
  */
JOYState_TypeDef BSP_JOY_GetState(void);

#endif /* BOARD_H_ */

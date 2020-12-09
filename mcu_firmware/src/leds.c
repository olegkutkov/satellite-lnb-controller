/*
   leds.c
     - LEDs controller
 
   Copyright 2020  Oleg Kutkov <contact@olegkutkov.me>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
 */

#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"
#include "stm32f1xx_ll_bus.h"

/* Define LEDs ports and pins */
#define LED3_CH1 LL_GPIO_PIN_1
#define LED3_CH1_GPIO_Port GPIOB
#define LED3_CH2 LL_GPIO_PIN_5
#define LED3_CH2_GPIO_Port GPIOA

#define LED2_CH1 LL_GPIO_PIN_10
#define LED2_CH1_GPIO_Port GPIOB
#define LED2_CH2 LL_GPIO_PIN_6
#define LED2_CH2_GPIO_Port GPIOA

#define LED1_CH1 LL_GPIO_PIN_11
#define LED1_CH1_GPIO_Port GPIOB
#define LED1_CH2 LL_GPIO_PIN_7
#define LED1_CH2_GPIO_Port GPIOA

#define LED_SYS_BLUE LL_GPIO_PIN_13
#define LED_SYS_BLUE_GPIO_Port GPIOC

/* LEDx ON/OFF macro */
#define LED1_CH1_OFF()  LL_GPIO_SetOutputPin(LED1_CH1_GPIO_Port, LED1_CH1);
#define LED1_CH1_ON()  LL_GPIO_ResetOutputPin(LED1_CH1_GPIO_Port, LED1_CH1);
#define LED1_CH2_OFF()  LL_GPIO_SetOutputPin(LED1_CH2_GPIO_Port, LED1_CH2);
#define LED1_CH2_ON()  LL_GPIO_ResetOutputPin(LED1_CH2_GPIO_Port, LED1_CH2);

#define LED2_CH1_OFF()  LL_GPIO_SetOutputPin(LED2_CH1_GPIO_Port, LED2_CH1);
#define LED2_CH1_ON()  LL_GPIO_ResetOutputPin(LED2_CH1_GPIO_Port, LED2_CH1);
#define LED2_CH2_OFF()  LL_GPIO_SetOutputPin(LED2_CH2_GPIO_Port, LED2_CH2);
#define LED2_CH2_ON()  LL_GPIO_ResetOutputPin(LED2_CH2_GPIO_Port, LED2_CH2);

#define LED3_CH1_OFF() LL_GPIO_SetOutputPin(LED3_CH1_GPIO_Port, LED3_CH1);
#define LED3_CH1_ON()  LL_GPIO_ResetOutputPin(LED3_CH1_GPIO_Port, LED3_CH1);
#define LED3_CH2_OFF() LL_GPIO_SetOutputPin(LED3_CH2_GPIO_Port, LED3_CH2);
#define LED3_CH2_ON()  LL_GPIO_ResetOutputPin(LED3_CH2_GPIO_Port, LED3_CH2);

#define SYS_LED_OFF() LL_GPIO_SetOutputPin(LED_SYS_BLUE_GPIO_Port, LED_SYS_BLUE);
#define SYS_LED_ON()  LL_GPIO_ResetOutputPin(LED_SYS_BLUE_GPIO_Port, LED_SYS_BLUE);
/* */

void init_leds(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Enable clocking for the GPIO lines */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOD);

	/* Configure output LED GPIOs */
	LL_GPIO_ResetOutputPin(LED_SYS_BLUE_GPIO_Port, LED_SYS_BLUE);
	LL_GPIO_ResetOutputPin(LED3_CH1_GPIO_Port, LED3_CH1);
	LL_GPIO_ResetOutputPin(LED2_CH1_GPIO_Port, LED2_CH1);
	LL_GPIO_ResetOutputPin(LED1_CH1_GPIO_Port, LED1_CH1);

	/* Init blue sys LED */
	GPIO_InitStruct.Pin = LED_SYS_BLUE;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED_SYS_BLUE_GPIO_Port, &GPIO_InitStruct);

	/* Init LED 3 */
	GPIO_InitStruct.Pin = LED3_CH1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED3_CH1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED3_CH2;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED3_CH2_GPIO_Port, &GPIO_InitStruct);

	/* Init LED 2 */
	GPIO_InitStruct.Pin = LED2_CH1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED2_CH1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED2_CH2;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED2_CH2_GPIO_Port, &GPIO_InitStruct);

	/* Init LED 1 */
	GPIO_InitStruct.Pin = LED1_CH1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED1_CH1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED1_CH2;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LED1_CH2_GPIO_Port, &GPIO_InitStruct);
}

void led13v_ch1_on(void)
{
	LED1_CH1_ON();
}

void led13v_ch1_off(void)
{
	LED1_CH1_OFF();
}

void led13v_ch2_on(void)
{
	LED1_CH2_ON();
}

void led13v_ch2_off(void)
{
	LED1_CH2_OFF();
}

void led18v_ch1_on(void)
{
	LED2_CH1_ON();
}

void led18v_ch1_off(void)
{
	LED2_CH1_OFF();
}

void led18v_ch2_on(void)
{
	LED2_CH2_ON();
}

void led18v_ch2_off(void)
{
	LED2_CH2_OFF();
}

void led22khz_ch1_tone_on(void)
{
	LED3_CH1_ON();
}

void led22khz_ch1_tone_off(void)
{
	LED3_CH1_OFF();
}

void led22khz_ch2_tone_on(void)
{
	LED3_CH2_ON();
}

void led22khz_ch2_tone_off(void)
{
	LED3_CH2_OFF();
}

void system_led_on(void)
{
	SYS_LED_ON();
}

void system_led_off(void)
{
	SYS_LED_OFF();
}

/* Just a funky blink */
void boot_blink(void)
{
	LL_mDelay(100);
	LED1_CH1_OFF();
	LL_mDelay(60);
	LED2_CH1_OFF();
	LL_mDelay(60);
	LED3_CH1_OFF();
	LL_mDelay(60);
	LED1_CH2_OFF();
	LL_mDelay(60);
	LED2_CH2_OFF();
	LL_mDelay(60);
	LED3_CH2_OFF();
	LL_mDelay(60);
	SYS_LED_OFF();
}


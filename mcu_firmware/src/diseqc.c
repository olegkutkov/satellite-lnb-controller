/*
   diseqc.c
    - LNB Controller: power supply, polarity, bands

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

#include <string.h>
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_utils.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_tim.h"
#include "diseqc.h"
#include "leds.h"
#include "device_state.h"

#define OUT_VOLTAGE_MODE_13V	0x0D /* 13v is Vertical/Right */
#define OUT_VOLTAGE_MODE_18V	0x12 /* 18v is Horizontal/Left */

/* Define channels control pins and ports */
#define CH1_VOLTAGE_CTRL_PIN LL_GPIO_PIN_9
#define CH1_VOLTAGE_CTRL_PORT GPIOB
#define CH2_VOLTAGE_CTRL_PIN LL_GPIO_PIN_8
#define CH2_VOLTAGE_CTRL_PORT GPIOB

#define PS_CTRL_PIN LL_GPIO_PIN_4
#define PS_CTRL_PORT GPIOA

/* Storage for the current device configuration */
/* Useful for the control and possible reconnects of the desktop soft */
static struct controller_state ctrl_state_storage;

/* Init TIM2 for 22KHz square wave */
/* This timer is used for the both channels */
static void init_22KHz_timer(void)
{
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

	/* Configuration of the timer and channels  */
	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 1085;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIM2, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM2);
	LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
	LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH2);
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 600;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH2);
	LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIM2);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

	/* Configure channels pins */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	LL_TIM_EnableIT_UPDATE(TIM2);

	LL_TIM_EnableCounter(TIM2);
}

/* Configure all GPIO pins */
static void init_diseqc_gpio(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    LL_GPIO_ResetOutputPin(CH1_VOLTAGE_CTRL_PORT, CH1_VOLTAGE_CTRL_PIN);
    LL_GPIO_ResetOutputPin(CH2_VOLTAGE_CTRL_PORT, CH2_VOLTAGE_CTRL_PIN);
    LL_GPIO_ResetOutputPin(PS_CTRL_PORT, PS_CTRL_PIN);

	/* Channel 1 voltage control pin */
    GPIO_InitStruct.Pin = CH1_VOLTAGE_CTRL_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(CH1_VOLTAGE_CTRL_PORT, &GPIO_InitStruct);

	/* Channel 2 voltage control pin */
    GPIO_InitStruct.Pin = CH2_VOLTAGE_CTRL_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(CH2_VOLTAGE_CTRL_PORT, &GPIO_InitStruct);

	/* Power supply control pin */
    GPIO_InitStruct.Pin = PS_CTRL_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(PS_CTRL_PORT, &GPIO_InitStruct);
}

/* Set channel 1 22KHz tone mode (enabled/disabled) */
void diseq_set_ch1_tone_signal_mode(uint8_t enabled)
{
	if (enabled) {
		LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
		led22khz_ch1_tone_on();
		ctrl_state_storage.ch1_tone_enabled = 1;
	} else {
		LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH1);
		led22khz_ch1_tone_off();
		ctrl_state_storage.ch1_tone_enabled = 0;
	}
}

/* Get the current saved state of the Channel 1 tone mode */
uint8_t diseq_get_ch1_tone_signal_mode(void)
{
	return ctrl_state_storage.ch1_tone_enabled;
}

/* Set channel 2 22KHz tone mode (enabled/disabled) */
void diseq_set_ch2_tone_signal_mode(uint8_t enabled)
{
	if (enabled) {
		LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);
		led22khz_ch2_tone_on();
		ctrl_state_storage.ch2_tone_enabled = 1;
	} else {
		LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2);
		led22khz_ch2_tone_off();
		ctrl_state_storage.ch2_tone_enabled = 0;
	}
}

/* Get the current saved state of the Channel 2 tone mode */
uint8_t diseq_get_ch2_tone_signal_mode(void)
{
	return ctrl_state_storage.ch2_tone_enabled;
}

/* Set power supply mode (enabled/disabled) */
void diseqc_set_ps_mode(uint8_t enabled)
{
	if (enabled) {
		LL_GPIO_SetOutputPin(PS_CTRL_PORT, PS_CTRL_PIN);
		ctrl_state_storage.ps_enabled = 1;
	} else {
		LL_GPIO_ResetOutputPin(PS_CTRL_PORT, PS_CTRL_PIN);
		ctrl_state_storage.ps_enabled = 0;
	}
}

/* Get the current saved state of the power supply */
uint8_t diseqc_get_ps_mode(void)
{	
	return ctrl_state_storage.ps_enabled;
}

/* Set the channel 1 output voltage (13v/18v) */
void diseqc_set_ch1_out_voltage(uint8_t voltage_val)
{
	if (voltage_val == OUT_VOLTAGE_MODE_13V) {
		LL_GPIO_SetOutputPin(CH1_VOLTAGE_CTRL_PORT, CH1_VOLTAGE_CTRL_PIN);
		led18v_ch1_off();
		led13v_ch1_on();
		ctrl_state_storage.ch1_out_voltage_high = 0;
	} else if (voltage_val == OUT_VOLTAGE_MODE_18V) {
		LL_GPIO_ResetOutputPin(CH1_VOLTAGE_CTRL_PORT, CH1_VOLTAGE_CTRL_PIN);
		led13v_ch1_off();
		led18v_ch1_on();
		ctrl_state_storage.ch1_out_voltage_high = 1;
	}
}

/* Get the current output voltage state of the channel 1 */
uint8_t diseqc_get_ch1_out_voltage(void)
{
	return ctrl_state_storage.ch1_out_voltage_high;
}

/* Set the channel 2 output voltage (13v/18v) */
void diseqc_set_ch2_out_voltage(uint8_t voltage_val)
{
	if (voltage_val == OUT_VOLTAGE_MODE_13V) {
		LL_GPIO_SetOutputPin(CH2_VOLTAGE_CTRL_PORT, CH2_VOLTAGE_CTRL_PIN);
		led18v_ch2_off();
		led13v_ch2_on();
		ctrl_state_storage.ch2_out_voltage_high = 0;
	} else if (voltage_val == OUT_VOLTAGE_MODE_18V) {
		LL_GPIO_ResetOutputPin(CH2_VOLTAGE_CTRL_PORT, CH2_VOLTAGE_CTRL_PIN);
		led13v_ch2_off();
		led18v_ch2_on();
		ctrl_state_storage.ch2_out_voltage_high = 1;
	}
}

/* Get the current output voltage state of the channel 2 */
uint8_t diseqc_get_ch2_out_voltage(void)
{
	return ctrl_state_storage.ch2_out_voltage_high;
}

/* Entry point of the module */
void init_diseqc(void)
{
	/* Init timer and GPIO */
	init_22KHz_timer();
	init_diseqc_gpio();

	/* Clear the saved state structure */
	memset(&ctrl_state_storage, 0, sizeof(struct controller_state));

	/* Apply default configuration */
	/* Power supply - disabled */
	/* Channel 1/2 output voltage - 13v */
	/* Channel 1/2 tone signal - off */
	diseqc_set_ps_mode(0);
	diseqc_set_ch1_out_voltage(OUT_VOLTAGE_MODE_13V);
	diseqc_set_ch2_out_voltage(OUT_VOLTAGE_MODE_13V);
	diseq_set_ch1_tone_signal_mode(0);
	diseq_set_ch2_tone_signal_mode(0);

	/* Initial state of the LEDs */
	led18v_ch1_off();
	led13v_ch1_on();

	led18v_ch2_off();
	led13v_ch2_on();
}

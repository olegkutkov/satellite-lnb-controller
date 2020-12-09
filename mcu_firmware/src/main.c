/*
   main.c
    - Firmware entry point

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

//#include "main.h"
#include "usb_device.h"
#include "stm32f1xx_ll_gpio.h"
#include "leds.h"
#include "diseqc.h"
#include "voltage_reader.h"

void configure_system_clocks(void);

int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	/* Required by USB driver */
	HAL_Init();

	configure_system_clocks();

	/* Initialize all  peripherals */
	init_leds();
	boot_blink();
	init_diseqc();
	init_voltage_reader();

	MX_USB_DEVICE_Init();

	/* Turn on the System LED */
	/* This will means that FW is started properly */
	system_led_on();

	while (1) {
		/* Just turn on the System LED in cycle */
		/* This LED is activated from the different parts of the FW */
		LL_mDelay(100);
		system_led_off();
	}

	return 0;
}

void configure_system_clocks(void)
{
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

	if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
		Error_Handler();
	}

	LL_RCC_HSE_Enable();

	/* Wait till HSE is ready */
	while (LL_RCC_HSE_IsReady() != 1) {    
	}

	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_6);
	LL_RCC_PLL_Enable();

	/* Wait till PLL is ready */
	while (LL_RCC_PLL_IsReady() != 1) {
	}

	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

	/* Wait till System clock is ready */
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
	}

	LL_SetSystemCoreClock(48000000);

	/* Update the time base */
	if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK) {
		Error_Handler();  
	};

	LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL);
}

/* Required by the USB Framework */
void Error_Handler(void)
{
	
}

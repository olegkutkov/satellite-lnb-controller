/*
   voltage_reader.c
    - Simple output voltages reader with ADC1 and DMA

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
#include "stm32f1xx_ll_adc.h"
#include "stm32f1xx_ll_dma.h"

#define ADC_DELAY_ENABLE_CALIB_CPU_CYCLES \
			(LL_ADC_DELAY_ENABLE_CALIB_ADC_CYCLES * 32)

#define VDDA_APPLI ((uint32_t)3300)
#define DATA_SIZE 2 /* 2 channels */

static volatile uint16_t adc_data[DATA_SIZE] = {0, 0};
static volatile uint8_t dma_transfer_complete = 0;

/* */

static void init_adc(void)
{
	LL_ADC_InitTypeDef ADC_InitStruct = {0};
	LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
	LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
  
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

	/* ADC1 GPIO Configuration  
		PA2   ------> ADC1_IN2
		PA3   ------> ADC1_IN3 
	*/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Common config */
	ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
	ADC_InitStruct.SequencersScanMode = LL_ADC_SEQ_SCAN_ENABLE;
	LL_ADC_Init(ADC1, &ADC_InitStruct);
	ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
	LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);
	ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
	ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
	ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
	ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
	ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
	LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);

	/* Configure Regular Channel */
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_2);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_2, LL_ADC_SAMPLINGTIME_71CYCLES_5);

	/** Configure Regular Channel */
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_3);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_3, LL_ADC_SAMPLINGTIME_71CYCLES_5);
}

static void init_dma(void)
{
	/* Configure DMA interrupts */
	NVIC_SetPriority(DMA1_Channel1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	/* Enable DMA clock */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

	/* Configure ADC - MEMORY data transfer direction and params */
	LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_HIGH);
	LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);
	LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_HALFWORD);
	LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_HALFWORD);

	/* Set destination memory address */
	LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1, LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA)
								, (uint32_t)&adc_data, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);	

	/* Set desitnation buffer size (num of the channels) */
	LL_DMA_SetDataLength(DMA1,LL_DMA_CHANNEL_1, DATA_SIZE);	

	/* Enable DMA transfer complete interrupt */
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);

	LL_DMA_EnableChannel(DMA1,LL_DMA_CHANNEL_1);
}

/* DMA interrupt callback 
    see stm32f1xx_it.c
 */
void dma_transfer_complete_cb(void)
{
	dma_transfer_complete = 1;
}

void adc_calibrate_and_run(void)
{
	uint32_t wait_loop_index = ADC_DELAY_ENABLE_CALIB_CPU_CYCLES >> 1;

	LL_ADC_Enable(ADC1);

	/* Give ADC some time to settle down */
	while (!wait_loop_index) {
		--wait_loop_index;
	}

	/* Run self-calibration */
	LL_ADC_StartCalibration(ADC1);

	/* Wait for self-calibration done */
	while (LL_ADC_IsCalibrationOnGoing(ADC1)) {
	}

	/* Trigger conversion */
	LL_ADC_REG_StartConversionSWStart(ADC1);
}

/* Module entry point */
void init_voltage_reader(void)
{
	init_dma();
	init_adc();
	adc_calibrate_and_run();
}

uint16_t get_ch1_voltage(void)
{
	/* Wait for DMA data transfer complete */
	while (!dma_transfer_complete) {
	}

	dma_transfer_complete = 0;

	return __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, adc_data[0], LL_ADC_RESOLUTION_12B);
}

uint16_t get_ch2_voltage(void)
{
	/* Wait for DMA data transfer complete */
	while (!dma_transfer_complete) {
	}

	dma_transfer_complete = 0;

	return __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, adc_data[1], LL_ADC_RESOLUTION_12B);
}


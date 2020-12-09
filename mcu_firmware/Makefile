#
# Makefile
#
#   Copyright 2020  Oleg Kutkov <contact@olegkutkov.me>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
#

######################################
# target
######################################
TARGET = stm32_diseqc

######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -O2 #-flto

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build
PRECOMPILED_DIR = precompiled

######################################
# source
######################################
# C sources
C_SOURCES =  \
	src/main.c \
	src/leds.c \
	src/usb_protocol.c \
	src/diseqc.c \
	src/voltage_reader.c \
	src/crc8.c \
	src/usb_device.c \
	src/usbd_conf.c \
	src/usbd_desc.c \
	src/usbd_cdc_if.c \
	src/stm32f1xx_it.c \
	src/stm32f1xx_hal_msp.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_gpio.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd_ex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usb.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_rcc.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_utils.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_exti.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_adc.c \
	src/system_stm32f1xx.c \
	middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
	middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
	middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
	middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_tim.c \
	drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_dma.c

# ASM sources
ASM_SOURCES =  \
	startup_stm32f103xb.s

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m3

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
	-DUSE_FULL_LL_DRIVER \
	-DUSE_HAL_DRIVER \
	-DSTM32F103xB


# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
	-Iinc \
	-Idrivers/STM32F1xx_HAL_Driver/Inc \
	-Idrivers/STM32F1xx_HAL_Driver/Inc/Legacy \
	-Imiddlewares/ST/STM32_USB_Device_Library/Core/Inc \
	-Imiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
	-Idrivers/CMSIS/Device/ST/STM32F1xx/Include \
	-Idrivers/CMSIS/Include

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F103C8Tx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

######################################
# flash firmware
######################################
upload:
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000

upload-precompiled:
	st-flash write $(PRECOMPILED_DIR)/$(TARGET).bin 0x08000000

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)


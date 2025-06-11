#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2017, Krzysztof Witek
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# The full GNU General Public License is included in this distribution in
# the file called "LICENSE".
#

ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ARCH = x86

else ifeq ($(CONFIG_ARCH),X86_TEST)

ARCH = x86

else ifeq ($(CONFIG_ARCH),AVR)
ifeq "$(or $(CONFIG_USART0), $(CONFIG_USART1))" "y"
	SRC += $(ARCH_DIR)/avr/_stdio.c
	SRC += $(ARCH_DIR)/avr/usart.c
endif

ifdef CONFIG_ADC
	SRC += $(ARCH_DIR)/avr/adc.c
endif
AVR_FLAGS = -DF_CPU=${CONFIG_AVR_F_CPU} -mmcu=${CONFIG_AVR_MCU}
AVR_FLAGS += -DF_CPU=$(CONFIG_AVR_F_CPU) -DCONFIG_AVR_MCU
AVR_FLAGS += -DCONFIG_AVR_F_CPU=$(CONFIG_AVR_F_CPU)

ifeq ($(CONFIG_AVR_MCU),attiny85)
AVR_FLAGS += -DATTINY85
endif
ifeq ($(CONFIG_AVR_MCU),atmega2561)
AVR_FLAGS += -DATMEGA2561
endif
ifeq ($(CONFIG_AVR_MCU),atmega2560)
AVR_FLAGS += -DATMEGA2560
endif
ifeq ($(CONFIG_AVR_MCU),atmega328p)
AVR_FLAGS += -DATMEGA328P
endif
ifeq ($(CONFIG_AVR_MCU),atmega16u2)
AVR_FLAGS += -DF_USB=${CONFIG_AVR_F_CPU}
endif

CFLAGS += $(AVR_FLAGS)
LDFLAGS += $(AVR_FLAGS)

ARCH = avr
CC = avr-gcc
AR = avr-ar
endif

ifeq ($(ARCH),x86)
CFLAGS += -DX86
CC = gcc
AR = ar
endif

CFLAGS += -I$(ARCH_DIR)/$(ARCH)

# Do not use a too small timer resolution on x86 in order
# not to have a busy CPU.
ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ifeq ($(shell test $(CONFIG_TIMER_RESOLUTION_US) -lt 1000; echo $$?),0)
CONFIG_TIMER_RESOLUTION_US=1000
endif
endif

CFLAGS += -DCONFIG_TIMER_RESOLUTION_US=$(CONFIG_TIMER_RESOLUTION_US)

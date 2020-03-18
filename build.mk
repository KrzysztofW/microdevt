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

ifdef CONFIG_SCHEDULER_MAX_TASKS
CFLAGS += -DCONFIG_SCHEDULER_MAX_TASKS=$(CONFIG_SCHEDULER_MAX_TASKS)
SRC += $(ROOT_PATH)/sys/scheduler.c
ifdef CONFIG_SCHEDULER_TASK_WATER_MARK
CFLAGS += -DCONFIG_SCHEDULER_TASK_WATER_MARK=$(CONFIG_SCHEDULER_TASK_WATER_MARK)
endif
endif

ifdef CONFIG_USART0
CFLAGS += -DCONFIG_USART0
ifdef CONFIG_USART0_SPEED
CFLAGS += -DCONFIG_USART0_SPEED=$(CONFIG_USART0_SPEED)
else
CFLAGS += -DCONFIG_USART0_SPEED=57600
endif
endif

ifdef CONFIG_USART1
CFLAGS += -DCONFIG_USART1
ifdef CONFIG_USART1_SPEED
CFLAGS += -DCONFIG_USART1_SPEED=$(CONFIG_USART1_SPEED)
else
CFLAGS += -DCONFIG_USART1_SPEED=57600
endif
endif

ifdef CONFIG_RND_SEED
CFLAGS += -DCONFIG_RND_SEED
SRC += $(ROOT_PATH)/sys/random.c
endif

ifeq "$(or $(CONFIG_RF_RECEIVER), $(CONFIG_RF_SENDER))" "y"
SRC += $(ROOT_PATH)/drivers/rf.c
endif
ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
ifdef CONFIG_RF_GENERIC_COMMANDS
CFLAGS += -DCONFIG_RF_GENERIC_COMMANDS
CFLAGS += -DCONFIG_RF_GENERIC_COMMANDS_SIZE=$(CONFIG_RF_GENERIC_COMMANDS_SIZE)
endif
endif
ifdef CONFIG_RF_SENDER
CFLAGS += -DCONFIG_RF_SENDER
endif

ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
endif

ifdef CONFIG_RF_CHECKS
CFLAGS += -DCONFIG_RF_CHECKS
SRC += $(ROOT_PATH)/drivers/rf-checks.c
endif

ifdef CONFIG_AVR_SIMU
SRC += $(ARCH_DIR)/$(ARCH)/simu.c
CFLAGS += -DCONFIG_AVR_SIMU -DCONFIG_AVR_SIMU_PATH=$(CONFIG_AVR_SIMU_PATH)
endif

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

ifdef CONFIG_TIMER_RESOLUTION_US
CFLAGS += -DCONFIG_TIMER_RESOLUTION_US=$(CONFIG_TIMER_RESOLUTION_US)
SRC += $(ROOT_PATH)/sys/timer.c $(ARCH_DIR)/$(ARCH)/timer.c
endif

ifdef CONFIG_ETHERNET
CFLAGS += -DCONFIG_ETHERNET
endif

ifdef CONFIG_IP
CFLAGS += -DCONFIG_IP
endif

ifdef CONFIG_UDP
CFLAGS += -DCONFIG_UDP
endif

ifdef CONFIG_DNS
CFLAGS += -DCONFIG_DNS
endif

ifdef CONFIG_TCP
CFLAGS += -DCONFIG_TCP
ifdef CONFIG_TCP_CLIENT
CFLAGS += -DCONFIG_TCP_CLIENT
endif
endif

ifdef CONFIG_BSD_COMPAT
CFLAGS += -DCONFIG_BSD_COMPAT
endif

ifdef CONFIG_EVENT
CFLAGS += -DCONFIG_EVENT
endif

ifdef CONFIG_GSM_SIM900
SRC += $(ROOT_PATH)/drivers/gsm-at.c gsm.c
CFLAGS += -DCONFIG_GSM_SIM900
endif

ifdef CONFIG_NETWORKING
CFLAGS += -DCONFIG_NETWORKING
SRC += network.c $(ARCH_DIR)/$(ARCH)/enc28j60.c
endif
ifdef CONFIG_PKT_NB_MAX
CFLAGS += -DCONFIG_PKT_NB_MAX=$(CONFIG_PKT_NB_MAX)
CFLAGS += -DCONFIG_PKT_SIZE=$(CONFIG_PKT_SIZE)
CFLAGS += -DCONFIG_PKT_DRIVER_NB_MAX=$(CONFIG_PKT_DRIVER_NB_MAX)
endif

ifdef CONFIG_SWEN_L3
CFLAGS += -DCONFIG_SWEN_L3
SRC += $(ROOT_PATH)/crypto/xtea.c
endif

ifdef CONFIG_POWER_MANAGEMENT
ifeq ($(CONFIG_AVR_SIMU),)
CFLAGS += -DCONFIG_POWER_MANAGEMENT
SRC += $(ROOT_PATH)/sys/power-management.c
endif
endif

ifdef CONFIG_IR_DONT_CHECK_INVERSIONS
CFLAGS += -DCONFIG_IR_DONT_CHECK_INVERSIONS
endif
ifdef CONFIG_IR_ADDRESS
CFLAGS+= -DCONFIG_IR_ADDRESS=$(CONFIG_IR_ADDRESS)
endif

include $(ROOT_PATH)/arch/build.mk

CFLAGS += -DVERSION=\"$(GIT_VERSION)\" -I$(ROOT_PATH) -I.
OBJ = $(SRC:.c=.o)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

export DEBUG
export CFG_FILE = $(PWD)/config
export CFLAGS

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

ifeq ($(CONFIG_ARCH),AVR)
SOURCES += $(ARCH_DIR)/$(ARCH)/timer.c
endif

ifdef CONFIG_AVR_SIMU
SOURCES += $(ARCH_DIR)/$(ARCH)/simu.c
CFLAGS += -DCONFIG_AVR_SIMU -DCONFIG_AVR_SIMU_PATH=$(CONFIG_AVR_SIMU_PATH)
undefine CONFIG_POWER_MANAGEMENT
endif

ifdef CONFIG_POWER_MANAGEMENT
CFLAGS += -DCONFIG_POWER_MANAGEMENT
SOURCES += ../../sys/power-management.c
endif

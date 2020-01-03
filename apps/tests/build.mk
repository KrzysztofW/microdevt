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


ifdef CONFIG_USE_CAPABILITIES
CFLAGS += -DCONFIG_USE_CAPABILITIES
LDFLAGS += -lcap
endif

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
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

ifeq "$(or $(CONFIG_RF_RECEIVER), $(CONFIG_RF_SENDER))" "y"
SOURCES += ../../drivers/rf.c ../../drivers/rf-checks.c
ifdef CONFIG_RF_SENDER
CFLAGS += -DCONFIG_RF_SENDER
endif

ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
endif

ifdef CONFIG_RF_BURST
CFLAGS += -DCONFIG_RF_BURST
endif

ifdef CONFIG_RF_GENERIC_COMMANDS
CFLAGS += -DCONFIG_RF_GENERIC_COMMANDS -DCONFIG_RF_GENERIC_COMMANDS_SIZE=$(CONFIG_RF_GENERIC_COMMANDS_SIZE)
endif

ifdef CONFIG_RF_CHECKS
CFLAGS += -DCONFIG_RF_CHECKS
endif
endif

ifdef CONFIG_SWEN_L3
CFLAGS += -DCONFIG_SWEN_L3
endif

ifdef CONFIG_PKT_NB_MAX
CFLAGS += -DCONFIG_PKT_NB_MAX=$(CONFIG_PKT_NB_MAX)
CFLAGS += -DCONFIG_PKT_SIZE=$(CONFIG_PKT_SIZE)
CFLAGS += -DCONFIG_PKT_DRIVER_NB_MAX=$(CONFIG_PKT_DRIVER_NB_MAX)
endif

ifdef CONFIG_HT_STORAGE
CFLAGS += -DCONFIG_HT_STORAGE
endif

ifdef CONFIG_EVENT
CFLAGS += -DCONFIG_EVENT
endif

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

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

ifdef CONFIG_PKT_NB_MAX
CFLAGS += -DCONFIG_PKT_NB_MAX=$(CONFIG_PKT_NB_MAX)
CFLAGS += -DCONFIG_PKT_DRIVER_NB_MAX=$(CONFIG_PKT_DRIVER_NB_MAX)
CFLAGS += -DCONFIG_PKT_SIZE=$(CONFIG_PKT_SIZE)
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

ifdef CONFIG_HT_STORAGE
CFLAGS += -DCONFIG_HT_STORAGE
endif

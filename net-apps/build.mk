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

ifdef CONFIG_UDP
ifdef CONFIG_BSD_COMPAT
SRC += $(NET_APPS_DIR)/udp-apps-bsd.c
else
SRC += $(NET_APPS_DIR)/udp-apps.c
endif
ifdef CONFIG_DNS
SRC += $(NET_APPS_DIR)/dns-app.c
endif
endif

ifdef CONFIG_TCP
ifdef CONFIG_BSD_COMPAT
SRC += $(NET_APPS_DIR)/tcp-apps-bsd.c
else
SRC += $(NET_APPS_DIR)/tcp-apps.c
endif
endif

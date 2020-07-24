#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2020, Krzysztof Witek
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 3, as published by the Free Software Foundation.
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

ROOT_PATH = ../..
ARCH_DIR=$(ROOT_PATH)/arch

EXE = watering-pump
CFLAGS = -Wall -Werror -Os -g -c
SRC = main.c $(ARCH_DIR)/$(ARCH)/watchdog.c

include config
include $(ROOT_PATH)/build.mk

# FUSES see http://www.engbedded.com/fusecalc

# 1mhz (internal clk/8)
FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

# 8mhz (internal clk)
# FUSES = lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

# 16mhz external crystal
# FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

all: $(SRC) $(EXE)
all_not_size_optimized: $(SRC) $(EXE)_not_size_optimized
include $(ROOT_PATH)/common.mk
include $(ARCH_DIR)/$(ARCH)/common.mk

.PHONY: all all_not_size_optimized


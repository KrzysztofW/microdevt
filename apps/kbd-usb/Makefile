#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2024, Krzysztof Witek
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

# Output file names
TARGET       = Keyboard
SRC = main.c Descriptors.c $(ARCH_DIR)/$(ARCH)/watchdog.c

# LUFA library path
LUFA_PATH    = /home/chris/Downloads/lufa
LUFA_SRC     = $(LUFA_PATH)/LUFA/Drivers/USB
LUFA_INC     = $(LUFA_PATH)/LUFA

# Compiler and linker flags
CFLAGS       = -DUSE_LUFA_CONFIG_HEADER -I. -I$(LUFA_INC) -I$(LUFA_PATH) -Os
LDFLAGS      = -Wl,--gc-sections

# Include LUFA modules
CFLAGS      += -I$(LUFA_INC)/Class -I$(LUFA_SRC)
SRC         += $(LUFA_SRC)/Class/Common/HIDParser.c
SRC         += $(LUFA_SRC)/Class/Device/HIDClassDevice.c
SRC         += $(LUFA_SRC)/Class/Device/CDCClassDevice.c
SRC         += $(LUFA_SRC)/Core/ConfigDescriptors.c
SRC         += $(LUFA_SRC)/Core/DeviceStandardReq.c
SRC         += $(LUFA_SRC)/Core/AVR8/USBController_AVR8.c
SRC         += $(LUFA_SRC)/Core/AVR8/Endpoint_AVR8.c
SRC         += $(LUFA_SRC)/Core/AVR8/EndpointStream_AVR8.c
SRC         += $(LUFA_SRC)/Core/AVR8/USBInterrupt_AVR8.c
SRC         += $(LUFA_SRC)/Core/Events.c
SRC         += $(LUFA_SRC)/Core/USBTask.c

ROOT_PATH = ../..
ARCH_DIR=$(ROOT_PATH)/arch

EXE = kbd-usb
#CFLAGS = -Wall -Werror -Os -g -c

include config
include $(ROOT_PATH)/build.mk

# FUSES see http://www.engbedded.com/fusecalc

# 8mhz (internal clk)
FUSES = -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xf4:m

# 16mhz external crystal
# FUSES = -U lfuse:w:0xff:m -U hfuse:w:0xd9:m -U efuse:w:0xf4:m

all: keyboard-layout payload-script-compiler $(SRC) $(EXE)
all_not_size_optimized: $(SRC) $(EXE)_not_size_optimized
include $(ROOT_PATH)/common.mk
include $(ARCH_DIR)/$(ARCH)/common.mk

keyboard-layout: keyboard-layout.c lufa-key-codes.h kbd-layout-us.h
	gcc -Wall keyboard-layout.c -o keyboard-layout

payload-script-compiler: payload-script-compiler.c kbd-layout-fr.h kbd-layout-us.h kbd-layout-scan-codes-ref.h
	gcc -I../.. -I../../arch/x86 -DCONFIG_EEPROM_SIZE=$(CONFIG_EEPROM_SIZE) payload-script-compiler.c -o payload-script-compiler -g -O0

clean: _clean
	@rm -f keyboard-layout payload-script-compiler

.PHONY: all all_not_size_optimized clean

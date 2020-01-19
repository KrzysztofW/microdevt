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

ROOT_PATH = ../..
ARCH_DIR=$(ROOT_PATH)/arch

EXE = rf-test
CFLAGS = -Wall -Werror -Os -g -c -DVERSION=\"$(GIT_VERSION)\" -DOPT_MAX_ARGS=3
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

SOURCES = main.c ../../sys/scheduler.c ../../sys/opts.c
SOURCES += $(ARCH_DIR)/$(ARCH)/watchdog.c

include config
include build.mk
include $(ROOT_PATH)/arch/build.mk

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

STATIC_LIBS = ../../net/libnet-avr.a
OBJ = $(SOURCES:.c=.o)

# 16mhz external crystal, preserve EEPROM
FUSES = -U lfuse:w:0xff:m -U hfuse:w:0xd0:m -U efuse:w:0xfd:m

CFLAGS += -I../.. -I.

all: libnet $(SOURCES) $(EXE)
all_not_optimized: libnet $(SOURCES) $(EXE)_not_optimized
include ../../common.mk

libnet:
	make -C ../../net

include ../../arch/avr/common.mk
.PHONY: all all_not_optimized

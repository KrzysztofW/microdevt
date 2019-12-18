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

# ../simulavr/src/simulavr  -f alarm -d atmega328 -F 16000000

ROOT_PATH = ../..
ARCH_DIR=$(ROOT_PATH)/arch

EXE = alarm
#CFLAGS = -Wall -Werror -Os -g -c -DSWEN_L3_DEBUG -DRF_DEBUG -DVERSION=\"$(GIT_VERSION)\"
CFLAGS = -Wall -Werror -Os -g -c -DRF_DEBUG -DVERSION=\"$(GIT_VERSION)\"

# definition to fake the simulator
CFLAGS += -DUSART0_RX_vect=USART_RX_vect -DDDRF=DDRD -DPORTF=PORTD -DPF1=PD1

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

SOURCES = main.c ../../sys/scheduler.c  ../../sys/array.c
SOURCES += module-common.c module.c $(ARCH_DIR)/$(ARCH)/watchdog.c
SOURCES += ../alarm/alarm-module1/module.c

include config
include build.mk
include $(ROOT_PATH)/arch/build.mk

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

NET_APPS_DIR=../../net-apps

include $(NET_APPS_DIR)/build.mk

STATIC_LIBS = ../../net/libnet-avr.a
OBJ = $(SOURCES:.c=.o)

CFLAGS += -I../.. -I.

all: libnet $(SOURCES) $(EXE)_not_optimized
all_not_optimized: libnet $(SOURCES) $(EXE)_not_optimized
include ../../common.mk

libnet:
	make -C ../../net

include ../../arch/avr/common.mk
.PHONY: all all_not_optimized

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
CFLAGS = -Wall -Werror -Os -g -c -DRF_DEBUG -DOPT_MAX_ARGS=3

# definition to fake the simulator
CFLAGS += -DUSART0_RX_vect=USART_RX_vect -DDDRF=DDRD -DPORTF=PORTD -DPF1=PD1

SRC = main.c ../../sys/array.c ../alarm/alarm-module1/module.c
SRC += module-common.c module.c $(ARCH_DIR)/$(ARCH)/watchdog.c
SRC += ../../sys/opts.c

include config
include $(ROOT_PATH)/build.mk

NET_APPS_DIR=../../net-apps

include $(NET_APPS_DIR)/build.mk

STATIC_LIBS = ../../net/libnet-avr.a

all: libnet $(SRC) $(EXE)_not_size_optimized
all_not_size_optimized: libnet $(SRC) $(EXE)_not_size_optimized
include $(ROOT_PATH)/common.mk

libnet:
	make -C ../../net

include $(ARCH_DIR)/$(ARCH)/common.mk
clean: _clean

.PHONY: all all_not_size_optimized

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

EXE = tests
CFLAGS = -Wall -Werror -O0 -g -DTEST
SRC = tests.c ../../sys/array.c ../../drivers/gsm-at.c

include config
include $(ROOT_PATH)/build.mk

export TEST=1

NET_APPS_DIR = ../../net-apps
include $(NET_APPS_DIR)/build.mk

LIBS = -L../../net -lnet-x86
STATIC_LIBS = ../../net/libnet-x86.a

all: libnet $(SRC) $(EXE)

static: libnet $(EXE)_static

libnet:
	make -C ../../net

$(EXE)_dynamic: $(OBJ) $(STATIC_LIBS)
	$(CC) $(OBJ) $(LIBS) $(LDFLAGS) -o $@

$(EXE): $(OBJ) $(STATIC_LIBS)
	$(CC) $(OBJ) $(STATIC_LIBS) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean:
	make -C ../../net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(ARCH_DIR)/$(ARCH)/*.o
	@rm -f $(EXE)_static

check: all
#	LD_LIBRARY_PATH=../../net ./tests_dynamic
	./tests || exit 1

.PHONY: all static

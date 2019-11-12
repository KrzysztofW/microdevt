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

EXE = tests
CFLAGS = -Wall -Werror -O0 -g
SOURCES = tests.c
export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS
export TEST=1

include config

include ../../arch/build.mk
include build.mk

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

NET_APPS_DIR=../../net-apps
ARCH_DIR=../../arch
include $(NET_APPS_DIR)/build.mk

CFLAGS += -I../.. -I. -DTEST

LIBS = -L../../net -lnet-x86
STATIC_LIBS = ../../net/libnet-x86.a
SOURCES += ../../sys/errno.c ../../sys/array.c
SOURCES += ../../drivers/gsm-at.c ../../crypto/xtea.c
OBJ = $(SOURCES:.c=.o)

all: libnet $(SOURCES) $(EXE)

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

run: all
#	LD_LIBRARY_PATH=../../net ./tests_dynamic
	./tests

.PHONY: all static

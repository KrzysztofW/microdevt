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

EXE = tun-driver
CFLAGS = -Wall -Werror -O0 -fno-inline -g
SOURCES = tun-driver.c tun.c

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

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

CFLAGS += -I../.. -DVERSION=\"$(GIT_VERSION)\"

LIBS = -L../../net -lnet-x86
STATIC_LIBS = ../../net/libnet-x86.a
SOURCES += ../../sys/scheduler.c
OBJ = $(SOURCES:.c=.o)

all: libnet $(SOURCES) $(EXE)
include ../../common.mk

libnet:
	make -C ../../net

$(EXE): $(OBJ) $(STATIC_LIBS)
	$(CC) $(OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $@
	@if [ -f $(EXE) ]; then sudo setcap cap_net_raw,cap_net_admin=eip $(EXE); fi

run: all
	LD_LIBRARY_PATH=../../net ./tun-driver

%.c:
	$(CC) $(CFLAGS) $*.c

clean: clean_common
	make -C ../../net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(ARCH_DIR)/$(ARCH)/*.o

.PHONY: all

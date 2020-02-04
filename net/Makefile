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

ifeq ($(CFG_FILE),)
CFLAGS = -Wall -Werror -Os -g
include default-config
else
include $(CFG_FILE)
endif

ARCH_DIR=../arch

include ../arch/build.mk
include build.mk

ifeq ($(ARCH),x86)
CFLAGS += -fPIC
LDFLAGS += -shared
SO = libnet-$(ARCH).so
endif

OBJ = $(SRC:.c=.o)
LIBNAME=libnet-$(ARCH)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

CFLAGS += -I..

all: $(OBJ) $(SO) libnet-$(ARCH).a

libnet-$(ARCH).a: $(OBJ)
	$(AR) -cr $(LIBNAME).a $(OBJ)
	ranlib $(LIBNAME).a

libnet-$(ARCH).so: $(OBJ)
	$(CC) $(LDFLAGS) $(LIBS) -o $(LIBNAME).so $(OBJ)

CHKSUM_TEST_OBJ = chksum-test.o ../sys/chksum.o
chksum_test: $(CHKSUM_TEST_OBJ)
	$(CC) $(CHKSUM_TEST_OBJ) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean:
	@rm -f $(LIBNAME).so $(LIBNAME).a *~ "#*#" $(OBJ) $(CHKSUM_TEST_OBJ) chksum_test

.PHONY: all

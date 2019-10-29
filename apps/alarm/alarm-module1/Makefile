# ../../simulavr/src/simulavr  -f alarm-module1 -d atmega328 -F 16000000

ROOT_PATH = ../../..
ARCH_DIR=$(ROOT_PATH)/arch

EXE = alarm-module1
CFLAGS = -Wall -Werror -Os -g -c -DVERSION=\"$(GIT_VERSION)\"
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif


SOURCES = main.c ../../../scheduler.c  ../../../sys/array.c
SOURCES += ../module-common.c module.c $(ARCH_DIR)/$(ARCH)/watchdog.c

include config
include build.mk
include $(ROOT_PATH)/arch/build.mk

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

STATIC_LIBS = ../../../net/libnet-avr.a
OBJ = $(SOURCES:.c=.o)

# 1mhz (internal clk/8)
# FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m

# 8mhz (no internal clk/8)
# FUSES = -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m

# 16mhz external crystal
FUSES = -U lfuse:w:0xee:m -U hfuse:w:0xd9:m -U efuse:w:0xfd:m

# preserve eeprom: hfuse:w:0xd1:m
# BOD 2.7V: efuse:w:0xfd:m

CFLAGS += -I../../.. -I.

all: libnet $(SOURCES) $(EXE)
include ../../../common.mk

libnet:
	make -C ../../../net

include ../../../arch/avr/common.mk
.PHONY: all

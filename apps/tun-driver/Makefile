EXE = tun-driver
CFLAGS = -Wall -Werror -O0 -fno-inline -g
SOURCES = tun-driver.c tun.c

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

include config

include ../../arch/build.mk
include ../../common.mk
include build.mk

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

NET_APPS_DIR=../../net-apps
ARCH_DIR=../../arch
include $(NET_APPS_DIR)/build.mk

CFLAGS += -I../..

LIBS = -L../../net -lnet-x86
STATIC_LIBS = ../../net/libnet-x86.a
SOURCES += ../../sys/errno.c ../../scheduler.c
OBJ = $(SOURCES:.c=.o)

all: revision libnet $(SOURCES) $(EXE)

libnet:
	make -C ../../net

$(EXE): revision $(OBJ) $(STATIC_LIBS)
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

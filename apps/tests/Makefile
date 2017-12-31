EXE = tests
CFLAGS = -Wall -Werror -Os -g
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

NET_APPS_DIR=../../net_apps
ARCH_DIR=../../arch
include $(NET_APPS_DIR)/build.mk

CFLAGS += -I../..

LIBS = -L../../net -lnet-x86
STATIC_LIBS = ../../net/libnet-x86.a
SOURCES += ../../sys/errno.c
OBJ = $(SOURCES:.c=.o)

all: libnet $(SOURCES) $(EXE)

libnet:
	make -C ../../net

$(EXE): $(OBJ)
	$(CC)  $(OBJ) $(LIBS) $(LDFLAGS) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean:
	make -C ../../net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(ARCH_DIR)/$(ARCH)/*.o

run: all
	LD_LIBRARY_PATH=../../net ./tests

.PHONY: all
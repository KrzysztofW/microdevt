ifeq ($(CFG_FILE),)
CFLAGS = -Wall -Werror -Os -g
include default_config
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

OBJ = $(SOURCES:.c=.o)
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

CHKSUM_TEST_OBJ = chksum_test.o ../sys/chksum.o
chksum_test: $(CHKSUM_TEST_OBJ)
	$(CC) $(CHKSUM_TEST_OBJ) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean:
	@rm -f $(LIBNAME).so $(LIBNAME).a *~ "#*#" $(OBJ) $(CHKSUM_TEST_OBJ) chksum_test

.PHONY: all

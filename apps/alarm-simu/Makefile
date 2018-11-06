# ../simulavr/src/simulavr  -f alarm -d atmega328 -F 16000000

EXE = alarm
# CFLAGS = -Wall -Werror -Os -g -c -DSWEN_L3_DEBUG -DRF_DEBUG
CFLAGS = -Wall -Werror -Os -g -c -DRF_DEBUG

# definition to fake the simulator
CFLAGS += -DUSART0_RX_vect=USART_RX_vect -DDDRF=DDRD -DPORTF=PORTD -DPF1=PD1

SOURCES = main.c ../../scheduler.c
SOURCES += ../alarm/alarm-module1/module.c ../../sys/array.c

include config
include build.mk

ARCH_DIR=../../arch
include ../../arch/build.mk

export CFG_FILE = $(PWD)/config
export DEBUG
export CFLAGS

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

NET_APPS_DIR=../../net-apps

include $(NET_APPS_DIR)/build.mk

CFLAGS += -I../.. -I.

STATIC_LIBS = ../../net/libnet-avr.a
OBJ = $(SOURCES:.c=.o)

all: libnet $(SOURCES) $(EXE)

libnet:
	make -C ../../net

$(EXE): $(OBJ) $(STATIC_LIBS) config
	#$(AR) -cr $(EXE).a $(OBJ)
	#ranlib $(EXE).a
	#$(CC) $(EXE).a $(STATIC_LIBS) $(LDFLAGS) -o $@
	$(CC) $(OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean:
	make -C ../../net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(EXE).hex $(EXE).srec
	@rm -f $(ARCH_DIR)/$(ARCH)/*.o $(EXE).a

run: all
	$(CONFIG_AVR_SIMU_PATH)/src/simulavr  -f $(EXE) -d $(CONFIG_AVR_SIMU_MCU) -F $(CONFIG_AVR_F_CPU)

size: all
	avr-size -C --mcu=${CONFIG_AVR_MCU} $(EXE)

nm: all
	avr-nm -S --size-sort -t d $(EXE)

.PHONY: all

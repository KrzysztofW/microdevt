EXE = alarm
CFLAGS = -Wall -Werror -Os -g -c

SOURCES = main.c ../../scheduler.c  ../../sys/array.c
SOURCES += module-common.c module.c $(ARCH_DIR)/$(ARCH)/watchdog.c

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


NET_APPS_DIR = ../../net-apps

include $(NET_APPS_DIR)/build.mk

CFLAGS += -I../.. -I.

STATIC_LIBS = ../../net/libnet-avr.a
OBJ = $(SOURCES:.c=.o)

all: libnet $(SOURCES) $(EXE)
include ../../common.mk

libnet:
	make -C ../../net

$(EXE): $(OBJ) $(STATIC_LIBS) config
	$(AR) -cr $(EXE).a $(OBJ)
	avr-ranlib $(EXE).a
	$(CC) $(EXE).a $(STATIC_LIBS) $(EXE).a $(LDFLAGS) -o $@

%.c:
	$(CC) $(CFLAGS) $*.c

clean: clean_common
	make -C ../../net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(EXE).hex $(EXE).srec
	@rm -f $(ARCH_DIR)/$(ARCH)/*.o $(EXE).a

run_simu: all
	$(CONFIG_AVR_SIMU_PATH)/src/simulavr  -f $(EXE) -d $(CONFIG_AVR_SIMU_MCU) -F $(CONFIG_AVR_F_CPU)

upload: all
	avr-size $(EXE)
	avr-objcopy -j .text -j .data -O ihex $(EXE) $(EXE).hex
	avr-objcopy -O srec $(EXE) $(EXE).srec
	sudo avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U flash:w:$(EXE).hex

size: all
	avr-size -C --mcu=${CONFIG_AVR_MCU} $(EXE)

nm: all
	avr-nm -S --size-sort -t d $(EXE)

reset:
	sudo avrdude -c usbtiny -p ${CONFIG_AVR_BMCU}

read_fuses:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h

# FUSES see http://www.engbedded.com/fusecalc

# 8MHZ (no internal clk/8)
#write_fuses:
#	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m

# 1MHZ (internal clk/8)
#write_fuses:
#	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m

# 16MHZ external crystal
# preserve eeprom: hfuse:w:0x91:m
# BOD 2.7V: efuse:w:0xfd:m
write_fuses:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:w:0xee:m -U hfuse:w:0x99:m -U efuse:w:0xfd:m

erase:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -e -B128

.PHONY: all

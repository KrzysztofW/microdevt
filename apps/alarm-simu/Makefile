# ../simulavr/src/simulavr  -f alarm -d atmega328 -F 16000000

EXE = alarm
CFLAGS = -Wall -Werror -Os -g -c

# definition to fake the simulator
CFLAGS += -DUSART0_RX_vect=USART_RX_vect -DDDRF=DDRD -DPORTF=PORTD -DPF1=PD1

SOURCES = alarm.c $(ARCH_DIR)/$(ARCH)/enc28j60.c $(ARCH_DIR)/$(ARCH)/adc.c
SOURCES += ../../crypto/xtea.c ../../scheduler.c fsm.c

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

upload: all
	avr-size $(EXE)
	avr-objcopy -j .text -j .data -O ihex $(EXE) $(EXE).hex
	avr-objcopy -O srec $(EXE) $(EXE).srec
	sudo avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U flash:w:$(EXE).hex

size: all
	avr-size -C --mcu=${CONFIG_AVR_MCU} $(EXE)

nm: all
	avr-nm -S --size-sort -t d $(EXE)

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
write_fuses:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:w:0xee:m -U hfuse:w:0x99:m -U efuse:w:0xff:m

erase:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -e -B128

.PHONY: all

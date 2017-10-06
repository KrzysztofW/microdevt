#LDFLAGS = -W -Wl,--as-needed
CFLAGS = -Wall -Werror -Os -g -c

COMMON =
SOURCES =
include config

include net/build.mk
include build.mk

SOURCES += $(COMMON)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@if [ -f tun-driver ]; then sudo setcap cap_net_raw,cap_net_admin=eip tun-driver; fi

.c.o: ring.h
	$(CC) $(CFLAGS) $< -o $@

upload: all
	avr-size $(EXECUTABLE)
	avr-objcopy -j .text -j .data -O ihex $(EXECUTABLE) $(EXECUTABLE).hex
	avr-objcopy -O srec $(EXECUTABLE) $(EXECUTABLE).srec
	sudo avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U flash:w:$(EXECUTABLE).hex

size: all
	#avr-nm -S --size-sort -t d <obj-file>
	avr-size -C --mcu=${CONFIG_AVR_MCU} alarm


clean:
	@rm -f *.o net/*.o sys/*.o *.pdf *.hex *.srec *.elf *~ tests alarm tun-driver

#pdf: README.rst
#	rst2pdf $< > $(<:.rst=.pdf)

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
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:w:0xee:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m 

erase:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -e -B128

MCU = atmega328p
BMCU = m328p
F_CPU = 16000000
NET_SRC = net/eth.c net/if.c net/arp.c net/ip.c net/icmp.c
SOURCES = alarm.c usart0.c timer.c enc28j60.c rf.c adc.c $(NET_SRC)

TEST_SOURCES = timer.c tests.c net/tests.c ${NET_SRC}

LDFLAGS += -W
CFLAGS = -Wall -Werror -Os -g -c $(LDFLAGS)

ifeq ($(TEST), 1)
	CC = gcc
	EXECUTABLE = tests
	SOURCES = ${TEST_SOURCES}
	OBJECTS = $(SOURCES:.c=.o)
	CFLAGS += -DTEST -O0
else
	CC = avr-gcc
	EXECUTABLE = alarm
	OBJECTS = $(SOURCES:.c=.o)
	LDFLAGS = -DF_CPU=${F_CPU} -mmcu=${MCU}
	CFLAGS += -DF_CPU=$(F_CPU)
	CFLAGS += -Wno-deprecated-declarations -D__PROG_TYPES_COMPAT__
endif

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o: ring.h
	$(CC) $(CFLAGS) $< -o $@

upload: all
	avr-size $(EXECUTABLE)
	avr-objcopy -j .text -j .data -O ihex $(EXECUTABLE) $(EXECUTABLE).hex
	avr-objcopy -O srec $(EXECUTABLE) $(EXECUTABLE).srec
	sudo avrdude -V -c usbtiny -p ${BMCU} -U flash:w:$(EXECUTABLE).hex

clean:
	@rm -f *.o net/*.o *.pdf *.hex *.srec *.elf *~ tests alarm

#pdf: README.rst
#	rst2pdf $< > $(<:.rst=.pdf)



read_fuses:
	sudo avrdude -p ${BMCU} -c usbtiny -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h

# FUSES see http://www.engbedded.com/fusecalc

# 8MHZ (no internal clk/8)
#write_fuses:
#	sudo avrdude -p ${BMCU} -c usbtiny -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m 

# 1MHZ (internal clk/8)
#write_fuses:
#	sudo avrdude -p ${BMCU} -c usbtiny -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m 

# 16MHZ external crystal
write_fuses:
	sudo avrdude -p ${BMCU} -c usbtiny -U lfuse:w:0xee:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m 

erase:
	sudo avrdude -p ${BMCU} -c usbtiny -e -B128

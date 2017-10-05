LDFLAGS = -W
CFLAGS = -Wall -Werror -Os -g -c $(LDFLAGS)

COMMON =
SOURCES = net_apps.c timer.c enc28j60.c adc.c
include config

ifeq "$(or $(TUN), $(TEST))" "1"
	CONFIG_ARCH = X86_TUN_TAP
endif
include net/build.mk
include build.mk

SOURCES += $(NET_SRC) $(COMMON)

TEST_SOURCES = timer.c tests.c net/tests.c ${NET_SRC} $(COMMON)
# sys/hash-tables.c might be already in NET_SRC
TEST_SOURCES := $(filter-out sys/hash-tables.c, $(TEST_SOURCES))
TEST_SOURCES += sys/hash-tables.c

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
	SOURCES += usart0.c
else
	CFLAGS += -DNDEBUG
endif

ifeq ($(TUN), 1)
ifndef CONFIG_TCP
$(error need CONFIG_TCP to compile the tun-driver)
endif
	CC = gcc
	EXECUTABLE = tun-driver
	TEST_SOURCES := $(filter-out tests.c, $(TEST_SOURCES))
	TEST_SOURCES := $(filter-out net/tests.c, $(TEST_SOURCES))
	SOURCES = net/tun-driver.c net_apps.c ${TEST_SOURCES}
	OBJECTS = $(SOURCES:.c=.o)
	CFLAGS += -DTEST -DTUN -O0
	LDFLAGS += -lcap
else ifeq ($(TEST), 1)
	CC = gcc
	EXECUTABLE = tests
	SOURCES = ${TEST_SOURCES}
	OBJECTS = $(SOURCES:.c=.o)
	CFLAGS += -DTEST -O0
else
	CC = avr-gcc
	EXECUTABLE = alarm
	OBJECTS = $(SOURCES:.c=.o)
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

MCU = atmega328p
BMCU = m328p
F_CPU = 16000000

CC = avr-gcc
LDFLAGS = -Wall -W -g -DF_CPU=${F_CPU} -mmcu=${MCU} -Os
CFLAGS = -c $(LDFLAGS) -DF_CPU=$(F_CPU)
SOURCES = alarm.c usart0.c sound.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = alarm


all: $(SOURCES) $(EXECUTABLE)
	avr-size $(EXECUTABLE)
	avr-objcopy -j .text -j .data -O ihex $(EXECUTABLE) $(EXECUTABLE).hex


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

upload: all
	avr-objcopy -O srec $(EXECUTABLE) $(EXECUTABLE).srec
	sudo avrdude -c usbtiny -p ${BMCU} -U flash:w:$(EXECUTABLE).hex

clean:
	@rm -f *.o ${EXECUTABLE} *.pdf *.hex *.srec *.elf *~

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

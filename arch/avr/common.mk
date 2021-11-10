#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2017, Krzysztof Witek
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# The full GNU General Public License is included in this distribution in
# the file called "LICENSE".
#

upload: all
	avr-size $(EXE)
	avr-objcopy -j .text -j .data -O ihex $(EXE) $(EXE).hex
	avr-objcopy -O srec $(EXE) $(EXE).srec
	avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U flash:w:$(EXE).hex

size: all
	avr-size -C --mcu=${CONFIG_AVR_MCU} $(EXE)

nm: all
	avr-nm -S --size-sort -t d $(EXE)

reset:
	avrdude -c usbtiny -p ${CONFIG_AVR_BMCU}

# FUSES see http://www.engbedded.com/fusecalc
read_fuses:
	avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h
write_fuses:
	avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny $(FUSES)

read_eeprom:
	avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U eeprom:r:$(file_name):r

write_eeprom:
	avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U eeprom:w:$(file_name):r

verify_eeprom:
	avrdude -V -c usbtiny -p ${CONFIG_AVR_BMCU} -U eeprom:v:$(file_name):r

erase:
	avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -e -B128

run_simu: all_not_size_optimized
	$(CONFIG_AVR_SIMU_PATH)/src/simulavr -f $(EXE) -d $(CONFIG_AVR_SIMU_MCU) -F $(CONFIG_AVR_F_CPU)

# not supported by the simulator
$(EXE): $(OBJ) $(STATIC_LIBS) config
	$(AR) -cr $(EXE).a $(OBJ)
	avr-ranlib $(EXE).a
	$(CC) $(EXE).a $(STATIC_LIBS) $(EXE).a $(LDFLAGS) -o $@

# supported by the simulator
$(EXE)_not_size_optimized: $(OBJ) $(STATIC_LIBS) config
	$(CC) $(OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(EXE)

%.c:
	$(CC) $(CFLAGS) $*.c

clean: clean_common
	make -C $(ROOT_PATH)/net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(EXE).hex $(EXE).srec
	@rm -f $(ARCH_DIR)/$(ARCH)/*.o $(EXE).a

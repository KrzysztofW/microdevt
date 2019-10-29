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

# FUSES see http://www.engbedded.com/fusecalc
read_fuses:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h
write_fuses:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny $(FUSES)

erase:
	sudo avrdude -p ${CONFIG_AVR_BMCU} -c usbtiny -e -B128

run_simu: all
	$(CONFIG_AVR_SIMU_PATH)/src/simulavr -f $(EXE) -d $(CONFIG_AVR_SIMU_MCU) -F $(CONFIG_AVR_F_CPU)

# not supported by the simulator
$(EXE): $(OBJ) $(STATIC_LIBS) config
	$(AR) -cr $(EXE).a $(OBJ)
	avr-ranlib $(EXE).a
	$(CC) $(EXE).a $(STATIC_LIBS) $(EXE).a $(LDFLAGS) -o $@

# supported by the simulator
$(EXE)_not_optimized: $(OBJ) $(STATIC_LIBS) config
	$(CC) $(OBJ) $(STATIC_LIBS) $(LDFLAGS) -o $(EXE)

%.c:
	$(CC) $(CFLAGS) $*.c

clean: clean_common
	make -C $(ROOT_PATH)/net clean
	@rm -f $(EXE) *~ "#*#" $(OBJ) $(EXE).hex $(EXE).srec
	@rm -f $(ARCH_DIR)/$(ARCH)/*.o $(EXE).a

/* simulavr/src/simulavr  -f <elf> -d atmega328 -F 16000000 */
#define SIMU_INFO_FILE src/simulavr_info.h
#define __incl_header(x) #x
#define incl_header(x) __incl_header(x)
#include incl_header(CONFIG_AVR_SIMU_PATH/SIMU_INFO_FILE)

SIMINFO_DEVICE("atmega328");
SIMINFO_CPUFREQUENCY(F_CPU);
SIMINFO_SERIAL_IN("D0", "-", CONFIG_SERIAL_SPEED);
SIMINFO_SERIAL_OUT("D1", "-", CONFIG_SERIAL_SPEED);


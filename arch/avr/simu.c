/* simulavr/src/simulavr  -f <elf> -d atmega328 -F 16000000 */
#include <sys/utils.h>
#define SIMU_INFO_FILE src/simulavr_info.h
#define incl_header(x) __STR(x)
#include incl_header(CONFIG_AVR_SIMU_PATH/SIMU_INFO_FILE)

SIMINFO_DEVICE(__STR(CONFIG_AVR_SIMU_MCU));
SIMINFO_CPUFREQUENCY(F_CPU);
SIMINFO_SERIAL_IN("D0", "-", CONFIG_USART0_SPEED);
SIMINFO_SERIAL_OUT("D1", "-", CONFIG_USART0_SPEED);


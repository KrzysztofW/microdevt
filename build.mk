# Do not use a too small timer resolution on x86 in order
# not to have a busy CPU.
ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ifeq ($(shell test $(CONFIG_TIMER_RESOLUTION_US) -lt 1000; echo $$?),0)
CFLAGS += -DCONFIG_TIMER_RESOLUTION_US=1000
else
CFLAGS += -DCONFIG_TIMER_RESOLUTION_US=$(CONFIG_TIMER_RESOLUTION_US)
endif
else
CFLAGS += -DCONFIG_TIMER_RESOLUTION_US=$(CONFIG_TIMER_RESOLUTION_US)
endif

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

CFLAGS += -DCONFIG_SERIAL_SPEED=$(CONFIG_SERIAL_SPEED)
CFLAGS += -Isys -Inet

ifeq ($(CONFIG_ARCH),AVR)
CC = avr-gcc
EXECUTABLE = alarm
SOURCES += alarm.c net_apps.c timer.c arch/avr/timer.c arch/avr/enc28j60.c
SOURCES += arch/avr/adc.c

ifeq ($(DEBUG), 1)
	SOURCES += arch/avr/_stdio.c
	SOURCES += arch/avr/usart0.c
endif
AVR_FLAGS = -DF_CPU=${CONFIG_AVR_F_CPU} -mmcu=${CONFIG_AVR_MCU} -Iarch/avr
AVR_FLAGS += -DF_CPU=$(CONFIG_AVR_F_CPU) -DCONFIG_AVR_MCU
AVR_FLAGS += -DCONFIG_AVR_F_CPU=$(CONFIG_AVR_F_CPU)
AVR_FLAGS += -Wno-deprecated-declarations -D__PROG_TYPES_COMPAT__
CFLAGS += $(AVR_FLAGS)
LDFLAGS += $(AVR_FLAGS)
OBJECTS = $(SOURCES:.c=.o)

ifdef CONFIG_RF
SOURCES += rf.c
CFLAGS += -DCONFIG_RF
endif

ifdef CONFIG_AVR_SIMU
SOURCES += arch/avr/simu.c
CFLAGS += -DCONFIG_AVR_SIMU -DCONFIG_AVR_SIMU_PATH=$(CONFIG_AVR_SIMU_PATH)
endif

else ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ifdef CONFIG_USE_CAPABILITIES
CFLAGS += -DCONFIG_USE_CAPABILITIES
endif
ifndef CONFIG_TCP
$(error need CONFIG_TCP to compile the tun-driver)
endif
CC = gcc
EXECUTABLE = tun-driver
SOURCES := $(filter-out tests.c, $(SOURCES))
SOURCES := $(filter-out net/tests.c, $(SOURCES))
SOURCES += net/tun-driver.c net_apps.c timer.c arch/x86/timer.c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS := $(filter-out -Isys, $(CFLAGS))
CFLAGS += -O0 -DX86 -Iarch/x86
LDFLAGS += -lcap
else ifeq ($(CONFIG_ARCH),X86_TEST)
CC = gcc
EXECUTABLE = tests
SOURCES += tests.c net/tests.c $(COMMON) timer.c arch/x86/timer.c
# sys/hash-tables.c might be already in SOURCES
SOURCES := $(filter-out sys/hash-tables.c, $(SOURCES))
SOURCES += sys/hash-tables.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -O0 -DTEST -DX86 -Iarch/x86
endif

ifdef NEED_ERRNO
COMMON += sys/errno.c
endif

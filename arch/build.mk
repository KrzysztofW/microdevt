ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ARCH = x86

else ifeq ($(CONFIG_ARCH),X86_TEST)

ARCH = x86

else ifeq ($(CONFIG_ARCH),AVR)
ifeq ($(DEBUG), 1)
	SOURCES += $(ARCH_DIR)/avr/_stdio.c
	SOURCES += $(ARCH_DIR)/avr/usart.c
endif

AVR_FLAGS = -DF_CPU=${CONFIG_AVR_F_CPU} -mmcu=${CONFIG_AVR_MCU}
AVR_FLAGS += -DF_CPU=$(CONFIG_AVR_F_CPU) -DCONFIG_AVR_MCU
AVR_FLAGS += -DCONFIG_AVR_F_CPU=$(CONFIG_AVR_F_CPU)
CFLAGS += $(AVR_FLAGS)
LDFLAGS += $(AVR_FLAGS)

ARCH = avr
CC = avr-gcc
AR = avr-ar
endif

ifeq ($(ARCH),x86)
CFLAGS += -DX86
CC = gcc
AR = ar
endif

CFLAGS += -I$(ARCH_DIR)/$(ARCH)

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

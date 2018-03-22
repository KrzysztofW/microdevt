ifdef CONFIG_USART0
CFLAGS += -DCONFIG_USART0
ifdef CONFIG_USART0_SPEED
CFLAGS += -DCONFIG_USART0_SPEED=$(CONFIG_USART0_SPEED)
else
CFLAGS += -DCONFIG_USART0_SPEED=57600
endif
endif

ifdef CONFIG_USART1
CFLAGS += -DCONFIG_USART1
ifdef CONFIG_USART1_SPEED
CFLAGS += -DCONFIG_USART1_SPEED=$(CONFIG_USART1_SPEED)
else
CFLAGS += -DCONFIG_USART1_SPEED=57600
endif
endif

ifeq "$(or $(CONFIG_RF_RECEIVER), $(CONFIG_RF_SENDER))" "y"
SOURCES += ../../../drivers/rf.c ../../../sys/chksum.c
endif
ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
ifdef CONFIG_RF_GENERIC_COMMANDS
CFLAGS += -DCONFIG_RF_GENERIC_COMMANDS
endif
endif
ifdef CONFIG_RF_SENDER
CFLAGS += -DCONFIG_RF_SENDER
endif

ifdef CONFIG_AVR_SIMU
SOURCES += $(ARCH_DIR)/$(ARCH)/simu.c
CFLAGS += -DCONFIG_AVR_SIMU -DCONFIG_AVR_SIMU_PATH=$(CONFIG_AVR_SIMU_PATH)
endif

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

ifdef CONFIG_RING_STATIC_ALLOC
CFLAGS += -DCONFIG_RING_STATIC_ALLOC=$(CONFIG_RING_STATIC_ALLOC)
ifdef CONFIG_RING_STATIC_ALLOC_DATA_SIZE
CFLAGS += -DCONFIG_RING_STATIC_ALLOC_DATA_SIZE=$(CONFIG_RING_STATIC_ALLOC_DATA_SIZE)
endif
endif

ifdef CONFIG_RF_STATIC_ALLOC
CFLAGS += -DCONFIG_RF_STATIC_ALLOC=$(CONFIG_RF_STATIC_ALLOC)
endif

ifdef CONFIG_SWEN_STATIC_ALLOC
CFLAGS += -DCONFIG_SWEN_STATIC_ALLOC=$(CONFIG_SWEN_STATIC_ALLOC)
endif
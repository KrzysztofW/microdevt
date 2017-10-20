#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_AVR_MCU
#include <avr/io.h>
#else
#include <stdint.h>
#endif
#include <stdio.h>

#include "../sys/buf.h"
#include "../sys/utils.h"
#include "pkt-mempool.h"
#include "proto_defs.h"
#include "if.h"
#if defined(CONFIG_BSD_COMPAT) && CONFIG_ARCH != X86_TUN_TAP
#include "../sys/errno.h"
#endif

#endif

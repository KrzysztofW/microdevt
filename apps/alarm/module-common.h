#ifndef _MODULE_COMMON_H_
#define _MODULE_COMMON_H_
#include <stdint.h>
#include <net/if.h>
#include <drivers/rf.h>
#include "rf-common.h"

extern const uint32_t rf_enc_defkey[4];

static inline uint8_t module_to_addr(uint8_t module)
{
	return RF_MOD0_HW_ADDR + module;
}

static inline uint8_t addr_to_module(uint8_t addr)
{
	return addr - RF_MOD0_HW_ADDR;
}

void module_init_iface(iface_t *iface, uint8_t *addr);

#endif

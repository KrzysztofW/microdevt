#ifndef _MODULE_COMMON_H_
#define _MODULE_COMMON_H_
#include <stdint.h>
#include <net/if.h>
#include <drivers/rf.h>
#include <net/swen-l3.h>
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

void module_init_op_queues(void);
void module_add_op(uint8_t op, uint8_t urgent);
int module_get_op(uint8_t *op);
void module_skip_op(void);
int
send_rf_msg(swen_l3_assoc_t *assoc, uint8_t cmd, const void *data, int len);

#endif

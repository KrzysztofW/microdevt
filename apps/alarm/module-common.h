#ifndef _MODULE_COMMON_H_
#define _MODULE_COMMON_H_
#include <stdint.h>
#include <net/if.h>
#include <drivers/rf.h>
#include <net/swen-l3.h>

#define RF_BURST_NUMBER 1

#define RF_MASTER_MOD_HW_ADDR 0x01
#define RF_GENERIC_MOD_HW_ADDR 0x02
#define RF_INIT_ADDR 0xFE

enum features {
	MODULE_FEATURE_HUMIDITY =       1,
	MODULE_FEATURE_TEMPERATURE =   (1 << 1),
	MODULE_FEATURE_FAN =           (1 << 2),
	MODULE_FEATURE_SIREN =         (1 << 3),
	MODULE_FEATURE_LAN =           (1 << 4),
	MODULE_FEATURE_RF =            (1 << 5),
	MODULE_FEATURE_ROLLING_CODES = (1 << 6),
};
typedef enum __attribute__ ((__packed__)) features features_t;

extern const uint32_t rf_enc_defkey[4];

static inline uint8_t module_id_to_addr(uint8_t id)
{
	return RF_MASTER_MOD_HW_ADDR + id;
}

static inline uint8_t addr_to_module_id(uint8_t addr)
{
	return addr - RF_MASTER_MOD_HW_ADDR;
}

void module_init_iface(iface_t *iface, uint8_t *addr);
int module_check_magic(void);
void module_update_magic(void);

void module_add_op(uint8_t op, uint8_t urgent);
int __module_add_op(ring_t *queue, uint8_t op);
int module_get_op(uint8_t *op);
int __module_get_op(ring_t *queue, uint8_t *op);
void module_skip_op(void);
void __module_skip_op(ring_t *queue);
static inline int __module_op_pending(ring_t *queue)
{
	return !ring_is_empty(queue);
}
void module_reset_op_queues(void);
void __module_reset_op_queue(ring_t *queue);

int
send_rf_msg(swen_l3_assoc_t *assoc, uint8_t cmd, const void *data, int len);

#endif

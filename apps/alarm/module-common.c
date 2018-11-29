#include <eeprom.h>
#include "module-common.h"

const uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

#define MAGIC 0xA2
static uint8_t EEMEM eeprom_magic;

static struct iface_queues {
	RING_DECL_IN_STRUCT(pkt_pool, CONFIG_PKT_DRIVER_NB_MAX);
	RING_DECL_IN_STRUCT(rx, CONFIG_PKT_NB_MAX);
	RING_DECL_IN_STRUCT(tx, CONFIG_PKT_NB_MAX);
} iface_queues = {
	.pkt_pool = RING_INIT(iface_queues.pkt_pool),
	.rx = RING_INIT(iface_queues.rx),
	.tx = RING_INIT(iface_queues.tx),
};
static rf_ctx_t ctx;

void module_init_iface(iface_t *iface, uint8_t *addr)
{
	iface->hw_addr = addr;
#ifdef CONFIG_RF_RECEIVER
	iface->recv = rf_input;
#endif
#ifdef CONFIG_RF_SENDER
	iface->send = rf_output;
#endif
	iface->flags = IF_UP|IF_RUNNING|IF_NOARP;
	if_init(iface, IF_TYPE_RF, &iface_queues.pkt_pool, &iface_queues.rx,
		&iface_queues.tx, 1);
	rf_init(iface, &ctx, 1);
}

int module_check_magic(void)
{
	uint8_t magic;

	eeprom_load(&magic, &eeprom_magic, sizeof(magic));
	return magic == MAGIC;
}

int8_t module_update_magic(void)
{
	uint8_t magic = MAGIC;

	return eeprom_update_and_check(&eeprom_magic, &magic, sizeof(magic));
}

int module_add_op(ring_t *queue, uint8_t op, event_t *event)
{
	uint8_t cur_op;
	int rlen = ring_len(queue);
	uint8_t i;

	for (i = 0; i < rlen; i++) {
		__ring_getc_at(queue, &cur_op, i);
		if (cur_op == op)
			return 0;
	}
	if (ring_addc(queue, op) < 0)
		return -1;
	event_set_mask(event, EV_READ | EV_WRITE);
	return 0;
}

int module_get_op(ring_t *queue, uint8_t *op)
{
	if (ring_is_empty(queue))
		return -1;
	__ring_getc_at(queue, op, 0);
	return 0;
}

void module_skip_op(ring_t *queue)
{
	if (!ring_is_empty(queue))
		__ring_skip(queue, 1);
}

int module_get_op2(uint8_t *op, ring_t *q1, ring_t *q2)
{
	if (module_get_op(q1, op) < 0)
		return module_get_op(q2, op);
	return 0;
}

void module_skip_op2(ring_t *q1, ring_t *q2)
{
	ring_t *ring = module_op_pending(q1) ? q1 : q2;

	module_skip_op(ring);
}

int
send_rf_msg(swen_l3_assoc_t *assoc, uint8_t cmd, const void *data, int len)
{
	buf_t buf = BUF(sizeof(uint8_t) + len);
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	if (data)
		__buf_add(&buf, data, len);
	sbuf = buf2sbuf(&buf);
	return swen_l3_send(assoc, &sbuf);
}

void module_set_default_cfg(module_cfg_t *cfg)
{
	memset(cfg, 0, sizeof(module_cfg_t));
	cfg->state = MODULE_STATE_DISABLED;
	cfg->sensor.humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
	cfg->siren_duration = DEFAULT_SIREN_ON_DURATION;
	cfg->siren_timeout = DEFAULT_SIREN_ON_TIMEOUT;
}

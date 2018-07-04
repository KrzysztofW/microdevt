#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <net/swen.h>
#include <net/swen-cmds.h>
#include <net/event.h>
#include "../rf-common.h"

static uint8_t rf_addr = RF_MOD1_HW_ADDR;

static iface_t eth1 = {
	.flags = IF_UP|IF_RUNNING|IF_NOARP,
	.hw_addr = &rf_addr,
#ifdef CONFIG_RF_RECEIVER
	.recv = &rf_input,
#endif
#ifdef CONFIG_RF_SENDER
	.send = &rf_output,
#endif
};

static uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	sbuf_t sbuf;
	const char *s = "Hello world!";
	static uint16_t seq;

	if (buf_addf(&buf, "%s %u", s, seq) < 0) {
		DEBUG_LOG("%s:%d\n", __func__, __LINE__);
		return;
	}

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("can't encode buf\n");
	sbuf = buf2sbuf(&buf);
	if (swen_sendto(&eth1, RF_MOD0_HW_ADDR, &sbuf) < 0)
		DEBUG_LOG("failed sending RF msg\n");
	timer_reschedule(timer, 5000000UL);
	seq++;
}
#endif

void send_rf_cmd(buf_t *buf)
{
	sbuf_t sbuf;

	if (xtea_encode(buf, rf_enc_defkey) < 0) {
		DEBUG_LOG("can't encode buf\n");
		return;
	}
	sbuf = buf2sbuf(buf);
	if (swen_sendto(&eth1, RF_MOD0_HW_ADDR, &sbuf) < 0)
		DEBUG_LOG("failed sending humidity value\n");
}
#ifdef CONFIG_RF_RECEIVER
static void
rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		sbuf_t sbuf;

		if (xtea_decode(buf, rf_enc_defkey) < 0) {
			DEBUG_LOG("cannot decode buf\n");
			return;
		}
		DEBUG_LOG("got from 0x%X: %s\n", from, buf_data(buf));

		/* send it back */
		if (xtea_encode(buf, rf_enc_defkey) < 0) {
			DEBUG_LOG("can't encode buf\n");
			return;
		}
		sbuf = buf2sbuf(buf);
		if (swen_sendto(&eth1, RF_MOD0_HW_ADDR, &sbuf) < 0)
			DEBUG_LOG("failed sending RF msg\n");
	}
}

#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
	if (nb == 0)
		PORTD |= 1 << PD3;
	else if (nb == 1)
		PORTD &= ~(1 << PD3);
	else if (nb == 2)
		PORTB |= 1 << PB2;
	else if (nb == 3)
		PORTB &= ~(1 << PB2);
}
#endif
#endif

void alarm_module1_rf_init(void)
{
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	if_init(&eth1, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&eth1, 3);
#ifdef CONFIG_RF_RECEIVER
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#if defined (CONFIG_RF_SENDER) && defined(CONFIG_RF_CHECKS)
	if (rf_checks(&eth1) < 0)
		__abort();
#endif
	timer_init(&timer_rf);
	timer_add(&timer_rf, 0, tim_rf_cb, &timer_rf);
}

#include <net/swen.h>
#include <net/swen-cmds.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <timer.h>
#include <net/socket.h>
#include "rf-common.h"

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
static uint8_t rf_addr = RF_MOD0_HW_ADDR;

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
#endif

#ifdef CONFIG_RF_SENDER
static uint8_t rf_wd;

static void send_rf_cmd(uint8_t incr)
{
	static uint16_t seq;
	buf_t buf = BUF(32);
	sbuf_t sbuf;
	const char *s = "I am your master!";

	buf_addf(&buf, "%s %u", s, seq);

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("cannot encode buf\n");
	sbuf = buf2sbuf(&buf);
	if (swen_sendto(&eth1, RF_MOD1_HW_ADDR, &sbuf) < 0)
		DEBUG_LOG("failed sending RF msg\n");
	if (incr)
		seq++;
}

extern ring_t *pkt_pool;
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;

	if (rf_wd == 0)
		send_rf_cmd(0);
	else
		rf_wd--;
	timer_reschedule(timer, 5000000UL);
	printf("eth1: pool:%d tx:%d rx:%d if_pool:%d\n",
	       ring_len(pkt_pool),
	       ring_len(eth1.tx), ring_len(eth1.rx), ring_len(eth1.pkt_pool));
}
#endif

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		if (xtea_decode(buf, rf_enc_defkey) < 0) {
			DEBUG_LOG("cannot decode buf\n");
			return;
		}
		rf_wd = 1;
		DEBUG_LOG("got from 0x%X: %s\n", from, buf_data(buf));
		send_rf_cmd(1);
	}
}
#endif

void alarm_rf_init(void)
{
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	if_init(&eth1, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&eth1, 1);
#ifdef CONFIG_RF_RECEIVER
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#ifdef CONFIG_RF_SENDER
	timer_init(&timer_rf);
	timer_add(&timer_rf, 3000000, tim_rf_cb, &timer_rf);
	/* port F used by the RF sender */
	DDRF = (1 << PF1);
#endif
#endif

}

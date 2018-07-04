#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <net/swen.h>
#include <net/swen-cmds.h>
#include <net/event.h>
#include <timer.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <drivers/gsm-at.h>
#include <scheduler.h>
#include "../rf-common.h"

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
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
#endif

#ifdef CONFIG_GSM_SIM900
static void gsm_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	DEBUG_LOG("gsm callback (status:%d)\n", status);
	if (status == GSM_STATUS_RECV) {
		DEBUG_LOG("from:");
		sbuf_print(from);
		DEBUG_LOG(" msg:<");
		sbuf_print(msg);
		DEBUG_LOG(">\n");
	}
}

ISR(USART_RX_vect)
{
	gsm_handle_interrupt(UDR0);
}

#endif

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	sbuf_t sbuf;
	const char *s = "Hello world!";

	__buf_adds(&buf, s, strlen(s));

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("can't encode buf\n");
	sbuf = buf2sbuf(&buf);
	if (swen_sendto(&eth1, RF_MOD0_HW_ADDR, &sbuf) < 0)
		DEBUG_LOG("failed sending RF msg\n");
	timer_reschedule(timer, 5000000UL);
}
#endif
void tim_led_cb(void *arg)
{
	tim_t *timer = arg;
#ifdef CONFIG_GSM_SIM900
	static uint8_t gsm;
#endif
	PORTD ^= 1 << PD4;
	timer_reschedule(timer, 1000000UL);
#ifdef CONFIG_GSM_SIM900
	if ((gsm % 60) == 0)
		gsm_send_sms("+33687236420", "SMS from KW alarm");
	gsm++;
#endif
}

#ifdef CONFIG_RF_RECEIVER
static void
rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		if (xtea_decode(buf, rf_enc_defkey) < 0) {
			DEBUG_LOG("cannot decode buf\n");
			return;
		}
		DEBUG_LOG("got from 0x%X: %s\n", from, buf_data(buf));
	}
}

#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#endif
INIT_ADC_DECL(c, DDRC, PORTC)

int main(void)
{
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
	tim_t timer_led;

	init_adc_c();
#ifdef DEBUG
#ifdef CONFIG_GSM_SIM900
	init_stream0(&stdout, &stdin, 1);
#else
	init_stream0(&stdout, &stdin, 0);
#endif
	DEBUG_LOG("KW alarm module 1\n");
#endif
	watchdog_shutdown();
	timer_subsystem_init();
	scheduler_init();

#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	timer_init(&timer_led);
	timer_add(&timer_led, 3000000, tim_led_cb, &timer_led);
	watchdog_enable();

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	if_init(&eth1, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&eth1, 1);
#endif
#ifdef CONFIG_RF_RECEIVER
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#ifdef CONFIG_RF_SENDER
#ifdef CONFIG_RF_CHECKS
	if (rf_checks(&eth1) < 0)
		__abort();
#endif
	timer_init(&timer_rf);
	timer_add(&timer_rf, 0, tim_rf_cb, &timer_rf);

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);
#endif
#ifdef CONFIG_GSM_SIM900
	gsm_init(stdin, stdout, gsm_cb);
#endif

	/* slow functions */
	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
#ifdef CONFIG_RF_SENDER
	/* rf_shutdown(); */
#endif
	return 0;
}

#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <net/swen.h>
#include <net/swen_cmds.h>
#include <timer.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include "../rf_common.h"

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
static uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};
void *rf_handle;
void *swen_handle;
#endif

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	const char *s = "Hello world!";

	__buf_adds(&buf, s);
	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("can't encode buf\n");
	if (swen_sendto(swen_handle, RF_MOD0_HW_ADDR, &buf) < 0)
		DEBUG_LOG("failed sending RF msg\n");

	timer_reschedule(timer, 5000000UL);
}
#endif
void tim_led_cb(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= 1 << PD4;
	timer_reschedule(timer, 1000000UL);
}

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#define RF_BUF_SIZE 64
uint8_t rf_buf_data[RF_BUF_SIZE];
#endif

int main(void)
{
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
	tim_t timer_led;
#ifdef CONFIG_RF_RECEIVER
	buf_t rf_buf;
	uint8_t rf_from;
#endif

	init_adc();
#ifdef DEBUG
	init_stream0(&stdout, &stdin);
	DEBUG_LOG("KW alarm module 1\n");
#endif
	watchdog_shutdown();
	timer_subsystem_init();

#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	timer_init(&timer_led);
	timer_add(&timer_led, 0, tim_led_cb, &timer_led);
	watchdog_enable();

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	rf_handle = rf_init(RF_BURST_NUMBER);
#endif
#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_handle = swen_init(rf_handle, RF_MOD1_HW_ADDR, rf_kerui_cb,
				rf_ke_cmds);
#else
	swen_handle = swen_init(rf_handle, RF_MOD1_HW_ADDR, NULL, NULL);
#endif
	if (swen_handle == NULL)
		return -1;
#endif

#ifdef CONFIG_RF_SENDER
	timer_init(&timer_rf);
	timer_add(&timer_rf, 0, tim_rf_cb, &timer_rf);

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);
#endif
#ifdef CONFIG_RF_RECEIVER
	buf_init(&rf_buf, rf_buf_data, RF_BUF_SIZE);
	rf_buf.len = 0;
#endif
	while (1) {
		/* slow functions */
#ifdef CONFIG_RF_RECEIVER
		/* TODO: this block should be put in a timer cb */
		if (swen_recvfrom(swen_handle, &rf_from, &rf_buf) >= 0
		    && buf_len(&rf_buf)) {
			if (xtea_decode(&rf_buf, rf_enc_defkey) < 0)
				DEBUG_LOG("can't decode buf\n");
			DEBUG_LOG("from: 0x%X: %s\n", rf_from, rf_buf.data);
			buf_reset(&rf_buf);
		}
#endif
		watchdog_reset();
	}
#ifdef CONFIG_RF_SENDER
	/* rf_shutdown(); */
#endif
	return 0;
}

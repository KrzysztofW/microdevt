#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <rf.h>
#include <timer.h>
#include <crypto/xtea.h>
#include "../rf_common.h"

static uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	const char *s = "Hello world!";

	__buf_adds(&buf, s);

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("can't encode buf\n");
	if (rf_sendto(RF_MOD0_HW_ADDR, &buf, 2) < 0)
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
#ifdef CONFIG_RF_KERUI_CMDS
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

#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
	if (rf_init() < 0) {
		DEBUG_LOG("can't initialize RF\n");
		return -1;
	}
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
#ifdef CONFIG_RF_KERUI_CMDS
	rf_set_kerui_cb(rf_kerui_cb);
#endif
#endif
	while (1) {
		/* slow functions */
#ifdef CONFIG_RF_RECEIVER
		/* TODO: this block should be put in a timer cb */
		if (rf_recvfrom(&rf_from, &rf_buf) >= 0 && buf_len(&rf_buf)) {
			(void)rf_enc_defkey;
			if (xtea_decode(&rf_buf, rf_enc_defkey) < 0)
				DEBUG_LOG("can't decode buf\n");
			DEBUG_LOG("from: 0x%X: %s\n", rf_from, rf_buf.data);
			buf_reset(&rf_buf);
		}
#endif
		watchdog_reset();
		delay_ms(1);
	}
#ifdef CONFIG_RF_SENDER
	/* rf_shutdown(); */
#endif
	return 0;
}

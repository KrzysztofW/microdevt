#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <net/swen.h>
#include <net/swen-cmds.h>
#include <net/event.h>
#include "../rf-common.h"
#include "../module.h"
#include "alarm-module1.h"

static uint8_t rf_addr = RF_MOD1_HW_ADDR;
iface_t rf_iface = {
	.flags = IF_UP|IF_RUNNING|IF_NOARP,
	.hw_addr = &rf_addr,
#ifdef CONFIG_RF_RECEIVER
	.recv = &rf_input,
#endif
#ifdef CONFIG_RF_SENDER
	.send = &rf_output,
#endif
};

uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	uint8_t cmd;

	DEBUG_LOG("received kerui cmd %d\n", nb);
	switch (nb) {
	case 0:
		cmd = CMD_ARM;
		break;
	case 1:
		cmd = CMD_DISARM;
		break;
	case 2:
		cmd = CMD_RUN_FAN;
		break;
	case 3:
		cmd = CMD_STOP_FAN;
		break;
	default:
		return;
	}
	handle_rf_commands(cmd, 0);
}
#endif
#endif

void alarm_module1_rf_init(void)
{
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	if_init(&rf_iface, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&rf_iface, 1);
#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#if defined (CONFIG_RF_SENDER) && defined(CONFIG_RF_CHECKS)
	if (rf_checks(&rf_iface) < 0)
		__abort();
#endif
}

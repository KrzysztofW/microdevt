#include <net/swen.h>
#include <net/swen-cmds.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <timer.h>
#include <net/socket.h>
#include "rf-common.h"

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
static uint8_t rf_addr = RF_MOD0_HW_ADDR;

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
#endif

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#endif

void alarm_rf_init(void)
{
#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	if_init(&rf_iface, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&rf_iface, 1);
#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#ifdef CONFIG_RF_SENDER
	/* port F used by the RF sender */
	DDRF = (1 << PF1);
#endif
#endif
}

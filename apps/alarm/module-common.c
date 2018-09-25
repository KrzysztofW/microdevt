#include "module-common.h"

const uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

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

	if_init(iface, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(iface, 1);
}

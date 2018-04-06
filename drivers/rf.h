#ifndef _RF_H_
#define _RF_H_
#include <sys/ring.h>
#include <net/if.h>

int rf_init(iface_t *iface, uint8_t burst);
void rf_shutdown(const iface_t *iface);
 void rf_input(const iface_t *iface);
int rf_output(const iface_t *iface, pkt_t *pkt);
int rf_checks(const iface_t *iface);
#endif

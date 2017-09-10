#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef TEST
#include <avr/io.h>
#else
#include <stdint.h>
#endif

#include "../sys/buf.h"
#include "../sys/utils.h"
#include "pkt-mempool.h"
#include "proto_defs.h"
#include "if.h"

/* #define CONFIG_IPV6 */
/* #define CONFIG_STATS */
/* #define CONFIG_PROMISC */
#define CONFIG_ARP_TABLE_SIZE 2
/* #define CONFIG_ARP_EXPIRY 10 */
/* #define CONFIG_DHCP */
/* #define CONFIG_IP_CHKSUM */
/* #define CONFIG_MORE_THAN_ONE_INTERFACE */

#endif

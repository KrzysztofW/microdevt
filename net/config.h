#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <avr/io.h>
#include "../sys/buf.h"
#include "../sys/utils.h"
#include "eth.h"
#include "if.h"

#define BYTE_ORDER LITTLE_ENDIAN

/* #define CONFIG_IPV6 */
/* #define CONFIG_STATS */
/* #define CONFIG_PROMISC */
#define CONFIG_ARP_TABLE_SIZE 4
#define CONFIG_ARP_EXPIRY 10
#define CONFIG_DHCP
#define CONFIG_IP_CHKSUM

#endif

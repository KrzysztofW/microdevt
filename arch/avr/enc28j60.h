#ifndef _ENC28J60_H_
#define _ENC28J60_H_

#include <sys/buf.h>
#include <net/pkt-mempool.h>

/* buffer boundaries applied to internal 8K ram
 * entire available packet buffer space is allocated
 */
/* start TX buffer at 0 */
#define TXSTART_INIT	0x0000

/* give TX buffer space for one full ethernet frame (~1500 bytes) */
#define RXSTART_INIT	0x0600

/* receive buffer gets the rest */
#define RXSTOP_INIT	0x1FFF

/* maximum ethernet frame length */
#define	MAX_FRAMELEN	1518

/* Ethernet constants */
#define ETHERNET_MIN_PACKET_LENGTH	0x3C

uint8_t enc28j60_read_op(uint8_t op, uint8_t address);
void enc28j60_write_op(uint8_t op, uint8_t address, uint8_t data);
void enc28j60_read_buffer(uint16_t len, buf_t *buf);
void enc28j60_write_buffer(uint16_t len, uint8_t* data);
uint8_t enc28j60_read(uint8_t address);
void enc28j60_write(uint8_t address, uint8_t data);
uint16_t enc28j60_phy_read(uint8_t address);
void enc28j60_phy_write(uint8_t address, uint16_t data);

void enc28j60_init(uint8_t *macaddr);

uint16_t enc28j60_pkt_send(const buf_t *out);
pkt_t *enc28j60_pkt_recv(void);

#endif

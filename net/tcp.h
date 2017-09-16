#ifndef _TCP_H_
#define _TCP_H_

#include "config.h"

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80

struct tcp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq;
	uint32_t ack;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t  reserved : 4;
	uint8_t  hdr_len  : 4;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t  hdr_len  : 4;
	uint8_t  reserved : 4;
#endif
	uint8_t  ctrl;
	uint16_t win_size;
	uint16_t checksum;
	uint16_t urg_ptr;
} __attribute__((__packed__));

typedef struct tcp_hdr tcp_hdr_t;
void tcp_output(pkt_t *pkt, uint32_t ip_dst, uint8_t ctrl,
		uint16_t sport, uint16_t dport);
void tcp_input(pkt_t *pkt);

#endif

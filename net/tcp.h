#ifndef _TCP_H_
#define _TCP_H_

#include "config.h"
#ifdef CONF_TCP_RETRANSMIT
#include "../timer.h"
#endif

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80

#define TCPOPT_EOL              0
#define    TCPOLEN_EOL                  1
#define TCPOPT_PAD              0               /* padding after EOL */
#define    TCPOLEN_PAD                  1
#define TCPOPT_NOP              1
#define    TCPOLEN_NOP                  1
#define TCPOPT_MAXSEG           2
#define    TCPOLEN_MAXSEG               4
#define TCPOPT_WINDOW           3
#define    TCPOLEN_WINDOW               3
#define TCPOPT_SACK_PERMITTED   4
#define    TCPOLEN_SACK_PERMITTED       2
#define TCPOPT_SACK             5
#define    TCPOLEN_SACKHDR              2
#define    TCPOLEN_SACK                 8       /* 2*sizeof(tcp_seq) */
#define TCPOPT_TIMESTAMP        8
#define    TCPOLEN_TIMESTAMP            10
#define    TCPOLEN_TSTAMP_APPA          (TCPOLEN_TIMESTAMP+2) /* appendix A */
#define TCPOPT_SIGNATURE        19              /* Keyed MD5: RFC 2385 */
#define    TCPOLEN_SIGNATURE            18
#define TCPOPT_FAST_OPEN        34
#define    TCPOLEN_FAST_OPEN_EMPTY      2
#define    TCPOLEN_FAST_OPEN_MIN        6
#define    TCPOLEN_FAST_OPEN_MAX        18

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

struct tcp_uid {
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
} __attribute__((__packed__));
typedef struct tcp_uid tcp_uid_t;

#ifdef CONF_TCP_RETRANSMIT
struct tcp_retrn_pkt {
	pkt_t *pkt;
	struct list_head list;
} __attribute__((__packed__));
typedef struct tcp_retrn_pkt tcp_retrn_pkt_t;

struct tcp_retrn {
	tim_t timer;
	uint8_t cnt;
	struct list_head retrn_pkt_list;
} __attribute__((__packed__));
typedef struct tcp_retrn tcp_retrn_t;
#endif

struct tcp_options {
	uint16_t mss;
}  __attribute__((__packed__));
typedef struct tcp_options tcp_options_t;

struct tcp_conn {
	uint32_t seqid;
	uint32_t ack;
	tcp_options_t opts;
	tcp_uid_t uid;
	void *sock_info;
	uint8_t status;
	struct list_head list;
	struct list_head pkt_list_head;
#ifdef CONF_TCP_RETRANSMIT
	tcp_retrn_t retrn;
#endif
} __attribute__((__packed__));
typedef struct tcp_conn tcp_conn_t;

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid);
void tcp_conn_delete(tcp_conn_t *tcp_conn);
int
tcp_connect(uint32_t dst_addr, uint16_t dst_port, void *sock_info);
void tcp_output(pkt_t *pkt, tcp_conn_t *tcp_conn, uint8_t flags);
void tcp_input(pkt_t *pkt);

#endif

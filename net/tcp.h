/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#ifndef _TCP_H_
#define _TCP_H_

#include "config.h"
#ifdef CONFIG_TCP_RETRANSMIT
#include "../timer.h"
#endif

#include "socket.h"

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

#ifdef CONFIG_TCP_RETRANSMIT
struct tcp_retrn_pkt {
	pkt_t *pkt;
	list_t list;
} __attribute__((__packed__));
typedef struct tcp_retrn_pkt tcp_retrn_pkt_t;

struct tcp_retrn {
	tim_t timer;
	uint8_t cnt;
	list_t retrn_pkt_list;
} __attribute__((__packed__));
typedef struct tcp_retrn tcp_retrn_t;
#endif

struct tcp_options {
	uint16_t mss;
}  __attribute__((__packed__));
typedef struct tcp_options tcp_options_t;

struct tcp_syn {
	uint32_t seqid;
	uint32_t ack;
	tcp_options_t opts;
	tcp_uid_t tuid;
	uint8_t status;
} __attribute__((__packed__));
typedef struct tcp_syn tcp_syn_t;

struct sock_info;
typedef struct sock_info sock_info_t;

struct tcp_conn {
	tcp_syn_t syn;
	sock_info_t *sock_info;
	list_t list;
	list_t pkt_list_head;
#ifdef CONFIG_TCP_RETRANSMIT
	tcp_retrn_t retrn;
#endif
} __attribute__((__packed__));
typedef struct tcp_conn tcp_conn_t;

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid);
int tcp_conn_add(tcp_conn_t *tcp_conn);
void tcp_conn_delete(tcp_conn_t *tcp_conn);
int
tcp_connect(uint32_t dst_addr, uint16_t dst_port, void *sock_info);
int tcp_output(pkt_t *pkt, tcp_conn_t *tcp_conn, uint8_t flags);
void tcp_input(pkt_t *pkt);

#ifdef CONFIG_HT_STORAGE
void tcp_init(void);
void tcp_shutdown(void);
#else
static inline void tcp_init(void) {}
static inline void tcp_shutdown(void) {}
#endif
#endif

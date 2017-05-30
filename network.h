#ifndef NETWORK_H
               # Replace Windows newlines with Unix newlines#define NETWORK_H
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#define ETH_HEADER_LEN 0x0E
               # Replace Windows newlines with Unix newlines#define ETH_DST_MAC    0x00
               # Replace Windows newlines with Unix newlines#define ETH_SRC_MAC    0x06
               # Replace Windows newlines with Unix newlines#define ETH_TYPE       0x0C
               # Replace Windows newlines with Unix newlines#define ETH_ARP_H      0x08
               # Replace Windows newlines with Unix newlines#define ETH_ARP_L      0x06
               # Replace Windows newlines with Unix newlines#define ETH_IP_H       0x08
               # Replace Windows newlines with Unix newlines#define ETH_IP_L       0x00
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#define ARP_OPCODE     0x14
               # Replace Windows newlines with Unix newlines#define ARP_SRC_MAC    0x16
               # Replace Windows newlines with Unix newlines#define ARP_SRC_IP     0x1C
               # Replace Windows newlines with Unix newlines#define ARP_DST_MAC    0x20
               # Replace Windows newlines with Unix newlines#define ARP_DST_IP     0x26
               # Replace Windows newlines with Unix newlines#define ARP_REPLY_H    0x00
               # Replace Windows newlines with Unix newlines#define ARP_REPLY_L    0x02
               # Replace Windows newlines with Unix newlines#define ARP_DST_IP     0x26
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#define IP_HEADER_LEN  0x14
               # Replace Windows newlines with Unix newlines#define IP_VERSION     0x0E
               # Replace Windows newlines with Unix newlines#define IP_TOTLEN      0x10
               # Replace Windows newlines with Unix newlines#define IP_FLAGS       0x14
               # Replace Windows newlines with Unix newlines#define IP_TTL         0x16
               # Replace Windows newlines with Unix newlines#define IP_PROTO       0x17
               # Replace Windows newlines with Unix newlines#define IP_CHECKSUM    0x18
               # Replace Windows newlines with Unix newlines#define IP_SRC         0x1A
               # Replace Windows newlines with Unix newlines#define IP_DST         0x1E
               # Replace Windows newlines with Unix newlines#define IP_ICMP        0x01
               # Replace Windows newlines with Unix newlines#define IP_TCP         0x06
               # Replace Windows newlines with Unix newlines#define IP_UDP         0x11
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#define ICMP_TYPE      0x22
               # Replace Windows newlines with Unix newlines#define ICMP_CHECKSUM  0x24
               # Replace Windows newlines with Unix newlines#define ICMP_REQUEST   0x08
               # Replace Windows newlines with Unix newlines#define ICMP_REPLY     0x00
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#define TCP_SRC_PORT   0x22
               # Replace Windows newlines with Unix newlines#define TCP_DST_PORT   0x24
               # Replace Windows newlines with Unix newlines#define TCP_SEQ        0x26
               # Replace Windows newlines with Unix newlines#define TCP_SEQACK     0x2A
               # Replace Windows newlines with Unix newlines#define TCP_HEADER_LEN 0x2E
               # Replace Windows newlines with Unix newlines#define TCP_FLAGS      0x2F
               # Replace Windows newlines with Unix newlines#define TCP_CHECKSUM   0x32
               # Replace Windows newlines with Unix newlines#define TCP_OPTIONS    0x36
               # Replace Windows newlines with Unix newlines#define TCP_URG        0x20
               # Replace Windows newlines with Unix newlines#define TCP_ACK        0x10
               # Replace Windows newlines with Unix newlines#define TCP_PSH        0x08
               # Replace Windows newlines with Unix newlines#define TCP_RST        0x04 
               # Replace Windows newlines with Unix newlines#define TCP_SYN        0x02
               # Replace Windows newlines with Unix newlines#define TCP_FIN        0x01
               # Replace Windows newlines with Unix newlines#define TCP_LEN_PLAIN  0x14
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid init_network(uint8_t *mymac,uint8_t *myip,uint16_t mywwwport);
               # Replace Windows newlines with Unix newlinesuint8_t eth_is_arp(uint8_t *buf,uint16_t len);
               # Replace Windows newlines with Unix newlinesuint8_t eth_is_ip(uint8_t *buf,uint16_t len);
               # Replace Windows newlines with Unix newlinesvoid arp_reply(uint8_t *buf);
               # Replace Windows newlines with Unix newlinesvoid icmp_reply(uint8_t *buf,uint16_t len);
               # Replace Windows newlines with Unix newlinesvoid tcp_synack(uint8_t *buf);
               # Replace Windows newlines with Unix newlinesvoid init_len_info(uint8_t *buf);
               # Replace Windows newlines with Unix newlinesuint16_t get_tcp_data_ptr(void);
               # Replace Windows newlines with Unix newlinesuint16_t make_tcp_data_pos(uint8_t *buf,uint16_t pos, const prog_char *progmem_s);
               # Replace Windows newlines with Unix newlinesvoid tcp_ack(uint8_t *buf);
               # Replace Windows newlines with Unix newlinesvoid tcp_ack_with_data(uint8_t *buf,uint16_t dlen);
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#endif
               # Replace Windows newlines with Unix newlines
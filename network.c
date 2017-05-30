#include <avr/io.h>
               # Replace Windows newlines with Unix newlines#include <avr/pgmspace.h>
               # Replace Windows newlines with Unix newlines#include "network.h"
               # Replace Windows newlines with Unix newlines#include "enc28j60.h"
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesstatic uint8_t macaddr[6];
               # Replace Windows newlines with Unix newlinesstatic uint8_t ipaddr[4];
               # Replace Windows newlines with Unix newlinesstatic uint16_t wwwport;
               # Replace Windows newlines with Unix newlinesstatic int16_t info_hdr_len = 0x0000;
               # Replace Windows newlines with Unix newlinesstatic int16_t info_data_len = 0x0000;
               # Replace Windows newlines with Unix newlinesstatic uint8_t seqnum = 0x0A;
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid init_network(uint8_t *mymac,uint8_t *myip,uint16_t mywwwport){
               # Replace Windows newlines with Unix newlines    uint8_t i;
               # Replace Windows newlines with Unix newlines    wwwport = mywwwport;
               # Replace Windows newlines with Unix newlines    for(i=0;i<4;i++) ipaddr[i]=myip[i];
               # Replace Windows newlines with Unix newlines    for(i=0;i<6;i++) macaddr[i]=mymac[i];
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type){
               # Replace Windows newlines with Unix newlines    uint32_t sum = 0;
               # Replace Windows newlines with Unix newlines    if(type==1) {
               # Replace Windows newlines with Unix newlines        sum += IP_UDP;
               # Replace Windows newlines with Unix newlines        sum += len-8;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    if(type==2){
               # Replace Windows newlines with Unix newlines        sum+=IP_TCP; 
               # Replace Windows newlines with Unix newlines        sum+=len-8;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    while(len>1){
               # Replace Windows newlines with Unix newlines        sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
               # Replace Windows newlines with Unix newlines        buf+=2;
               # Replace Windows newlines with Unix newlines        len-=2;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    if(len) sum += ((uint32_t)(0xFF & *buf))<<8;
               # Replace Windows newlines with Unix newlines    while(sum>>16) sum = (sum & 0xFFFF)+(sum >> 16);
               # Replace Windows newlines with Unix newlines    return( (uint16_t) sum ^ 0xFFFF);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t eth_is_arp(uint8_t *buf,uint16_t len){
               # Replace Windows newlines with Unix newlines    uint8_t i=0;
               # Replace Windows newlines with Unix newlines    if (len<41) return(0);
               # Replace Windows newlines with Unix newlines    if(buf[ETH_TYPE] != ETH_ARP_H) return(0);
               # Replace Windows newlines with Unix newlines    if(buf[ETH_TYPE+1] != ETH_ARP_L) return(0);
               # Replace Windows newlines with Unix newlines    for(i=0;i<4;i++) if(buf[ARP_DST_IP+i] != ipaddr[i]) return(0);
               # Replace Windows newlines with Unix newlines    return(1);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t eth_is_ip(uint8_t *buf,uint16_t len){
               # Replace Windows newlines with Unix newlines    uint8_t i=0;
               # Replace Windows newlines with Unix newlines    if(len<42) return(0);
               # Replace Windows newlines with Unix newlines    if(buf[ETH_TYPE] != ETH_IP_H) return(0);
               # Replace Windows newlines with Unix newlines    if(buf[ETH_TYPE+1] != ETH_IP_L) return(0);
               # Replace Windows newlines with Unix newlines    if(buf[IP_VERSION] != 0x45) return(0);
               # Replace Windows newlines with Unix newlines    for(i=0;i<4;i++) if(buf[IP_DST+i]!=ipaddr[i]) return(0);
               # Replace Windows newlines with Unix newlines    return(1);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid make_eth_hdr(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint8_t i=0;
               # Replace Windows newlines with Unix newlines    for(i=0;i<6;i++) {
               # Replace Windows newlines with Unix newlines        buf[ETH_DST_MAC+i]=buf[ETH_SRC_MAC+i];
               # Replace Windows newlines with Unix newlines        buf[ETH_SRC_MAC+i]=macaddr[i];
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid make_ip_checksum(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint16_t ck;
               # Replace Windows newlines with Unix newlines    buf[IP_CHECKSUM]=0;
               # Replace Windows newlines with Unix newlines    buf[IP_CHECKSUM+1]=0;
               # Replace Windows newlines with Unix newlines    buf[IP_FLAGS]=0x40;
               # Replace Windows newlines with Unix newlines    buf[IP_FLAGS+1]=0;
               # Replace Windows newlines with Unix newlines    buf[IP_TTL]=64;
               # Replace Windows newlines with Unix newlines    ck = checksum(&buf[0x0E],IP_HEADER_LEN,0);
               # Replace Windows newlines with Unix newlines    buf[IP_CHECKSUM] = ck >> 8;
               # Replace Windows newlines with Unix newlines    buf[IP_CHECKSUM+1] = ck & 0xFF;
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid make_ip_hdr(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint8_t i=0;
               # Replace Windows newlines with Unix newlines    while(i<4){
               # Replace Windows newlines with Unix newlines        buf[IP_DST+i]=buf[IP_SRC+i];
               # Replace Windows newlines with Unix newlines        buf[IP_SRC+i]=ipaddr[i];
               # Replace Windows newlines with Unix newlines        i++;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    make_ip_checksum(buf);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid make_tcp_hdr(uint8_t *buf,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq) {
               # Replace Windows newlines with Unix newlines    uint8_t i=4;
               # Replace Windows newlines with Unix newlines    uint8_t tseq;
               # Replace Windows newlines with Unix newlines    buf[TCP_DST_PORT] = buf[TCP_SRC_PORT];
               # Replace Windows newlines with Unix newlines    buf[TCP_DST_PORT+1] = buf[TCP_SRC_PORT+1];
               # Replace Windows newlines with Unix newlines    buf[TCP_SRC_PORT] = wwwport >> 8;
               # Replace Windows newlines with Unix newlines    buf[TCP_SRC_PORT+1] = wwwport & 0xFF;
               # Replace Windows newlines with Unix newlines    while(i>0){
               # Replace Windows newlines with Unix newlines        rel_ack_num=buf[TCP_SEQ+i-1]+rel_ack_num;
               # Replace Windows newlines with Unix newlines        tseq=buf[TCP_SEQACK+i-1];
               # Replace Windows newlines with Unix newlines        buf[TCP_SEQACK+i-1]=0xFF&rel_ack_num;
               # Replace Windows newlines with Unix newlines        if (cp_seq) buf[TCP_SEQ+i-1]=tseq;
               # Replace Windows newlines with Unix newlines        else buf[TCP_SEQ+i-1] = 0;
               # Replace Windows newlines with Unix newlines        rel_ack_num=rel_ack_num>>8;
               # Replace Windows newlines with Unix newlines        i--;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    if(cp_seq==0) {
               # Replace Windows newlines with Unix newlines        buf[TCP_SEQ] = 0;
               # Replace Windows newlines with Unix newlines        buf[TCP_SEQ+1] = 0;
               # Replace Windows newlines with Unix newlines        buf[TCP_SEQ+2] = seqnum; 
               # Replace Windows newlines with Unix newlines        buf[TCP_SEQ+3] = 0;
               # Replace Windows newlines with Unix newlines        seqnum += 2;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM] = 0;
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM+1] = 0;
               # Replace Windows newlines with Unix newlines    if(mss) {
               # Replace Windows newlines with Unix newlines        buf[TCP_OPTIONS] = 0x02;
               # Replace Windows newlines with Unix newlines        buf[TCP_OPTIONS+1] = 0x04;
               # Replace Windows newlines with Unix newlines        buf[TCP_OPTIONS+2] = 0x05; 
               # Replace Windows newlines with Unix newlines        buf[TCP_OPTIONS+3] = 0x80;
               # Replace Windows newlines with Unix newlines        buf[TCP_HEADER_LEN] = 0x60;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    else buf[TCP_HEADER_LEN] = 0x50;
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid arp_reply(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint8_t i=0;
               # Replace Windows newlines with Unix newlines    make_eth_hdr(buf);
               # Replace Windows newlines with Unix newlines    buf[ARP_OPCODE] = ARP_REPLY_H;
               # Replace Windows newlines with Unix newlines    buf[ARP_OPCODE+1] = ARP_REPLY_L;
               # Replace Windows newlines with Unix newlines    for(i=0;i<6;i++) {
               # Replace Windows newlines with Unix newlines        buf[ARP_DST_MAC+i] = buf[ARP_SRC_MAC+i];
               # Replace Windows newlines with Unix newlines        buf[ARP_SRC_MAC+i] = macaddr[i];
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    for(i=0;i<4;i++) {
               # Replace Windows newlines with Unix newlines        buf[ARP_DST_IP+i]=buf[ARP_SRC_IP+i];
               # Replace Windows newlines with Unix newlines        buf[ARP_SRC_IP+i]=ipaddr[i];
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    ENC28J60_PacketSend(42,buf); 
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid icmp_reply(uint8_t *buf,uint16_t len) {
               # Replace Windows newlines with Unix newlines    make_eth_hdr(buf);
               # Replace Windows newlines with Unix newlines    make_ip_hdr(buf);
               # Replace Windows newlines with Unix newlines    buf[ICMP_TYPE] = ICMP_REPLY;
               # Replace Windows newlines with Unix newlines    if(buf[ICMP_CHECKSUM] > (0xFF-0x08)) buf[ICMP_CHECKSUM+1]++;
               # Replace Windows newlines with Unix newlines    buf[ICMP_CHECKSUM] += 0x08;
               # Replace Windows newlines with Unix newlines    ENC28J60_PacketSend(len,buf);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid tcp_synack(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint16_t ck;
               # Replace Windows newlines with Unix newlines    make_eth_hdr(buf);
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN] = 0;
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN+1] = IP_HEADER_LEN+TCP_LEN_PLAIN+4;
               # Replace Windows newlines with Unix newlines    make_ip_hdr(buf);
               # Replace Windows newlines with Unix newlines    buf[TCP_FLAGS] = TCP_SYN|TCP_ACK;
               # Replace Windows newlines with Unix newlines    make_tcp_hdr(buf,1,1,0);
               # Replace Windows newlines with Unix newlines    ck=checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN+4,2);
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM] = ck >> 8;
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM+1] = ck & 0xFF;
               # Replace Windows newlines with Unix newlines    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid init_len_info(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    info_data_len = (((int16_t)buf[IP_TOTLEN])<<8)|(buf[IP_TOTLEN+1]&0xFF);
               # Replace Windows newlines with Unix newlines    info_data_len -= IP_HEADER_LEN;
               # Replace Windows newlines with Unix newlines    info_hdr_len = (buf[TCP_HEADER_LEN]>>4)*4;
               # Replace Windows newlines with Unix newlines    info_data_len -= info_hdr_len;
               # Replace Windows newlines with Unix newlines    if(info_data_len<=0) info_data_len=0;
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t get_tcp_data_ptr(void) {
               # Replace Windows newlines with Unix newlines    if(info_data_len) return((uint16_t)TCP_SRC_PORT+info_hdr_len);
               # Replace Windows newlines with Unix newlines    else return(0);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t make_tcp_data_pos(uint8_t *buf,uint16_t pos, const prog_char *progmem_s) {
               # Replace Windows newlines with Unix newlines    char c;
               # Replace Windows newlines with Unix newlines    while((c = pgm_read_byte(progmem_s++))) {
               # Replace Windows newlines with Unix newlines        buf[TCP_CHECKSUM+4+pos] = c;
               # Replace Windows newlines with Unix newlines        pos++;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    return(pos);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t make_tcp_data(uint8_t *buf,uint16_t pos, const char *s) {
               # Replace Windows newlines with Unix newlines    while(*s) {
               # Replace Windows newlines with Unix newlines        buf[TCP_CHECKSUM+4+pos]=*s;
               # Replace Windows newlines with Unix newlines        pos++;
               # Replace Windows newlines with Unix newlines        s++;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    return(pos);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid tcp_ack(uint8_t *buf) {
               # Replace Windows newlines with Unix newlines    uint16_t j;
               # Replace Windows newlines with Unix newlines    make_eth_hdr(buf);
               # Replace Windows newlines with Unix newlines    buf[TCP_FLAGS] = TCP_ACK;
               # Replace Windows newlines with Unix newlines    if(info_data_len==0) make_tcp_hdr(buf,1,0,1);
               # Replace Windows newlines with Unix newlines    else make_tcp_hdr(buf,info_data_len,0,1);
               # Replace Windows newlines with Unix newlines    j = IP_HEADER_LEN+TCP_LEN_PLAIN;
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN] = j >> 8;
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN+1] = j & 0xFF;
               # Replace Windows newlines with Unix newlines    make_ip_hdr(buf);
               # Replace Windows newlines with Unix newlines    j = checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN,2);
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM] = j >> 8;
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM+1] = j & 0xFF;
               # Replace Windows newlines with Unix newlines    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+ETH_HEADER_LEN,buf);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid tcp_ack_with_data(uint8_t *buf,uint16_t dlen) {
               # Replace Windows newlines with Unix newlines    uint16_t j;
               # Replace Windows newlines with Unix newlines    buf[TCP_FLAGS] = TCP_ACK|TCP_PSH|TCP_FIN;
               # Replace Windows newlines with Unix newlines    j = IP_HEADER_LEN+TCP_LEN_PLAIN+dlen;
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN] = j >> 8;
               # Replace Windows newlines with Unix newlines    buf[IP_TOTLEN+1] = j & 0xFF;
               # Replace Windows newlines with Unix newlines    make_ip_checksum(buf);
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM] = 0;
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM+1] = 0;
               # Replace Windows newlines with Unix newlines    j = checksum(&buf[IP_SRC],8+TCP_LEN_PLAIN+dlen,2);
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM] = j>>8;
               # Replace Windows newlines with Unix newlines    buf[TCP_CHECKSUM+1] = j& 0xFF;
               # Replace Windows newlines with Unix newlines    ENC28J60_PacketSend(IP_HEADER_LEN+TCP_LEN_PLAIN+dlen+ETH_HEADER_LEN,buf);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
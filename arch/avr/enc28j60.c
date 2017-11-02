#include <avr/io.h>
#include <util/delay.h>
#include "log.h"
#include "enc28j60.h"
#include "spi_config.h"

static uint8_t ENC28J60_Bank;
static int16_t gNextPacketPtr;

void cs_on(void)
{
	SPI_PORT &= ~(1<<SPI_CS);
}

void cs_off(void)
{
	SPI_PORT |= (1<<SPI_CS);
}

void wait_spi(void)
{
	while (!(SPSR & (1 << SPIF)));
}

uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t address)
{
    cs_on();
    SPDR = op | (address & ADDR_MASK);
    wait_spi();
    SPDR = 0x00;
    wait_spi();
    if (address & 0x80) {
        SPDR = 0x00;
        wait_spi();
    }
    cs_off();
    return SPDR;
}

void ENC28J60_WriteOp(uint8_t op,uint8_t address,uint8_t data) {
    cs_on();
    SPDR = op | (address & ADDR_MASK);
    wait_spi();
    SPDR = data;
    wait_spi();
    cs_off();
}

void ENC28J60_ReadBuffer(uint16_t len,uint8_t* data) {
    cs_on();
    SPDR = RBM;
    wait_spi();
    while (len) {
        len--;
        SPDR = 0x00;
        wait_spi();
        *data = SPDR;
        data++;
    }
    *data='\0';
    cs_off();
}

static void ENC28J60_read_buf(unsigned len, buf_t *buf) {
    int ret = 0;

    cs_on();
    SPDR = RBM;
    wait_spi();
    while (len && ret == 0) {
	    len--;
	    SPDR = 0x00;
	    wait_spi();
	    ret = buf_addc(buf, SPDR);
    }
    cs_off();
}

void ENC28J60_reset(void)
{
    cs_on();
    SPDR = SC;
    wait_spi();
    cs_off();
}

void ENC28J60_WriteBuffer(uint16_t len,uint8_t* data) {
    cs_on();
    SPDR = WBM;
    wait_spi();
    while(len) {
        len--;
        SPDR = *data;
        data++;
        wait_spi();
    }
    cs_off();
}

void ENC28J60_SetBank(uint8_t address) {
    if((address & BANK_MASK) != ENC28J60_Bank) {
        ENC28J60_WriteOp(BFC,ECON1,(BSEL1|BSEL0));
        ENC28J60_WriteOp(BFS,ECON1,(address & BANK_MASK)>>5);
        ENC28J60_Bank = (address & BANK_MASK);
    }
}

uint8_t ENC28J60_Read(uint8_t address) {
    ENC28J60_SetBank(address);
    return ENC28J60_ReadOp(RCR,address);
}

void ENC28J60_Write(uint8_t address,uint8_t data) {
    ENC28J60_SetBank(address);
    ENC28J60_WriteOp(WCR,address,data);
}

uint16_t ENC28J60_PhyRead(uint8_t address) {
    ENC28J60_Write(MIREGADR,address);
    ENC28J60_Write(MICMD,MIIRD);
    _delay_us(15);
    while(ENC28J60_Read(MISTAT) & BUSY);
    ENC28J60_Write(MICMD,0x00);
    return(ENC28J60_Read(MIRDH));
}

void ENC28J60_PhyWrite(uint8_t address,uint16_t data) {
    ENC28J60_Write(MIREGADR,address);
    ENC28J60_Write(MIWRL,data);
    ENC28J60_Write(MIWRH,data>>8);
    while(ENC28J60_Read(MISTAT) & BUSY) _delay_us(15);
}

void ENC28J60_ClkOut(uint8_t clock) {
    ENC28J60_Write(ECOCON,clock & 0x7);
}

void ENC28J60_Init(uint8_t* macaddr) {
    SPI_DDR |= (1<<SPI_CS);
    cs_off();
    SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK);
    SPI_DDR &= ~(1<<SPI_MISO);
    SPI_PORT &= ~(1<<SPI_MOSI);
    SPI_PORT &= ~(1<<SPI_SCK);
    SPCR = (1<<SPE)|(1<<MSTR);
    SPSR |= (1<<SPI2X);
    ENC28J60_WriteOp(SC,0,SC);
    _delay_loop_1(205);
    gNextPacketPtr = RXSTART_INIT;

    ENC28J60_Write(ERXSTL,RXSTART_INIT&0xFF);
    ENC28J60_Write(ERXSTH,RXSTART_INIT>>8);

    ENC28J60_Write(ERXRDPTL,RXSTART_INIT&0xFF);
    ENC28J60_Write(ERXRDPTH,RXSTART_INIT>>8);

    ENC28J60_Write(ERXNDL,RXSTOP_INIT&0xFF);
    ENC28J60_Write(ERXNDH,RXSTOP_INIT>>8);

    ENC28J60_Write(ETXSTL,TXSTART_INIT&0xFF);
    ENC28J60_Write(ETXSTH,TXSTART_INIT>>8);

    ENC28J60_Write(ETXNDL,TXSTOP_INIT&0xFF);
    ENC28J60_Write(ETXNDH,TXSTOP_INIT>>8);

    ENC28J60_Write(ERXFCON,UCEN|CRCEN|PMEN);
    ENC28J60_Write(EPMM0,0x3f);
    ENC28J60_Write(EPMM1,0x30);
    ENC28J60_Write(EPMCSL,0xf9);
    ENC28J60_Write(EPMCSH,0xf7);
    ENC28J60_Write(MACON1,(MARXEN|TXPAUS|RXPAUS));
    ENC28J60_Write(MACON2,0x00);
    ENC28J60_WriteOp(BFS,MACON3,(PADCFG0|TXCRCEN|FRMLNEN));
    ENC28J60_Write(MAIPGL,0x12);
    ENC28J60_Write(MAIPGH,0x0C);
    ENC28J60_Write(MABBIPG,0x12);
    ENC28J60_Write(MAMXFLL,MAX_FRAMELEN&0xFF);    
    ENC28J60_Write(MAMXFLH,MAX_FRAMELEN>>8);
    ENC28J60_Write(MAADR5,macaddr[0]);
    ENC28J60_Write(MAADR4,macaddr[1]);
    ENC28J60_Write(MAADR3,macaddr[2]);
    ENC28J60_Write(MAADR2,macaddr[3]);
    ENC28J60_Write(MAADR1,macaddr[4]);
    ENC28J60_Write(MAADR0,macaddr[5]);
    ENC28J60_PhyWrite(PHCON2,HDLDIS);
    ENC28J60_SetBank(ECON1);
    ENC28J60_WriteOp(BFS,EIE,(INTIE|PKTIE));
    ENC28J60_WriteOp(BFS,ECON1,RXEN);
}

uint8_t ENC28J60_HasRxPkt(void) {
    if(ENC28J60_Read(EPKTCNT)==0) return(0);
    return(1);
}

uint16_t ENC28J60_PacketReceive(buf_t *buf) {
    uint16_t rxstat;
    uint16_t len;

    if (ENC28J60_Read(EPKTCNT) == 0)
	    return 0;

    ENC28J60_Write(ERDPTL,(gNextPacketPtr & 0xFF));
    ENC28J60_Write(ERDPTH,(gNextPacketPtr)>>8);
    gNextPacketPtr = ENC28J60_ReadOp(RBM,0);
    gNextPacketPtr |= ENC28J60_ReadOp(RBM,0)<<8;
    len = ENC28J60_ReadOp(RBM,0);
    len |= ENC28J60_ReadOp(RBM,0)<<8;
    len -= 4;
    rxstat = ENC28J60_ReadOp(RBM,0);
    rxstat |= ((uint16_t)ENC28J60_ReadOp(RBM,0))<<8;

    if (len > (unsigned)buf->size) {
	    /* XXX check pkt sizes (> PKT_SIZE) icmp: pkt too big? */
	    len = buf->size;
    }

    if ((rxstat & 0x80) == 0) {
    	    len = 0;
    } else
	    ENC28J60_read_buf(len, buf);

    ENC28J60_Write(ERXRDPTL,(gNextPacketPtr &0xFF));
    ENC28J60_Write(ERXRDPTH,(gNextPacketPtr)>>8);
    if ((gNextPacketPtr-1 < RXSTART_INIT)||(gNextPacketPtr-1 > RXSTOP_INIT)) {
        ENC28J60_Write(ERXRDPTL,(RXSTOP_INIT)&0xFF);
        ENC28J60_Write(ERXRDPTH,(RXSTOP_INIT)>>8);
    }
    else {
        ENC28J60_Write(ERXRDPTL,(gNextPacketPtr-1)&0xFF);
        ENC28J60_Write(ERXRDPTH,(gNextPacketPtr-1)>>8);
    }
    ENC28J60_WriteOp(BFS,ECON2,PKTDEC);
    return len;
}

uint16_t ENC28J60_PacketSend(const buf_t *buf) {
    while (ENC28J60_ReadOp(RCR,ECON1) & TXRTS) {
        if(ENC28J60_Read(EIR) & TXERIF) {
            ENC28J60_WriteOp(BFS,ECON1,TXRST);
            ENC28J60_WriteOp(BFC,ECON1,TXRST);
        }
    }
    ENC28J60_Write(EWRPTL,TXSTART_INIT&0xFF);
    ENC28J60_Write(EWRPTH,TXSTART_INIT>>8);
    ENC28J60_Write(ETXNDL,(TXSTART_INIT+buf->len)&0xFF);
    ENC28J60_Write(ETXNDH,(TXSTART_INIT+buf->len)>>8);
    ENC28J60_WriteOp(WBM,0,0x00);
    ENC28J60_WriteBuffer(buf->len, buf->data);
    ENC28J60_WriteOp(BFS,ECON1,TXRTS);
    return 0;
}

uint8_t ENC28J60_GetRev(void) {
    return(ENC28J60_Read(EREVID));
}

uint8_t ENC28J60_LinkUp(void) {
    return(ENC28J60_PhyRead(PHSTAT2) && 4);
}

void enc28j60_iface_reset(iface_t *iface)
{
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	_delay_loop_1(50);
	ENC28J60_Init(iface->mac_addr);
	ENC28J60_ClkOut(2);
	_delay_loop_1(50);
	ENC28J60_PhyWrite(PHLCON,0x0476);
	_delay_loop_1(50);
}

extern uint8_t net_wd;

void enc28j60_get_pkts(iface_t *iface)
{
	uint8_t eint = ENC28J60_Read(EIR);
	uint16_t plen;
	pkt_t *pkt;
//	uint16_t freespace, erxwrpt, erxrdpt, erxnd, erxst;

	net_wd = 0;
	if (eint == 0)
		return;
#if 0
	erxwrpt = ENC28J60_Read(ERXWRPTL);
	erxwrpt |= ENC28J60_Read(ERXWRPTH) << 8;

	erxrdpt = ENC28J60_Read(ERXRDPTL);
	erxrdpt |= ENC28J60_Read(ERXRDPTH) << 8;

	erxnd = ENC28J60_Read(ERXNDL);
	erxnd |= ENC28J60_Read(ERXNDH) << 8;

	erxst = ENC28J60_Read(ERXSTL);
	erxst |= ENC28J60_Read(ERXSTH) << 8;

	if (erxwrpt > erxrdpt) {
		freespace = (erxnd - erxst) - (erxwrpt - erxrdpt);
	} else if (erxwrpt == erxrdpt) {
		freespace = erxnd - erxst;
	} else {
		freespace = erxrdpt - erxwrpt - 1;
	}
	DEBUG_LOG("int:0x%X freespace:%u\n", eint, freespace);
#endif
	if (eint & TXERIF) {
		ENC28J60_WriteOp(BFC, EIE, TXERIF);
	}

	if (eint & RXERIF) {
		ENC28J60_WriteOp(BFC, EIE, RXERIF);
	}

	if (!(eint & PKTIF)) {
		return;
	}

	if ((pkt = pkt_alloc()) == NULL) {
		DEBUG_LOG("out of packets\n");
		return;
	}

	plen = iface->recv(&pkt->buf);
	DEBUG_LOG("len:%u\n", plen);
	if (plen == 0)
		goto end;

	if (pkt_put(&iface->rx, pkt) < 0)
		pkt_free(pkt);

	return;
 end:
	pkt_free(pkt);
}
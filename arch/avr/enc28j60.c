#include <utils.h>
#include "avr/io.h"
#include "enc28j60.h"
#include "enc28j60conf.h"
#include "log.h"

#ifndef cbi
#define cbi(reg,bit) reg &= ~(_BV(bit))
#endif

#ifndef sbi
#define sbi(reg,bit) reg |= (_BV(bit))
#endif

#define ADDR_MASK	0x1F
#define BANK_MASK	0x60
#define SPRD_MASK	0x80

/* All-bank registers */
#define EIE		0x1B
#define EIR		0x1C
#define ESTAT		0x1D
#define ECON2		0x1E
#define ECON1		0x1F

/* Bank 0 registers */
#define ERDPTL		(0x00|0x00)
#define ERDPTH		(0x01|0x00)
#define EWRPTL		(0x02|0x00)
#define EWRPTH		(0x03|0x00)
#define ETXSTL		(0x04|0x00)
#define ETXSTH		(0x05|0x00)
#define ETXNDL		(0x06|0x00)
#define ETXNDH		(0x07|0x00)
#define ERXSTL		(0x08|0x00)
#define ERXSTH		(0x09|0x00)
#define ERXNDL		(0x0A|0x00)
#define ERXNDH		(0x0B|0x00)
#define ERXRDPTL	(0x0C|0x00)
#define ERXRDPTH	(0x0D|0x00)
#define ERXWRPTL	(0x0E|0x00)
#define ERXWRPTH	(0x0F|0x00)
#define EDMASTL		(0x10|0x00)
#define EDMASTH		(0x11|0x00)
#define EDMANDL		(0x12|0x00)
#define EDMANDH		(0x13|0x00)
#define EDMADSTL	(0x14|0x00)
#define EDMADSTH	(0x15|0x00)
#define EDMACSL		(0x16|0x00)
#define EDMACSH		(0x17|0x00)

/* Bank 1 registers */
#define EHT0		(0x00|0x20)
#define EHT1		(0x01|0x20)
#define EHT2		(0x02|0x20)
#define EHT3		(0x03|0x20)
#define EHT4		(0x04|0x20)
#define EHT5		(0x05|0x20)
#define EHT6		(0x06|0x20)
#define EHT7		(0x07|0x20)
#define EPMM0		(0x08|0x20)
#define EPMM1		(0x09|0x20)
#define EPMM2		(0x0A|0x20)
#define EPMM3		(0x0B|0x20)
#define EPMM4		(0x0C|0x20)
#define EPMM5		(0x0D|0x20)
#define EPMM6		(0x0E|0x20)
#define EPMM7		(0x0F|0x20)
#define EPMCSL		(0x10|0x20)
#define EPMCSH		(0x11|0x20)
#define EPMOL		(0x14|0x20)
#define EPMOH		(0x15|0x20)
#define EWOLIE		(0x16|0x20)
#define EWOLIR		(0x17|0x20)
#define ERXFCON		(0x18|0x20)
#define EPKTCNT		(0x19|0x20)

/* Bank 2 registers */
#define MACON1		(0x00|0x40|0x80)
#define MACON2		(0x01|0x40|0x80)
#define MACON3		(0x02|0x40|0x80)
#define MACON4		(0x03|0x40|0x80)
#define MABBIPG		(0x04|0x40|0x80)
#define MAIPGL		(0x06|0x40|0x80)
#define MAIPGH		(0x07|0x40|0x80)
#define MACLCON1	(0x08|0x40|0x80)
#define MACLCON2	(0x09|0x40|0x80)
#define MAMXFLL		(0x0A|0x40|0x80)
#define MAMXFLH		(0x0B|0x40|0x80)
#define MAPHSUP		(0x0D|0x40|0x80)
#define MICON		(0x11|0x40|0x80)
#define MICMD		(0x12|0x40|0x80)
#define MIREGADR	(0x14|0x40|0x80)
#define MIWRL		(0x16|0x40|0x80)
#define MIWRH		(0x17|0x40|0x80)
#define MIRDL		(0x18|0x40|0x80)
#define MIRDH		(0x19|0x40|0x80)

/* Bank 3 registers */
#define MAADR1		(0x00|0x60|0x80)
#define MAADR0		(0x01|0x60|0x80)
#define MAADR3		(0x02|0x60|0x80)
#define MAADR2		(0x03|0x60|0x80)
#define MAADR5		(0x04|0x60|0x80)
#define MAADR4		(0x05|0x60|0x80)
#define EBSTSD		(0x06|0x60)
#define EBSTCON		(0x07|0x60)
#define EBSTCSL		(0x08|0x60)
#define EBSTCSH		(0x09|0x60)
#define MISTAT		(0x0A|0x60|0x80)
#define EREVID		(0x12|0x60)
#define ECOCON		(0x15|0x60)
#define EFLOCON		(0x17|0x60)
#define EPAUSL		(0x18|0x60)
#define EPAUSH		(0x19|0x60)

/* PHY registers */
#define PHCON1		0x00
#define PHSTAT1		0x01
#define PHHID1		0x02
#define PHHID2		0x03
#define PHCON2		0x10
#define PHSTAT2		0x11
#define PHIE		0x12
#define PHIR		0x13
#define PHLCON		0x14

/* ENC28J60 EIE Register Bit Definitions */
#define EIE_INTIE		0x80
#define EIE_PKTIE		0x40
#define EIE_DMAIE		0x20
#define EIE_LINKIE		0x10
#define EIE_TXIE		0x08
#define EIE_WOLIE		0x04
#define EIE_TXERIE		0x02
#define EIE_RXERIE		0x01

/* ENC28J60 EIR Register Bit Definitions */
#define EIR_PKTIF		0x40
#define EIR_DMAIF		0x20
#define EIR_LINKIF		0x10
#define EIR_TXIF		0x08
#define EIR_WOLIF		0x04
#define EIR_TXERIF		0x02
#define EIR_RXERIF		0x01

/* ENC28J60 ESTAT Register Bit Definitions */
#define ESTAT_INT		0x80
#define ESTAT_LATECOL	0x10
#define ESTAT_RXBUSY	0x04
#define ESTAT_TXABRT	0x02
#define ESTAT_CLKRDY	0x01

/* ENC28J60 ECON2 Register Bit Definitions */
#define ECON2_AUTOINC	0x80
#define ECON2_PKTDEC	0x40
#define ECON2_PWRSV		0x20
#define ECON2_VRPS		0x08

/* ENC28J60 ECON1 Register Bit Definitions */
#define ECON1_TXRST		0x80
#define	ECON1_RXRST		0x40
#define ECON1_DMAST		0x20
#define ECON1_CSUMEN	0x10
#define ECON1_TXRTS		0x08
#define	ECON1_RXEN		0x04
#define ECON1_BSEL1		0x02
#define ECON1_BSEL0		0x01

/* ENC28J60 MACON1 Register Bit Definitions */
#define MACON1_LOOPBK	0x10
#define MACON1_TXPAUS	0x08
#define MACON1_RXPAUS	0x04
#define MACON1_PASSALL	0x02
#define MACON1_MARXEN	0x01

/* ENC28J60 MACON2 Register Bit Definitions */
#define MACON2_MARST	0x80
#define MACON2_RNDRST	0x40
#define MACON2_MARXRST	0x08
#define MACON2_RFUNRST	0x04
#define MACON2_MATXRST	0x02
#define MACON2_TFUNRST	0x01

/* ENC28J60 MACON3 Register Bit Definitions */
#define MACON3_PADCFG2	0x80
#define MACON3_PADCFG1	0x40
#define MACON3_PADCFG0	0x20
#define MACON3_TXCRCEN	0x10
#define MACON3_PHDRLEN	0x08
#define MACON3_HFRMLEN	0x04
#define MACON3_FRMLNEN	0x02
#define MACON3_FULDPX	0x01

/* ENC28J60 MICMD Register Bit Definitions */
#define MICMD_MIISCAN	0x02
#define MICMD_MIIRD	0x01

/* ENC28J60 MISTAT Register Bit Definitions */
#define MISTAT_NVALID	0x04
#define MISTAT_SCAN	0x02
#define MISTAT_BUSY	0x01

/* ENC28J60 PHY PHCON1 Register Bit Definitions */
#define	PHCON1_PRST	0x8000
#define	PHCON1_PLOOPBK	0x4000
#define	PHCON1_PPWRSV	0x0800
#define	PHCON1_PDPXMD	0x0100

/* ENC28J60 PHY PHSTAT1 Register Bit Definitions */
#define	PHSTAT1_PFDPX	0x1000
#define	PHSTAT1_PHDPX	0x0800
#define	PHSTAT1_LLSTAT	0x0004
#define	PHSTAT1_JBSTAT	0x0002

/* ENC28J60 PHY PHCON2 Register Bit Definitions */
#define PHCON2_FRCLINK	0x4000
#define PHCON2_TXDIS	0x2000
#define PHCON2_JABBER	0x0400
#define PHCON2_HDLDIS	0x0100

/* ENC28J60 Packet Control Byte Bit Definitions */
#define PKTCTRL_PHUGEEN		0x08
#define PKTCTRL_PPADEN		0x04
#define PKTCTRL_PCRCEN		0x02
#define PKTCTRL_POVERRIDE	0x01

/* SPI operation codes */
#define ENC28J60_READ_CTRL_REG	0x00
#define ENC28J60_READ_BUF_MEM	0x3A
#define ENC28J60_WRITE_CTRL_REG	0x40
#define ENC28J60_WRITE_BUF_MEM	0x7A
#define ENC28J60_BIT_FIELD_SET	0x80
#define ENC28J60_BIT_FIELD_CLR	0xA0
#define ENC28J60_SOFT_RESET	0xFF

static uint8_t enc28j60_bank;
static uint16_t next_pkt_ptr;

uint8_t enc28j60_read_op(uint8_t op, uint8_t address)
{
	uint8_t data;

	ENC28J60_CONTROL_PORT &= ~(1 << ENC28J60_CONTROL_CS);

	SPDR = op | (address & ADDR_MASK);
	while (!(SPSR & (1 << SPIF)));

	SPDR = 0x00;
	while (!(SPSR & (1 << SPIF)));

	if (address & 0x80) {
		SPDR = 0x00;
		while (!(SPSR & (1 << SPIF)));
	}
	data = SPDR;

	ENC28J60_CONTROL_PORT |= (1 << ENC28J60_CONTROL_CS);
	return data;
}

void enc28j60_write_op(uint8_t op, uint8_t address, uint8_t data)
{
	ENC28J60_CONTROL_PORT &= ~(1 << ENC28J60_CONTROL_CS);

	SPDR = op | (address & ADDR_MASK);
	while (!(SPSR & (1 << SPIF)));

	SPDR = data;
	while (!(SPSR & (1 << SPIF)));

	ENC28J60_CONTROL_PORT |= (1 << ENC28J60_CONTROL_CS);
}

void enc28j60_read_buffer(uint16_t len, buf_t *buf)
{
	ENC28J60_CONTROL_PORT &= ~(1 << ENC28J60_CONTROL_CS);

	SPDR = ENC28J60_READ_BUF_MEM;
	while (!(SPSR & (1 << SPIF)));
	while (len--) {
		SPDR = 0x00;
		while (!(SPSR & (1 << SPIF)));
		__buf_addc(buf, SPDR);
	}
	ENC28J60_CONTROL_PORT |= (1 << ENC28J60_CONTROL_CS);
}

void enc28j60_write_buffer(uint16_t len, uint8_t* data)
{
	ENC28J60_CONTROL_PORT &= ~(1 << ENC28J60_CONTROL_CS);

	SPDR = ENC28J60_WRITE_BUF_MEM;
	while (!(SPSR & (1 << SPIF)));
	while (len--) {
		SPDR = *data++;
		while (!(SPSR & (1 << SPIF)));
	}
	ENC28J60_CONTROL_PORT |= (1 << ENC28J60_CONTROL_CS);
}

static void enc28j60_set_bank(uint8_t address)
{
	if ((address & BANK_MASK) == enc28j60_bank)
		return;
	enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, ECON1,
			  ECON1_BSEL1 | ECON1_BSEL0);
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1,
			  (address & BANK_MASK) >> 5);
	enc28j60_bank = (address & BANK_MASK);
}

uint8_t enc28j60_read(uint8_t address)
{
	enc28j60_set_bank(address);
	return enc28j60_read_op(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60_write(uint8_t address, uint8_t data)
{
	enc28j60_set_bank(address);
	enc28j60_write_op(ENC28J60_WRITE_CTRL_REG, address, data);
}

uint16_t enc28j60_phy_read(uint8_t address)
{
	uint16_t data;

	enc28j60_write(MIREGADR, address);
	enc28j60_write(MICMD, MICMD_MIIRD);

	while (enc28j60_read(MISTAT) & MISTAT_BUSY);

	enc28j60_write(MICMD, 0x00);

	data  = enc28j60_read(MIRDL);
	data |= enc28j60_read(MIRDH);
	return data;
}

void enc28j60_phy_write(uint8_t address, uint16_t data)
{
	enc28j60_write(MIREGADR, address);

	enc28j60_write(MIWRL, data);
	enc28j60_write(MIWRH, data >> 8);

	while (enc28j60_read(MISTAT) & MISTAT_BUSY);
}

void enc28j60_init(uint8_t *macaddr)
{
	sbi(ENC28J60_CONTROL_DDR, ENC28J60_CONTROL_CS);
	sbi(ENC28J60_CONTROL_PORT, ENC28J60_CONTROL_CS);

	sbi(ENC28J60_SPI_PORT, ENC28J60_SPI_SCK);
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_SCK);
	cbi(ENC28J60_SPI_DDR, ENC28J60_SPI_MISO);
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_MOSI);
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_SS);

	sbi(SPCR, MSTR);
	cbi(SPCR, CPOL);
	cbi(SPCR,DORD);
	cbi(SPCR, SPR0);
	cbi(SPCR, SPR1);
	sbi(SPSR, SPI2X);
	sbi(SPCR, SPE);

	enc28j60_write_op(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	delay_us(50);

	while (!(enc28j60_read(ESTAT) & ESTAT_CLKRDY));

	next_pkt_ptr = RXSTART_INIT;
	enc28j60_write(ERXSTL, RXSTART_INIT & 0xFF);
	enc28j60_write(ERXSTH, RXSTART_INIT >> 8);
	enc28j60_write(ERXRDPTL, RXSTART_INIT & 0xFF);
	enc28j60_write(ERXRDPTH, RXSTART_INIT >> 8);
	enc28j60_write(ERXNDL, RXSTOP_INIT & 0xFF);
	enc28j60_write(ERXNDH, RXSTOP_INIT >> 8);
	enc28j60_write(ETXSTL, TXSTART_INIT & 0xFF);
	enc28j60_write(ETXSTH, TXSTART_INIT >> 8);

	enc28j60_write(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
	enc28j60_write(MACON2, 0x00);
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, MACON3,
			  MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
	enc28j60_write(MAIPGL, 0x12);
	enc28j60_write(MAIPGH, 0x0C);
	enc28j60_write(MABBIPG, 0x12);
	enc28j60_write(MAMXFLL, MAX_FRAMELEN & 0xFF);
	enc28j60_write(MAMXFLH, MAX_FRAMELEN >> 8);

	enc28j60_write(MAADR5, macaddr[0]);
	enc28j60_write(MAADR4, macaddr[1]);
	enc28j60_write(MAADR3, macaddr[2]);
	enc28j60_write(MAADR2, macaddr[3]);
	enc28j60_write(MAADR1, macaddr[4]);
	enc28j60_write(MAADR0, macaddr[5]);

	enc28j60_phy_write(PHCON2, PHCON2_HDLDIS);

	enc28j60_set_bank(ECON1);
	/* enc28j60_write_op(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE | EIE_PKTIE); */
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE | EIE_LINKIE);
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

int enc28j60_pkt_send(const iface_t *iface, pkt_t *pkt)
{
	enc28j60_write(EWRPTL, TXSTART_INIT);
	enc28j60_write(EWRPTH, TXSTART_INIT>>8);
	enc28j60_write(ETXNDL, (TXSTART_INIT + pkt->buf.len));
	enc28j60_write(ETXNDH, (TXSTART_INIT + pkt->buf.len) >> 8);

	enc28j60_write_op(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	enc28j60_write_buffer(pkt->buf.len,  buf_data(&pkt->buf));

	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
	pkt_free(pkt);

	return 0;
}

uint8_t enc28j60_get_interrupts(void)
{
	return enc28j60_read(EIR);
}

#ifdef INTERRUPT_DRIVEN
void enc28j60_pkt_recv(const iface_t *iface)
{
	pkt_t *pkt;
	uint16_t rxstat;
	uint16_t len;
	uint8_t interrupts = enc28j60_read(EIR);

	if (interrupts & EIE_RXERIE)
		enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, EIR, EIE_RXERIE);

	if (interrupts & EIE_TXERIE)
		enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, EIR, EIE_TXERIE);

	while (enc28j60_read(EIR) & EIR_PKTIF) {
		enc28j60_write(ERDPTL, next_pkt_ptr);
		enc28j60_write(ERDPTH, next_pkt_ptr >> 8);

		next_pkt_ptr  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
		next_pkt_ptr |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;

		len  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
		len |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;

		if (len > 1518)
			goto end;

		rxstat  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
		rxstat |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;

		len = MIN(len, CONFIG_PKT_SIZE);
		if (len == 0)
			goto end;

		if ((pkt = pkt_get(iface->pkt_pool)) == NULL) {
			DEBUG_LOG("out of packets\n");
			break;
		}
		enc28j60_read_buffer(len, &pkt->buf);
		if_schedule_receive(iface, pkt);

	end:
		enc28j60_write(ERXRDPTL, next_pkt_ptr);
		enc28j60_write(ERXRDPTH, next_pkt_ptr >> 8);
		enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	}
}
#else
void enc28j60_handle_interrupts(const iface_t *iface)
{
	uint8_t interrupts = enc28j60_read(EIR);

	if (interrupts & EIE_RXERIE)
		enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, EIR, EIE_RXERIE);

	if (interrupts & EIE_TXERIE)
		enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, EIR, EIE_TXERIE);

	/* while (enc28j60_read(EIR) & EIR_PKTIF) { */
	/* } */
}

void enc28j60_pkt_recv(const iface_t *iface)
{
	pkt_t *pkt;
	uint16_t len, rxstat;

	if (enc28j60_read(EPKTCNT) == 0)
		return;

	enc28j60_write(ERDPTL, next_pkt_ptr);
	enc28j60_write(ERDPTH, next_pkt_ptr >> 8);

	next_pkt_ptr  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
	next_pkt_ptr |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;
	len  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;

	/* discard packet with incorrect length */
	if (len == 0 || len > 1518)
		goto end;

	rxstat  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0) << 8;

	len = MIN(len, CONFIG_PKT_SIZE);

	if ((pkt = pkt_alloc()) == NULL) {
		DEBUG_LOG("out of packets\n");
		return;
	}
	enc28j60_read_buffer(len, &pkt->buf);
	pkt_put(iface->rx, pkt);

 end:
	enc28j60_write(ERXRDPTL, next_pkt_ptr);
	enc28j60_write(ERXRDPTH, next_pkt_ptr >> 8);
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	iface->if_input(iface);
}
#endif

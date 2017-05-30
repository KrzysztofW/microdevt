#ifndef ENC28J60_H
               # Replace Windows newlines with Unix newlines#define ENC28J60_H
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Register Masks
               # Replace Windows newlines with Unix newlines#define ADDR_MASK 0x1F
               # Replace Windows newlines with Unix newlines#define BANK_MASK 0x60
               # Replace Windows newlines with Unix newlines#define SPRD_MASK 0x80
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// All Banks Registers
               # Replace Windows newlines with Unix newlines#define EIE      0x1B 
               # Replace Windows newlines with Unix newlines#define EIR      0x1C
               # Replace Windows newlines with Unix newlines#define ESTAT    0x1D
               # Replace Windows newlines with Unix newlines#define ECON2    0x1E
               # Replace Windows newlines with Unix newlines#define ECON1    0x1F
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Bank 0 Registers
               # Replace Windows newlines with Unix newlines#define ERDPTL   0x00
               # Replace Windows newlines with Unix newlines#define ERDPTH   0x01
               # Replace Windows newlines with Unix newlines#define EWRPTL   0x02
               # Replace Windows newlines with Unix newlines#define EWRPTH   0x03
               # Replace Windows newlines with Unix newlines#define ETXSTL   0x04
               # Replace Windows newlines with Unix newlines#define ETXSTH   0x05
               # Replace Windows newlines with Unix newlines#define ETXNDL   0x06
               # Replace Windows newlines with Unix newlines#define ETXNDH   0x07
               # Replace Windows newlines with Unix newlines#define ERXSTL   0x08
               # Replace Windows newlines with Unix newlines#define ERXSTH   0x09
               # Replace Windows newlines with Unix newlines#define ERXNDL   0x0A
               # Replace Windows newlines with Unix newlines#define ERXNDH   0x0B
               # Replace Windows newlines with Unix newlines#define ERXRDPTL 0x0C
               # Replace Windows newlines with Unix newlines#define ERXRDPTH 0x0D
               # Replace Windows newlines with Unix newlines#define ERXWRPTL 0x0E
               # Replace Windows newlines with Unix newlines#define ERXWRPTH 0x0F
               # Replace Windows newlines with Unix newlines#define EDMASTL  0x10
               # Replace Windows newlines with Unix newlines#define EDMASTH  0x11
               # Replace Windows newlines with Unix newlines#define EDMANDL  0x12
               # Replace Windows newlines with Unix newlines#define EDMANDH  0x13
               # Replace Windows newlines with Unix newlines#define EDMADSTL 0x14
               # Replace Windows newlines with Unix newlines#define EDMADSTH 0x15
               # Replace Windows newlines with Unix newlines#define EDMACSL  0x16
               # Replace Windows newlines with Unix newlines#define EDMACSH  0x17
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Bank 1 Registers
               # Replace Windows newlines with Unix newlines#define EHT0     0x20
               # Replace Windows newlines with Unix newlines#define EHT1     0x21
               # Replace Windows newlines with Unix newlines#define EHT2     0x22
               # Replace Windows newlines with Unix newlines#define EHT3     0x23
               # Replace Windows newlines with Unix newlines#define EHT4     0x24
               # Replace Windows newlines with Unix newlines#define EHT5     0x25
               # Replace Windows newlines with Unix newlines#define EHT6     0x26
               # Replace Windows newlines with Unix newlines#define EHT7     0x27
               # Replace Windows newlines with Unix newlines#define EPMM0    0x28
               # Replace Windows newlines with Unix newlines#define EPMM1    0x29
               # Replace Windows newlines with Unix newlines#define EPMM2    0x2A
               # Replace Windows newlines with Unix newlines#define EPMM3    0x2B
               # Replace Windows newlines with Unix newlines#define EPMM4    0x2C
               # Replace Windows newlines with Unix newlines#define EPMM5    0x2D
               # Replace Windows newlines with Unix newlines#define EPMM6    0x2E
               # Replace Windows newlines with Unix newlines#define EPMM7    0x2F
               # Replace Windows newlines with Unix newlines#define EPMCSL   0x30
               # Replace Windows newlines with Unix newlines#define EPMCSH   0x31
               # Replace Windows newlines with Unix newlines#define EPMOL    0x34
               # Replace Windows newlines with Unix newlines#define EPMOH    0x35
               # Replace Windows newlines with Unix newlines#define EWOLIE   0x36
               # Replace Windows newlines with Unix newlines#define EWOLIR   0x37
               # Replace Windows newlines with Unix newlines#define ERXFCON  0x38
               # Replace Windows newlines with Unix newlines#define EPKTCNT  0x39
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Bank 2 Register
               # Replace Windows newlines with Unix newlines#define MACON1   0xC0
               # Replace Windows newlines with Unix newlines#define MACON2   0xC1
               # Replace Windows newlines with Unix newlines#define MACON3   0xC2
               # Replace Windows newlines with Unix newlines#define MACON4   0xC3
               # Replace Windows newlines with Unix newlines#define MABBIPG  0xC4
               # Replace Windows newlines with Unix newlines#define MAIPGL   0xC6
               # Replace Windows newlines with Unix newlines#define MAIPGH   0xC7
               # Replace Windows newlines with Unix newlines#define MACLCON1 0xC8
               # Replace Windows newlines with Unix newlines#define MACLCON2 0xC9
               # Replace Windows newlines with Unix newlines#define MAMXFLL  0xCA
               # Replace Windows newlines with Unix newlines#define MAMXFLH  0xCB
               # Replace Windows newlines with Unix newlines#define MAPHSUP  0xCD
               # Replace Windows newlines with Unix newlines#define MICON    0xD1
               # Replace Windows newlines with Unix newlines#define MICMD    0xD2
               # Replace Windows newlines with Unix newlines#define MIREGADR 0xD4
               # Replace Windows newlines with Unix newlines#define MIWRL    0xD6
               # Replace Windows newlines with Unix newlines#define MIWRH    0xD7
               # Replace Windows newlines with Unix newlines#define MIRDL    0xD8
               # Replace Windows newlines with Unix newlines#define MIRDH    0xD9
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Bank 3 Registers
               # Replace Windows newlines with Unix newlines#define MAADR1   0xE0
               # Replace Windows newlines with Unix newlines#define MAADR0   0xE1
               # Replace Windows newlines with Unix newlines#define MAADR3   0xE2
               # Replace Windows newlines with Unix newlines#define MAADR2   0xE3
               # Replace Windows newlines with Unix newlines#define MAADR5   0xE4
               # Replace Windows newlines with Unix newlines#define MAADR4   0xE5
               # Replace Windows newlines with Unix newlines#define EBSTSD   0x66
               # Replace Windows newlines with Unix newlines#define EBSTCON  0x67
               # Replace Windows newlines with Unix newlines#define EBSTCSL  0x68
               # Replace Windows newlines with Unix newlines#define EBSTCSH  0x69
               # Replace Windows newlines with Unix newlines#define MISTAT   0xEA
               # Replace Windows newlines with Unix newlines#define EREVID   0x72
               # Replace Windows newlines with Unix newlines#define ECOCON   0x75
               # Replace Windows newlines with Unix newlines#define EFLOCON  0x77
               # Replace Windows newlines with Unix newlines#define EPAUSL   0x78
               # Replace Windows newlines with Unix newlines#define EPAUSH   0x79
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// PHY Registers
               # Replace Windows newlines with Unix newlines#define PHCON1    0x00
               # Replace Windows newlines with Unix newlines#define PHSTAT1   0x01
               # Replace Windows newlines with Unix newlines#define PHHID1    0x02
               # Replace Windows newlines with Unix newlines#define PHHID2    0x03
               # Replace Windows newlines with Unix newlines#define PHCON2    0x10
               # Replace Windows newlines with Unix newlines#define PHSTAT2   0x11
               # Replace Windows newlines with Unix newlines#define PHIE      0x12
               # Replace Windows newlines with Unix newlines#define PHIR      0x13
               # Replace Windows newlines with Unix newlines#define PHLCON    0x14
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// ERXFCON bit definitions
               # Replace Windows newlines with Unix newlines#define UCEN      0x80
               # Replace Windows newlines with Unix newlines#define ANDOR     0x40
               # Replace Windows newlines with Unix newlines#define CRCEN     0x20
               # Replace Windows newlines with Unix newlines#define PMEN      0x10
               # Replace Windows newlines with Unix newlines#define MPEN      0x08
               # Replace Windows newlines with Unix newlines#define HTEN      0x04
               # Replace Windows newlines with Unix newlines#define MCEN      0x02
               # Replace Windows newlines with Unix newlines#define BCEN      0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// EIE bit definitions
               # Replace Windows newlines with Unix newlines#define INTIE     0x80
               # Replace Windows newlines with Unix newlines#define PKTIE     0x40
               # Replace Windows newlines with Unix newlines#define DMAIE     0x20
               # Replace Windows newlines with Unix newlines#define LINKIE    0x10
               # Replace Windows newlines with Unix newlines#define TXIE      0x08
               # Replace Windows newlines with Unix newlines#define WOLIE     0x04
               # Replace Windows newlines with Unix newlines#define TXERIE    0x02
               # Replace Windows newlines with Unix newlines#define RXERIE    0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// EIR bit definitions
               # Replace Windows newlines with Unix newlines#define PKTIF     0x40
               # Replace Windows newlines with Unix newlines#define DMAIF     0x20
               # Replace Windows newlines with Unix newlines#define LINKIF    0x10
               # Replace Windows newlines with Unix newlines#define TXIF      0x08
               # Replace Windows newlines with Unix newlines#define WOLIF     0x04
               # Replace Windows newlines with Unix newlines#define TXERIF    0x02
               # Replace Windows newlines with Unix newlines#define RXERIF    0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// ESTAT bit definitions
               # Replace Windows newlines with Unix newlines#define INT       0x80
               # Replace Windows newlines with Unix newlines#define LATECOL   0x10
               # Replace Windows newlines with Unix newlines#define RXBUSY    0x04
               # Replace Windows newlines with Unix newlines#define TXABRT    0x02
               # Replace Windows newlines with Unix newlines#define CLKRDY    0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// ECON2 bit definitions
               # Replace Windows newlines with Unix newlines#define AUTOINC   0x80
               # Replace Windows newlines with Unix newlines#define PKTDEC    0x40
               # Replace Windows newlines with Unix newlines#define PWRSV     0x20
               # Replace Windows newlines with Unix newlines#define VRPS      0x08
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// ECON1 bit definitions
               # Replace Windows newlines with Unix newlines#define TXRST     0x80
               # Replace Windows newlines with Unix newlines#define RXRST     0x40
               # Replace Windows newlines with Unix newlines#define DMAST     0x20
               # Replace Windows newlines with Unix newlines#define CSUMEN    0x10
               # Replace Windows newlines with Unix newlines#define TXRTS     0x08
               # Replace Windows newlines with Unix newlines#define RXEN      0x04
               # Replace Windows newlines with Unix newlines#define BSEL1     0x02
               # Replace Windows newlines with Unix newlines#define BSEL0     0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// MACON1 bit definitions
               # Replace Windows newlines with Unix newlines#define LOOPBK    0x10
               # Replace Windows newlines with Unix newlines#define TXPAUS    0x08
               # Replace Windows newlines with Unix newlines#define RXPAUS    0x04
               # Replace Windows newlines with Unix newlines#define PASSALL   0x02
               # Replace Windows newlines with Unix newlines#define MARXEN    0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// MACON2 bit definitions
               # Replace Windows newlines with Unix newlines#define MARST     0x80
               # Replace Windows newlines with Unix newlines#define RNDRST    0x40
               # Replace Windows newlines with Unix newlines#define MARXRST   0x08
               # Replace Windows newlines with Unix newlines#define RFUNRST   0x04
               # Replace Windows newlines with Unix newlines#define MATXRST   0x02
               # Replace Windows newlines with Unix newlines#define TFUNRST   0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// MACON3 bit definitions
               # Replace Windows newlines with Unix newlines#define PADCFG2   0x80
               # Replace Windows newlines with Unix newlines#define PADCFG1   0x40
               # Replace Windows newlines with Unix newlines#define PADCFG0   0x20
               # Replace Windows newlines with Unix newlines#define TXCRCEN   0x10
               # Replace Windows newlines with Unix newlines#define PHDRLEN   0x08
               # Replace Windows newlines with Unix newlines#define HFRMLEN   0x04
               # Replace Windows newlines with Unix newlines#define FRMLNEN   0x02
               # Replace Windows newlines with Unix newlines#define FULDPX    0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// MICMD bit definitions
               # Replace Windows newlines with Unix newlines#define MIISCAN   0x02
               # Replace Windows newlines with Unix newlines#define MIIRD     0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// MISTAT bit definitions
               # Replace Windows newlines with Unix newlines#define NVALID    0x04
               # Replace Windows newlines with Unix newlines#define SCAN      0x02
               # Replace Windows newlines with Unix newlines#define BUSY      0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// PHCON1 bit definitions
               # Replace Windows newlines with Unix newlines#define PRST      0x8000
               # Replace Windows newlines with Unix newlines#define PLOOPBK   0x4000
               # Replace Windows newlines with Unix newlines#define PPWRSV    0x0800
               # Replace Windows newlines with Unix newlines#define PDPXMD    0x0100
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// PHSTAT1 bit definitions
               # Replace Windows newlines with Unix newlines#define PFDPX     0x1000
               # Replace Windows newlines with Unix newlines#define PHDPX     0x0800
               # Replace Windows newlines with Unix newlines#define LLSTAT    0x0004
               # Replace Windows newlines with Unix newlines#define JBSTAT    0x0002
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// PHCON2 bit definitions
               # Replace Windows newlines with Unix newlines#define FRCLINK   0x4000
               # Replace Windows newlines with Unix newlines#define TXDIS     0x2000
               # Replace Windows newlines with Unix newlines#define JABBER    0x0400
               # Replace Windows newlines with Unix newlines#define HDLDIS    0x0100
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Packet Control bit Definitions
               # Replace Windows newlines with Unix newlines#define PHUGEEN   0x08
               # Replace Windows newlines with Unix newlines#define PPADEN    0x04
               # Replace Windows newlines with Unix newlines#define PCRCEN    0x02
               # Replace Windows newlines with Unix newlines#define POVERRIDE 0x01
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// SPI Instruction Set
               # Replace Windows newlines with Unix newlines#define RCR 0x00 // Read Control Register
               # Replace Windows newlines with Unix newlines#define RBM 0x3A // Read Buffer Memory
               # Replace Windows newlines with Unix newlines#define WCR 0x40 // Write Control Register
               # Replace Windows newlines with Unix newlines#define WBM 0x7A // Write Buffer Memory
               # Replace Windows newlines with Unix newlines#define BFS 0x80 // Bit Field Set
               # Replace Windows newlines with Unix newlines#define BFC 0xA0 // Bit Field Clear
               # Replace Windows newlines with Unix newlines#define SC  0xFF // Soft Reset
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines// Buffer
               # Replace Windows newlines with Unix newlines#define RXSTART_INIT 0x0000
               # Replace Windows newlines with Unix newlines#define RXSTOP_INIT  (0x1FFF-0x0600-1)
               # Replace Windows newlines with Unix newlines#define TXSTART_INIT (0x1FFF-0x0600)
               # Replace Windows newlines with Unix newlines#define TXSTOP_INIT  0x1FFF
               # Replace Windows newlines with Unix newlines#define MAX_FRAMELEN 1500
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines//Functions
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_ReadOp(uint8_t op, uint8_t address);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_WriteOp(uint8_t op, uint8_t address, uint8_t data);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_ReadBuffer(uint16_t len, uint8_t* data);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_WriteBuffer(uint16_t len, uint8_t* data);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_SetBank(uint8_t address);
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_Read(uint8_t address);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_Write(uint8_t address, uint8_t data);
               # Replace Windows newlines with Unix newlinesuint16_t ENC28J60_PhyRead(uint8_t address);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_PhyWrite(uint8_t address, uint16_t data);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_ClkOut(uint8_t clk);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_Init(uint8_t* macaddr);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_PacketSend(uint16_t len, uint8_t* packet);
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_HasRxPkt(void);
               # Replace Windows newlines with Unix newlinesuint16_t ENC28J60_PacketReceive(uint16_t maxlen, uint8_t* packet);
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_GetRev(void);
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_LinkUp(void);
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_reset(void);
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines#endif
               # Replace Windows newlines with Unix newlines
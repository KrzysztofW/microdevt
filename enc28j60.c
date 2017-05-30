#include <avr/io.h>
               # Replace Windows newlines with Unix newlines#include <util/delay.h>
               # Replace Windows newlines with Unix newlines#include "enc28j60.h"
               # Replace Windows newlines with Unix newlines#include "../conf/config.h"
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesstatic uint8_t ENC28J60_Bank;
               # Replace Windows newlines with Unix newlinesstatic uint16_t gNextPacketPtr;
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid csactive(void) {
               # Replace Windows newlines with Unix newlines    SPI_PORT &= ~(1<<SPI_CS);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid cspassive(void) {
               # Replace Windows newlines with Unix newlines    SPI_PORT |= (1<<SPI_CS);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid waitspi(void) {
               # Replace Windows newlines with Unix newlines    while(!(SPSR&(1<<SPIF)));
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_ReadOp(uint8_t op,uint8_t address) {
               # Replace Windows newlines with Unix newlines    csactive();
               # Replace Windows newlines with Unix newlines    SPDR = op | (address & ADDR_MASK);
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    SPDR = 0x00;
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    if(address & 0x80) {
               # Replace Windows newlines with Unix newlines        SPDR = 0x00;
               # Replace Windows newlines with Unix newlines        waitspi();
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    cspassive();
               # Replace Windows newlines with Unix newlines    return(SPDR);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_WriteOp(uint8_t op,uint8_t address,uint8_t data) {
               # Replace Windows newlines with Unix newlines    csactive();
               # Replace Windows newlines with Unix newlines    SPDR = op | (address & ADDR_MASK);
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    SPDR = data;
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    cspassive();
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_ReadBuffer(uint16_t len,uint8_t* data) {
               # Replace Windows newlines with Unix newlines    csactive();
               # Replace Windows newlines with Unix newlines    SPDR = RBM;
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    while(len) {
               # Replace Windows newlines with Unix newlines        len--;
               # Replace Windows newlines with Unix newlines        SPDR = 0x00;
               # Replace Windows newlines with Unix newlines        waitspi();
               # Replace Windows newlines with Unix newlines        *data = SPDR;
               # Replace Windows newlines with Unix newlines        data++;
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    *data='\0';
               # Replace Windows newlines with Unix newlines    cspassive();
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_WriteBuffer(uint16_t len,uint8_t* data) {
               # Replace Windows newlines with Unix newlines    csactive();
               # Replace Windows newlines with Unix newlines    SPDR = WBM;
               # Replace Windows newlines with Unix newlines    waitspi();
               # Replace Windows newlines with Unix newlines    while(len) {
               # Replace Windows newlines with Unix newlines        len--;
               # Replace Windows newlines with Unix newlines        SPDR = *data;
               # Replace Windows newlines with Unix newlines        data++;
               # Replace Windows newlines with Unix newlines        waitspi();
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    cspassive();
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_SetBank(uint8_t address) {
               # Replace Windows newlines with Unix newlines    if((address & BANK_MASK) != ENC28J60_Bank) {
               # Replace Windows newlines with Unix newlines        ENC28J60_WriteOp(BFC,ECON1,(BSEL1|BSEL0));
               # Replace Windows newlines with Unix newlines        ENC28J60_WriteOp(BFS,ECON1,(address & BANK_MASK)>>5);
               # Replace Windows newlines with Unix newlines        ENC28J60_Bank = (address & BANK_MASK);
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_Read(uint8_t address) {
               # Replace Windows newlines with Unix newlines    ENC28J60_SetBank(address);
               # Replace Windows newlines with Unix newlines    return ENC28J60_ReadOp(RCR,address);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_Write(uint8_t address,uint8_t data) {
               # Replace Windows newlines with Unix newlines    ENC28J60_SetBank(address);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(WCR,address,data);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t ENC28J60_PhyRead(uint8_t address) {
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MIREGADR,address);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MICMD,MIIRD);
               # Replace Windows newlines with Unix newlines    _delay_us(15);
               # Replace Windows newlines with Unix newlines    while(ENC28J60_Read(MISTAT) & BUSY);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MICMD,0x00);
               # Replace Windows newlines with Unix newlines    return(ENC28J60_Read(MIRDH));
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_PhyWrite(uint8_t address,uint16_t data) {
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MIREGADR,address);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MIWRL,data);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MIWRH,data>>8);
               # Replace Windows newlines with Unix newlines    while(ENC28J60_Read(MISTAT) & BUSY) _delay_us(15);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_ClkOut(uint8_t clock) {
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ECOCON,clock & 0x7);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_Init(uint8_t* macaddr) {
               # Replace Windows newlines with Unix newlines    SPI_DDR |= (1<<SPI_CS);
               # Replace Windows newlines with Unix newlines    cspassive();
               # Replace Windows newlines with Unix newlines    SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK);
               # Replace Windows newlines with Unix newlines    SPI_DDR &= ~(1<<SPI_MISO);
               # Replace Windows newlines with Unix newlines    SPI_PORT &= ~(1<<SPI_MOSI);
               # Replace Windows newlines with Unix newlines    SPI_PORT &= ~(1<<SPI_SCK);
               # Replace Windows newlines with Unix newlines    SPCR = (1<<SPE)|(1<<MSTR);
               # Replace Windows newlines with Unix newlines    SPSR |= (1<<SPI2X);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(SC,0,SC);
               # Replace Windows newlines with Unix newlines    _delay_loop_1(205);
               # Replace Windows newlines with Unix newlines    gNextPacketPtr = RXSTART_INIT;
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXSTL,RXSTART_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXSTH,RXSTART_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXRDPTL,RXSTART_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXRDPTH,RXSTART_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXNDL,RXSTOP_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXNDH,RXSTOP_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXSTL,TXSTART_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXSTH,TXSTART_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXNDL,TXSTOP_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXNDH,TXSTOP_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXFCON,UCEN|CRCEN|PMEN);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EPMM0,0x3f);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EPMM1,0x30);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EPMCSL,0xf9);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EPMCSH,0xf7);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MACON1,(MARXEN|TXPAUS|RXPAUS));
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MACON2,0x00);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(BFS,MACON3,(PADCFG0|TXCRCEN|FRMLNEN));
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAIPGL,0x12);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAIPGH,0x0C);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MABBIPG,0x12);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAMXFLL,MAX_FRAMELEN&0xFF);    
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAMXFLH,MAX_FRAMELEN>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR5,macaddr[0]);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR4,macaddr[1]);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR3,macaddr[2]);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR2,macaddr[3]);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR1,macaddr[4]);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(MAADR0,macaddr[5]);
               # Replace Windows newlines with Unix newlines    ENC28J60_PhyWrite(PHCON2,HDLDIS);
               # Replace Windows newlines with Unix newlines    ENC28J60_SetBank(ECON1);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(BFS,EIE,(INTIE|PKTIE));
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(BFS,ECON1,RXEN);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_HasRxPkt(void) {
               # Replace Windows newlines with Unix newlines    if(ENC28J60_Read(EPKTCNT)==0) return(0);
               # Replace Windows newlines with Unix newlines    return(1);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint16_t ENC28J60_PacketReceive(uint16_t maxlen,uint8_t* packet) {
               # Replace Windows newlines with Unix newlines    uint16_t rxstat;
               # Replace Windows newlines with Unix newlines    uint16_t len;
               # Replace Windows newlines with Unix newlines    if(ENC28J60_Read(EPKTCNT)==0) return(0);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERDPTL,(gNextPacketPtr & 0xFF));
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERDPTH,(gNextPacketPtr)>>8);
               # Replace Windows newlines with Unix newlines    gNextPacketPtr  = ENC28J60_ReadOp(RBM,0);
               # Replace Windows newlines with Unix newlines    gNextPacketPtr |= ENC28J60_ReadOp(RBM,0)<<8;
               # Replace Windows newlines with Unix newlines    len  = ENC28J60_ReadOp(RBM,0);
               # Replace Windows newlines with Unix newlines    len |= ENC28J60_ReadOp(RBM,0)<<8;
               # Replace Windows newlines with Unix newlines    len-=4;
               # Replace Windows newlines with Unix newlines    rxstat  = ENC28J60_ReadOp(RBM,0);
               # Replace Windows newlines with Unix newlines    rxstat |= ((uint16_t)ENC28J60_ReadOp(RBM,0))<<8;
               # Replace Windows newlines with Unix newlines    if (len>maxlen-1) len=maxlen-1;
               # Replace Windows newlines with Unix newlines    if ((rxstat & 0x80)==0) len=0;
               # Replace Windows newlines with Unix newlines    else ENC28J60_ReadBuffer(len,packet);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXRDPTL,(gNextPacketPtr &0xFF));
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ERXRDPTH,(gNextPacketPtr)>>8);
               # Replace Windows newlines with Unix newlines    if ((gNextPacketPtr-1 < RXSTART_INIT)||(gNextPacketPtr-1 > RXSTOP_INIT)) {
               # Replace Windows newlines with Unix newlines        ENC28J60_Write(ERXRDPTL,(RXSTOP_INIT)&0xFF);
               # Replace Windows newlines with Unix newlines        ENC28J60_Write(ERXRDPTH,(RXSTOP_INIT)>>8);
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    else {
               # Replace Windows newlines with Unix newlines        ENC28J60_Write(ERXRDPTL,(gNextPacketPtr-1)&0xFF);
               # Replace Windows newlines with Unix newlines        ENC28J60_Write(ERXRDPTH,(gNextPacketPtr-1)>>8);
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(BFS,ECON2,PKTDEC);
               # Replace Windows newlines with Unix newlines    return(len);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesvoid ENC28J60_PacketSend(uint16_t len,uint8_t* packet) {
               # Replace Windows newlines with Unix newlines    while (ENC28J60_ReadOp(RCR,ECON1) & TXRTS) {
               # Replace Windows newlines with Unix newlines        if(ENC28J60_Read(EIR) & TXERIF) {
               # Replace Windows newlines with Unix newlines            ENC28J60_WriteOp(BFS,ECON1,TXRST);
               # Replace Windows newlines with Unix newlines            ENC28J60_WriteOp(BFC,ECON1,TXRST);
               # Replace Windows newlines with Unix newlines        }
               # Replace Windows newlines with Unix newlines    }
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EWRPTL,TXSTART_INIT&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(EWRPTH,TXSTART_INIT>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXNDL,(TXSTART_INIT+len)&0xFF);
               # Replace Windows newlines with Unix newlines    ENC28J60_Write(ETXNDH,(TXSTART_INIT+len)>>8);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(WBM,0,0x00);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteBuffer(len,packet);
               # Replace Windows newlines with Unix newlines    ENC28J60_WriteOp(BFS,ECON1,TXRTS);
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_GetRev(void) {
               # Replace Windows newlines with Unix newlines    return(ENC28J60_Read(EREVID));
               # Replace Windows newlines with Unix newlines}
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesuint8_t ENC28J60_LinkUp(void) {
               # Replace Windows newlines with Unix newlines    return(ENC28J60_PhyRead(PHSTAT2) && 4);
               # Replace Windows newlines with Unix newlines}
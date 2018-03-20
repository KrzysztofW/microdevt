#ifndef _RF_COMMON_H_
#define _RF_COMMON_H_

/* Modules' addresses */
#define RF_MOD0_HW_ADDR 0x69
#define RF_MOD1_HW_ADDR 0x70
#define RF_MOD2_HW_ADDR 0x71
#define RF_MOD3_HW_ADDR 0x72
#define RF_MOD4_HW_ADDR 0x73
#define RF_MOD5_HW_ADDR 0x74
#define RF_MOD6_HW_ADDR 0x75
#define RF_MOD7_HW_ADDR 0x76
#define RF_MOD8_HW_ADDR 0x77
#define RF_MOD9_HW_ADDR 0x78

/* Command types */
#define RF_CMD_PIR         0 /* move detector triggered */
#define RF_CMD_OC          1 /* open/close detector triggered */
#define RF_CMD_SH          2 /* shake detector triggered */
#define RF_CMD_PWR         3 /* power supply disconnected */
#define RF_CMD_PWR_BAT     4 /* low battery level cmd */
#define RF_CMD_USER1       5 /* generic cmd */
#define RF_CMD_USER2       6 /* generic cmd */
#define RF_CMD_USER3       7 /* generic cmd */
#define RF_CMD_USER4       8 /* generic cmd */
#define RF_CMD_USER5       9 /* generic cmd */
#define RF_CMD_USER6      10 /* generic cmd */

#define RF_CMD_ACK      0xFF /* cmd acknowledgment */

#define RF_BURST_NUMBER 2    /* send the same command twice */

/* try to keep this structure a multiple of 4 bytes for xxtea encryption */
typedef struct __attribute__((__packed__)) rf_cmd {
	uint8_t  seq_num;
	uint8_t  type;
	uint16_t info;
} rf_cmd_t;

#endif

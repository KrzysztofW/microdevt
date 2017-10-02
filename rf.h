#ifndef _RF_H_
#define _RF_H_

#define TIMER_RESOLUTION_US 150UL

int rf_init(void);
void rf_shutdown(void);
void rf_decode_cmds(void);

#endif

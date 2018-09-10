#ifndef _ALARM_MODULE1_H_
#define _ALARM_MODULE1_H_

void alarm_module1_rf_init(void);
void module_print_status(void);
void module_arm(uint8_t on);
void handle_rf_commands(uint8_t cmd, uint16_t value);

#endif

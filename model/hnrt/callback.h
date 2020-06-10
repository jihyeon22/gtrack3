#ifndef __MODEL_CIPRMC_CALLBACK_H__
#define __MODEL_CIPRMC_CALLBACK_H__

#include <util/transfer.h>

void init_model_callback(void);
void button2_callback(void);
void button1_callback(void);
void network_on_callback(void);
void ignition_on_callback(void);
void ignition_off_callback(void);
void power_on_callback(void);
void power_off_callback(void);
void gps_parse_one_context_callback(void);
void main_loop_callback(void);
void terminate_app_callback(void);
void exit_main_loop();

extern transferSetting_t gSetting_report;
extern transferSetting_t gSetting_request;

void network_fail_emergency_reset_callback(void);

#endif


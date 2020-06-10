/*
 * sms_msg_process.h
 *
 *  Created on: 2013. 3. 19.
 *      Author: ongten
 */

#ifndef SMS_MSG_PROCESS_H_
#define SMS_MSG_PROCESS_H_

int sms_set_mdt_svrip_info(char* sms_msg);
int sms_set_dtg_svrip_info(char* sms_msg);
int sms_set_mdt_period(char* sms_msg);
int sms_set_dtg_period(char* sms_msg);
int sms_set_device_reset(char* sms_msg);
int sms_set_dtg_setting_value1(char* sms_msg);
int sms_set_dtg_setting_value2(char* sms_msg);
int sms_device_status_req(char *sender);

#endif /* SMS_MSG_PROCESS_H_ */

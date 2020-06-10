/*
 * sms_msg_process.h
 *
 *  Created on: 2013. 3. 19.
 *      Author: ongten
 */

#ifndef SMS_MSG_PROCESS_H_
#define SMS_MSG_PROCESS_H_

int sms_set_svrip_info(char* sms_msg);
int sms_set_report_period1(char* sms_msg);
int sms_set_report_period2(char* sms_msg);
int sms_request_device_status(char* sms_msg);
int sms_set_gpio_mode(char* sms_msg);
int sms_set_gpio_output(char* sms_msg);
int sms_set_device_reset(char* sms_msg);
int sms_set_cumulative_distance(char* sms_msg);
int sms_set_geo_fence(char* sms_msg);

#endif /* SMS_MSG_PROCESS_H_ */

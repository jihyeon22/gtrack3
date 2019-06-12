#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

#define SMS_PW_SETIP				"mdt800ip"
#define SMS_PW_RESET				"m2m%flemgkwk*2006"

#define SMS_CMD_RESPONSE_SMS		0
#define SMS_CMD_RESPONSE_TCP		1

#define szSMS_IP_SETUP "&1"
#define szSMS_REPORT_CYCLE_SETUP "&2"
#define szSMS_REPORT2_CYCLE_SETUP "&11"
#define szSMS_REQUEST_STATUS "&ms"
#define szSMS_REPORT_DM_SETUP "&dm"

int parse_model_sms(char *time, char *phonenum, char *sms);

#endif


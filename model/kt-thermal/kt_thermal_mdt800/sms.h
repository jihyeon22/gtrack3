#ifndef __MDT800_SMS_H__
#define __MDT800_SMS_H__


// 기본은 다음의 설정을 따른다.
#define SMS_PW_SETIP				"ktp800ip"
#define SMS_PW_RESET				"ktp%flemgkwk*2020"
#define SMS_CMD_RESPONSE_NEED		1

typedef enum SmsType SmsType_t;
enum SmsType
{
	eSMS_DEFAILT = 0,
	eSMS_IP_SETUP = 1,              /* IP Set Message */
	eSMS_REPORT_CYCLE_SETUP = 2,    /* Report Cycle Set Message */
	eSMS_GET_VERSION = 3,			/* for KT certification */
	eMSG_MDT_RESET = 4,            /* MDT Reset Message */
	eMSG_END
};

int parse_model_sms(char *time, char *phonenum, char *sms);

#endif


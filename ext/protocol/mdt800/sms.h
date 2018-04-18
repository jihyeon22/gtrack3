#ifndef __MDT800_SMS_H__
#define __MDT800_SMS_H__

#ifdef SERVER_ABBR_BICD
#error "BICD MODEL IS NOT USE THIS CODE"
#endif

#ifdef CORP_ABBR_HNR
// 하누리티엔의 경우 mdt 프로토콜은 같지만 
// pw 가 다르다.
	#define SMS_PW_SETIP				"#ipSet#@"
	#define SMS_PW_RESET				"m2m%eksakffltpt*@"
	#define SMS_CMD_RESPONSE_NEED		1
#else
// 기본은 다음의 설정을 따른다.
	#define SMS_PW_SETIP				"mdt800ip"
	#define SMS_PW_RESET				"m2m%flemgkwk*2006"
	#define SMS_CMD_RESPONSE_NEED		1
#endif

typedef enum SmsType SmsType_t;
enum SmsType
{
	eSMS_DEFAILT = 0,
	eSMS_IP_SETUP = 1,              /* IP Set Message */
	eSMS_REPORT_CYCLE_SETUP = 2,    /* Report Cycle Set Message */
	eSMS_ODO_SETUP = 3,             /* Count Distance Set Message */

	eSMS_MDT_STATUS = 4,            /* MDT Status Reqeust Message */
	eSMS_GPIO_MODE = 5,             /* GPIO Mode Set Message */
	eSMS_GPIO_OUTPUT = 6,           /* GPIO Output Set Message */

	eMSG_MDT_RESET = 10,            /* MDT Reset Message */
	eSMS_REPORT2_CYCLE_SETUP = 11,  /* Second Report Cycle Set Message */
	eSMS_GEO_FENCE_SET = 12,        /* Geo Fence Set Message */

	eSMS_GET_VERSION = 100,			/* for KT certification */
	eSMS_GET_MODEM_STATUS = 300,	/* for KT certification */
	
	eMSG_END
};

int parse_model_sms(char *time, char *phonenum, char *sms);

#endif


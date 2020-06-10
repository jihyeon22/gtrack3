#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

#define SMS_PW_SETIP	"mdt800ip"
#define SMS_PW_RESET	"m2m%flemgkwk*2006"

typedef enum ciprmcSmsType ciprmcSmsType_t;
enum ciprmcSmsType
{
	eMSG_DEFAILT = 0,
	eMSG_IP_SET,                         /* IP Set Message */
	eMSG_RPT_CYCLE,                   /* Report Cycle Set Message */
	eMSG_COT_DISTANCE,            /* Count Distance Set Message */
	eMSG_MDT_STATUS,                /* MDT Status Reqeust Message */
	eMSG_GPIO_MODE,                  /* GPIO Mode Set Message */
	eMSG_GPIO_OUTPUT,              /* GPIO Output Set Message */
	eMSG_MDT_RESET = 10,                  /* MDT Reset Message */
	eMSG_CYCLE_TERM,                 /* Second Report Cycle Set Message */
	eMSG_FENCE_SET,                   /* Geo Fence Set Message */
	eMSG_IP_SET_REQ,
	eMSG_SYS_STATUS_REQ = 1000,
	eMSG_END
};

int parse_model_sms(char *time, char *phonenum, char *sms);

#endif


#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

int parse_model_sms(char *time, char *phonenum, char *sms);

#define SMS_CMD_SET__IP_INFO			"&IP"
#define SMS_CMD_SET__MDT_RESET			"&RESET"
#define SMS_CMD_SET__SET_INTERVAL		"&TIME"
#define SMS_CMD_GET__GET_SETTING		"&SHOW"
#define SMS_CMD_GET__DTG_STATUS			"&DTG"

typedef enum
{
	eSMS_CMD_SET__IP_INFO,
	eSMS_CMD_SET__MDT_RESET,
	eSMS_CMD_SET__SET_INTERVAL,
	eSMS_CMD_GET__GET_SETTING,
	eSMS_CMD_GET__DTG_STATUS,
	MAX_SMS_CMD,
}SMS_CMD_INDEX;

typedef struct
{
	int index;
    unsigned char * cmd;
    int (*proc_func)(int argc, char* argv[], const char* phonenum);
}SMS_CMD_FUNC_T;

int _sms_cmd_proc_set_ip_info(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_reset(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_packet_interval(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_setting(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_dtg_status(int argc, char* argv[], const char* phonenum);

#endif

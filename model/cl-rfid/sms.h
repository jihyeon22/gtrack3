#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

int parse_model_sms(char *time, char *phonenum, char *sms);

#define SMS_CMD_GET__DEVICE_INFO		    "&0G"
#define SMS_CMD_SET__DEVICE_CLR_REDOWN		"&CLR"
#define SMS_CMD_SET__DEVICE_RESET		    "&RST"
#define SMS_CMD_GET__RFID_FW_VER	        "&RFVER"
#define SMS_CMD_SET__RFID_FW_DOWNLOAD       "&RFDN"


#define SMS_CMD_PWD				"cl2k"

typedef enum
{
	eSMS_CMD_GET__DEVICE_INFO,
    eSMS_CMD_SET__DEVICE_CLR_REDOWN,
    eSMS_CMD_SET__DEVICE_CLR,
    eSMS_CMD_SET__DEVICE_RESET,
    eSMS_CMD_GET__RFID_FW_VER,
    eSMS_CMD_SET__RFID_FW_DOWNLOAD,
	MAX_SMS_CMD,
}SMS_CMD_INDEX;

typedef struct
{
	int index;
    unsigned char * cmd;
    int (*proc_func)(int argc, char* argv[], const char* phonenum);
}SMS_CMD_FUNC_T;

int _sms_cmd_proc_get_dev_info(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_clear_redown_rfid(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_clear(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_device_reset(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_rfid_fw_ver(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_rfid_fw_down(int argc, char* argv[], const char* phonenum);



#endif

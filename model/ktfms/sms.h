#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

int parse_model_sms(char *time, char *phonenum, char *sms);

#define SMS_CMD_SET__DEVICE_INFO		"&0S"
#define SMS_CMD_GET__DEVICE_INFO		"&0G"

#define SMS_CMD_SET__DEVICE_INIT_1		"&I1"
#define SMS_CMD_SET__DEVICE_INIT_2		"&I2"

#define SMS_CMD_SET__IP_PORT			"&1S"
#define SMS_CMD_GET__IP_PORT			"&1G"
#define SMS_CMD_SET__CAR_VIN			"&2S"
#define SMS_CMD_GET__CAR_VIN			"&2G"
#define SMS_CMD_SET__CAR_NUM			"&3S"
#define SMS_CMD_GET__CAR_NUM			"&3G"
#define SMS_CMD_SET__COMPANY_ID			"&4S"
#define SMS_CMD_GET__COMPANY_ID			"&4G"
#define SMS_CMD_SET__TOTAL_TRIP			"&5S"
#define SMS_CMD_GET__TOTAL_TRIP			"&5G"

#define SMS_CMD_GET__DEV_STAT			"&DG"

#define SMS_CMD_SET__DEV_RESET_1		"&RS1"
#define SMS_CMD_SET__DEV_RESET_2		"&RS2"

#define SMS_CMD_SET__S_FOTA_DM_SERVER			"&KFS"
#define SMS_CMD_GET__S_FOTA_DM_SERVER			"&KFG"
#define SMS_CMD_SET__S_FOTA_QTY_SERVER			"&KQS"
#define SMS_CMD_GET__S_FOTA_QTY_SERVER			"&KQG"

#define SMS_CMD_TEST__PWR_MODE_ON			"&TON"
#define SMS_CMD_TEST__PWR_MODE_OFF			"&TOFF"
#define SMS_CMD_TEST__CASE_1				"&TC1"
#define SMS_CMD_TEST__CASE_2				"&TC2"

#define SMS_CMD_TEST__TEST_DEBUG_1				"&TD1"


#define SMS_CMD_PWD				"netkti2wf"

typedef enum
{
	eSMS_CMD_SET__DEVICE_INFO,
	eSMS_CMD_GET__DEVICE_INFO,
	eSMS_CMD_SET__DEVICE_INIT_1,
	eSMS_CMD_SET__DEVICE_INIT_2,
	eSMS_CMD_SET__IP_PORT,
	eSMS_CMD_GET__IP_PORT,
	eSMS_CMD_SET__CAR_VIN,
	eSMS_CMD_GET__CAR_VIN,
	eSMS_CMD_SET__CAR_NUM,
	eSMS_CMD_GET__CAR_NUM,
	eSMS_CMD_SET__COMPANY_ID,
	eSMS_CMD_GET__COMPANY_ID,
	eSMS_CMD_SET__TOTAL_TRIP,
	eSMS_CMD_GET__TOTAL_TRIP,
	
	eSMS_CMD_GET__DEV_STAT,
	
	eSMS_CMD_SET__DEV_RESET_1,
	eSMS_CMD_SET__DEV_RESET_2,
	
	eSMS_CMD_SET__S_FOTA_DM_SERVER,
	eSMS_CMD_GET__S_FOTA_DM_SERVER,
	eSMS_CMD_SET__S_FOTA_QTY_SERVER,
	eSMS_CMD_GET__S_FOTA_QTY_SERVER,
	
	eSMS_CMD_TEST__PWR_MODE_ON,
	eSMS_CMD_TEST__PWR_MODE_OFF,
	eSMS_CMD_TEST__CASE_1,
	eSMS_CMD_TEST__CASE_2,
	
	eSMS_CMD_TEST__TEST_DEBUG_1,
	
	MAX_SMS_CMD,
}SMS_CMD_INDEX;

typedef struct
{
	int index;
    unsigned char * cmd;
    int (*proc_func)(int argc, char* argv[], const char* phonenum);
}SMS_CMD_FUNC_T;

int _sms_cmd_proc_set_dev_info(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_dev_info(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_dev_init_1(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_dev_init_2(int argc, char* argv[], const char* phonenum);


int _sms_cmd_proc_set_ip_port(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_ip_port(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_car_vin(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_car_vin(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_car_num(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_car_num(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_company_id(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_company_id(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_total_trip(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_total_trip(int argc, char* argv[], const char* phonenum);

int _sms_cmd_proc_get_dev_stat(int argc, char* argv[], const char* phonenum);


int _sms_cmd_proc_set_reset_1(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_reset_2(int argc, char* argv[], const char* phonenum);

int _sms_cmd_proc_set_s_fota_dm_server(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_s_fota_dm_server(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_set_s_fota_qty_server(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_get_s_fota_qty_server(int argc, char* argv[], const char* phonenum);

int _sms_cmd_proc_test_pwr_mode_on(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_test_pwr_mode_off(int argc, char* argv[], const char* phonenum);
//int _sms_cmd_proc_test_server_policy_remove(int argc, char* argv[], const char* phonenum);

int _sms_cmd_proc_test_case_1(int argc, char* argv[], const char* phonenum);
int _sms_cmd_proc_test_case_2(int argc, char* argv[], const char* phonenum);

int _sms_cmd_proc_test_debug_1(int argc, char* argv[], const char* phonenum);

#endif

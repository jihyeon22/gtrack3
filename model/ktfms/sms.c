#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/thread.h>
#include <board/power.h>
#include <util/tools.h>
#include <config.h>

#include "include/defines.h"
#include "util/nettool.h"

#include "logd/logd_rpc.h"
#include "util/validation.h"

#include <at/at_util.h>
#include "sms.h"
#include "kt_fms_packet.h"

//#include "base/kt_fota_config.h"
#include "seco_obd.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static SMS_CMD_FUNC_T sms_cmd_func[] =
{
	{eSMS_CMD_SET__DEVICE_INFO, SMS_CMD_SET__DEVICE_INFO, _sms_cmd_proc_set_dev_info},
	{eSMS_CMD_GET__DEVICE_INFO, SMS_CMD_GET__DEVICE_INFO, _sms_cmd_proc_get_dev_info},
	{eSMS_CMD_SET__DEVICE_INIT_1, SMS_CMD_SET__DEVICE_INIT_1, _sms_cmd_proc_set_dev_init_1},
	{eSMS_CMD_SET__DEVICE_INIT_2, SMS_CMD_SET__DEVICE_INIT_2, _sms_cmd_proc_set_dev_init_2},
	
    {eSMS_CMD_SET__IP_PORT, SMS_CMD_SET__IP_PORT, _sms_cmd_proc_set_ip_port},
	{eSMS_CMD_GET__IP_PORT, SMS_CMD_GET__IP_PORT, _sms_cmd_proc_get_ip_port},
	{eSMS_CMD_SET__CAR_VIN, SMS_CMD_SET__CAR_VIN, _sms_cmd_proc_set_car_vin},
	{eSMS_CMD_GET__CAR_VIN, SMS_CMD_GET__CAR_VIN, _sms_cmd_proc_get_car_vin},
	{eSMS_CMD_SET__CAR_NUM, SMS_CMD_SET__CAR_NUM, _sms_cmd_proc_set_car_num},
	{eSMS_CMD_GET__CAR_NUM, SMS_CMD_GET__CAR_NUM, _sms_cmd_proc_get_car_num},
	{eSMS_CMD_SET__COMPANY_ID, SMS_CMD_SET__COMPANY_ID, _sms_cmd_proc_set_company_id},
	{eSMS_CMD_GET__COMPANY_ID, SMS_CMD_GET__COMPANY_ID, _sms_cmd_proc_get_company_id},
	{eSMS_CMD_SET__TOTAL_TRIP, SMS_CMD_SET__TOTAL_TRIP, _sms_cmd_proc_set_total_trip},
	{eSMS_CMD_GET__TOTAL_TRIP, SMS_CMD_GET__TOTAL_TRIP, _sms_cmd_proc_get_total_trip},
	
	{eSMS_CMD_GET__DEV_STAT, SMS_CMD_GET__DEV_STAT, _sms_cmd_proc_get_dev_stat},
	
	{eSMS_CMD_SET__DEV_RESET_1, SMS_CMD_SET__DEV_RESET_1, _sms_cmd_proc_set_reset_1},
	{eSMS_CMD_SET__DEV_RESET_2, SMS_CMD_SET__DEV_RESET_2, _sms_cmd_proc_set_reset_2},

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
	{eSMS_CMD_SET__S_FOTA_DM_SERVER, SMS_CMD_SET__S_FOTA_DM_SERVER, _sms_cmd_proc_set_s_fota_dm_server},
	{eSMS_CMD_GET__S_FOTA_DM_SERVER, SMS_CMD_GET__S_FOTA_DM_SERVER, _sms_cmd_proc_get_s_fota_dm_server},
	{eSMS_CMD_SET__S_FOTA_QTY_SERVER, SMS_CMD_SET__S_FOTA_QTY_SERVER, _sms_cmd_proc_set_s_fota_qty_server},
	{eSMS_CMD_GET__S_FOTA_QTY_SERVER, SMS_CMD_GET__S_FOTA_QTY_SERVER, _sms_cmd_proc_get_s_fota_qty_server},
#endif

	{eSMS_CMD_TEST__PWR_MODE_ON, SMS_CMD_TEST__PWR_MODE_ON, _sms_cmd_proc_test_pwr_mode_on},
	{eSMS_CMD_TEST__PWR_MODE_ON, SMS_CMD_TEST__PWR_MODE_OFF, _sms_cmd_proc_test_pwr_mode_off},
	//{eSMS_CMD_TEST__SERVER_POLICY_REMOVE, SMS_CMD_TEST__SERVER_POLICY_REMOVE, _sms_cmd_proc_test_server_policy_remove},
	
	{eSMS_CMD_TEST__CASE_1, SMS_CMD_TEST__CASE_1, _sms_cmd_proc_test_case_1},
	{eSMS_CMD_TEST__CASE_2, SMS_CMD_TEST__CASE_2, _sms_cmd_proc_test_case_2},
	
	{eSMS_CMD_TEST__TEST_DEBUG_1, SMS_CMD_TEST__TEST_DEBUG_1, _sms_cmd_proc_test_debug_1},
	
};

int parse_model_sms(char *time, char *phonenum, char *sms)
{
//	int msg_type = 0;
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};

	char *base = 0;
	int len = 0;
	int i = 0;
	
	int found_cmd = 0;

	base = sms;
	len = strlen(sms);
	
	model_argv[model_argc++] = base;

	while(model_argc <= 32 && len--) 
	{
		switch(*base) {
			case ':':
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case ',':
				*base = '\0';
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case '\r':
			case '\n':
				*base = '\0';
				break;
			default:
				break;
		}
		base++;
	}
	model_argc--;
	
	for(i = 0; i <= model_argc; i++)
	{
		LOGD(LOG_TARGET, "%d %s\n", i, model_argv[i]);
	}

	if(model_argv[0] == NULL)
	{
		return -1;
	}
	
	// Command prefix always place in 1st argv.

	// find function..
	for(i = 0; i < MAX_SMS_CMD; i++)
	{
		if  (!( strncasecmp ( model_argv[0], sms_cmd_func[i].cmd, strlen(sms_cmd_func[i].cmd) ) ))
		{
			found_cmd = 1;
			break;
		}
	}

	if (found_cmd)
	{
		if ( sms_cmd_func[i].proc_func != NULL )
			sms_cmd_func[i].proc_func(model_argc, model_argv, phonenum);
	}
	
	return ret;
}


int _sms_cmd_proc_set_dev_info(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	char url_path[64] = {0,};
	
	int success = 1;
	
	char* p_arg = NULL;
	
	LOGI(LOG_TARGET, "SMS PROC : set - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 5 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 5);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// -----------------------
	// ARG 2 : 차대번호 유효성 검사
	// -----------------------
	p_arg = argv[2];
	if (( p_arg == NULL ) || (strlen(p_arg) > 17))
	{
		success = 0;
		goto FINISH;
	}
	
	// -----------------------
	// ARG 3 : 차량번호 유효성 검사
	// -----------------------
	p_arg = argv[3];
	if (( p_arg == NULL ) || (strlen(p_arg) > 12))
	{
		success = 0;
		goto FINISH;
	}
	
	
	// -----------------------
	// ARG 4 : 회사 id 유효성 검사
	// -----------------------
	p_arg = argv[4];
	if (( p_arg == NULL ) || (strlen(p_arg) > 16))
	{
		success = 0;
		goto FINISH;
	}
	
	// -----------------------------
	/*
	{
		iniData_t arr_data[3+1] =
		{
			{.name="car:car_vin"},
			{.name="car:car_no"},
			{.name="car:url_path"},
			{.name=NULL, .msg=NULL},
		};

		// 여기까지왔으면 모든 세팅완료
		//save_car_info_car_vin(argv[2]);
		arr_data[0].msg = argv[2];
		
		//save_car_info_car_no(argv[3]);
		arr_data[1].msg = argv[3];
		
		sprintf(url_path, "TFM_CT/%s/vid-rp", argv[4]);
		//set_car_info_url_path(url_path);
		arr_data[2].msg = url_path;
		
		save_config_array_user(arr_data);
	}
	*/
	
	if ( save_car_info_car_vin(argv[2]) == -1 )
		success = 0;
	
	sleep(1);
	
	if ( save_car_info_car_no(argv[3]) == -1 )
		success = 0;
	
	sleep(1);
	
	sprintf(url_path, "TFM_CT/%s/vid-rp", argv[4]);
	if ( set_car_info_url_path(url_path) == -1 )
		success = 0;
	
	sleep(1);
	
	// -----------------------
	// ARG 5 : 응답관련
	// -----------------------
 FINISH:
 
	if ( atoi(argv[5]) == 1 )
	{
		if ( success == 1)
			at_send_sms(phonenum, "SET DEV INFO OK");
		else 
			at_send_sms(phonenum, "SET DEV INFO FAIL -> retry");
	}
	
	
	return 0;
}

int _sms_cmd_proc_get_dev_info(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	memset(tmp_buff, 0x00, sizeof(tmp_buff));
	get_car_info_car_vin(tmp_buff);
	str_len += sprintf(result_buff + str_len, "%s | ", tmp_buff);
	//printf("result buff is [%s] / [%d]\r\n",result_buff, strlen);
	
	memset(tmp_buff, 0x00, sizeof(tmp_buff));
	get_car_info_car_no(tmp_buff);
	str_len += sprintf(result_buff + str_len, "%s | ", tmp_buff);
	//printf("result buff is [%s] / [%d]\r\n",result_buff, strlen);
	
	memset(tmp_buff, 0x00, sizeof(tmp_buff));
	get_car_info_url_path(tmp_buff);
	str_len += sprintf(result_buff + str_len, "%s", tmp_buff);
	
	//printf("result buff is [%s] / [%d]\r\n",result_buff, strlen);
	
	at_send_sms(phonenum, result_buff);
	
	return 0;
}

int _sms_cmd_proc_set_dev_init_1(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	LOGI(LOG_TARGET, "SMS PROC : set - dev init 1 / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	
	// -----------------------------
	{
		iniData_t arr_data[3+1] =
		{
			{.name="car:car_vin"},
			{.name="car:car_no"},
			{.name="car:url_path"},
			{.name=NULL, .msg=NULL},
		};

		// 여기까지왔으면 모든 세팅완료
		//save_car_info_car_vin(argv[2]);
		arr_data[0].msg = DEFAULT_FMS_CAR_VIN;
		
		//save_car_info_car_no(argv[3]);
		arr_data[1].msg = DEFAULT_FMS_CAR_NUM;
		
		//sprintf(url_path, "TFM_CT/%s/vid-rp", argv[4]);
		//set_car_info_url_path(url_path);
		arr_data[2].msg = DEFAULT_FMS_SUB_SERVER_PATH;
		
		save_config_array_user(arr_data);
	}
		
	at_send_sms(phonenum, "SET DEV INIT 1 OK => modem reset..");
	
	init_server_and_poweroff();
	return 0;
}

int _sms_cmd_proc_set_dev_init_2(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	char url_path[128] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : set - dev init 2 / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 3 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	
	// -----------------------------
	{
		iniData_t arr_data[3+1] =
		{
			{.name="car:car_vin"},
			{.name="car:car_no"},
			{.name="car:url_path"},
			{.name=NULL, .msg=NULL},
		};

		// 여기까지왔으면 모든 세팅완료
		//save_car_info_car_vin(argv[2]);
		arr_data[0].msg = DEFAULT_FMS_CAR_VIN2;
		
		//save_car_info_car_no(argv[3]);
		arr_data[1].msg = DEFAULT_FMS_CAR_NUM2;
		
		sprintf(url_path, "TFM_CT/%s/vid-rp", argv[2]);
		//set_car_info_url_path(url_path);
		arr_data[2].msg = url_path;
		
		save_config_array_user(arr_data);
	}
		
	at_send_sms(phonenum, "SET DEV INIT 2 OK => modem reset..");
	
	init_server_and_poweroff();
	return 0;
}



int _sms_cmd_proc_set_ip_port(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int success = 1;
	
	LOGI(LOG_TARGET, "SMS PROC : set - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 4 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	nettool_init_hostbyname_func();
	
	
	
	if ( set_server_ip(argv[2]) == -1 ) 
		success = 0;
	
	sleep(1);
	
	if ( set_server_port(atoi(argv[3])) == -1)
		success = 0;
	
	sleep(1);
	
	if ( atoi(argv[4]) == 1 )
	{
		if ( success == 1)
			at_send_sms(phonenum, "SET IP / PORT OK");
		else
			at_send_sms(phonenum, "SET IP / PORT FAIL -> try again");
	}
	
	set_change_server(1);
	
	return 0;
}

int _sms_cmd_proc_get_ip_port(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
	unsigned char tmp_buff[64] = {0,};
	unsigned char resp_buff[64] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	get_server_ip(tmp_buff);
	
	sprintf(resp_buff,"%s:%d", tmp_buff, get_server_port());
	
	at_send_sms(phonenum, resp_buff);
	
	return 0;
	
}

int _sms_cmd_proc_set_car_vin(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int success = 1;
	char* p_arg = NULL;
	//unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : set - car vin / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 3 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// -----------------------
	// ARG 2 : 차대번호 유효성 검사
	// -----------------------
	p_arg = argv[2];
	if (( p_arg == NULL ) || (strlen(p_arg) > 17))
	{
		success = 0;
		goto FINISH;
	}
	
	if ( save_car_info_car_vin(argv[2]) == -1)
		success = 0;
	
FINISH:
	if ( atoi(argv[3]) == 1 )
	{
		if ( success == 1)
			at_send_sms(phonenum, "SET CAR VIN OK");
		else
			at_send_sms(phonenum, "SET CAR VIN FAIL -> try again");
	}

	return 0;
	
}


int _sms_cmd_proc_get_car_vin(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	unsigned char tmp_buff[32] = {0,};
	unsigned char resp_buff[64] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - car vin / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	
	get_car_info_car_vin(tmp_buff);
	sprintf(resp_buff, "%s", tmp_buff);
	at_send_sms(phonenum, resp_buff);
	
	return 0;
}

int _sms_cmd_proc_set_car_num(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int success = 1;
	char* p_arg = NULL;
	
	LOGI(LOG_TARGET, "SMS PROC : set - car num / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 3 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	

	
	// -----------------------
	// ARG 3 : 차량번호 유효성 검사
	// -----------------------
	p_arg = argv[2];
	if (( p_arg == NULL ) || (strlen(p_arg) > 12))
	{
		success = 0;
		goto FINISH;
	}
	
	
	if ( save_car_info_car_no(argv[2]) == -1 )
		success = 0;
FINISH:
	if ( atoi(argv[3]) == 1 )
	{
		if (success == 1)
			at_send_sms(phonenum, "SET CAR NUM OK");
		else
			at_send_sms(phonenum, "SET CAR NUM FAIL -> try again");
	}
	
	return 0;
}

int _sms_cmd_proc_get_car_num(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - car num / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	get_car_info_car_no(tmp_buff);
	at_send_sms(phonenum, tmp_buff);
	return 0;
	
}


int _sms_cmd_proc_set_company_id(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	char url_path[128]= {0,};
	int success = 1;
	char* p_arg = NULL;
	
	LOGI(LOG_TARGET, "SMS PROC : set - company id / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 3 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// -----------------------
	// ARG 4 : 회사 id 유효성 검사
	// -----------------------
	p_arg = argv[2];
	if (( p_arg == NULL ) || (strlen(p_arg) > 16))
	{
		success = 0;
		goto FINISH;
	}
	
	sprintf(url_path, "TFM_CT/%s/vid-rp", argv[2]);
	
	if ( set_car_info_url_path(url_path) == -1)
		success = 0;
FINISH:
	if ( atoi(argv[3]) == 1 )
	{
		if ( success == 1 )
			at_send_sms(phonenum, "SET COMPNAY ID OK");
		else
			at_send_sms(phonenum, "SET COMPNAY ID FAIL -> try again..");
	}
	
	return 0;
}


int _sms_cmd_proc_get_company_id(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - car num / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	get_car_info_url_path(tmp_buff);
	at_send_sms(phonenum, tmp_buff);
	
	return 0;
}

#define MAX_WAIT_SETTING_SEC	13

int _sms_cmd_proc_set_total_trip(int argc, char* argv[], const char* phonenum)
{
	int i = MAX_WAIT_SETTING_SEC ;
	
	int set_success = 0;
	
	LOGI(LOG_TARGET, "SMS PROC : set - total trip / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 3 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// 내부 마일리지 파일을 세팅한다.
	mileage_set_m(atoi(argv[2]));
	mileage_write();

	// obd 세팅을 하기 위해서는 일단 동작이 되어야하는데,
	// main thread 가 세팅값 default 로 인해서 아예 시작도 못하고있는것이다.
	if ( get_last_obd_stat() == OBD_CMD_RET_INVALID_COND )
	{
		if ( atoi(argv[3]) == 1 )
		{
			at_send_sms(phonenum, "SET TOTAL TRIP FAIL => first setting car info ");
			return -1;
		}
	}
	
	/* // 무조건 세팅하게 한다
	if (( get_send_policy() == KT_FMS_SEND_POLICY__PWR_ON_EVENT ) || \
		( get_send_policy() == KT_FMS_SEND_POLICY__RUNNING ) || \
		( get_send_policy() == KT_FMS_SEND_POLICY__INIT_EVENT ) )
	{
		if ( atoi(argv[3]) == 1 )
		{
			at_send_sms(phonenum, "SET TOTAL TRIP FAIL => key off and try again.. ");
			return -1;
		}
	}
	*/
	
	req_trip_setting(atoi(argv[2]));
	
	i = MAX_WAIT_SETTING_SEC;
	
	while(i--)
	{
		if ( get_req_trip_setting_result() == 0 )
		{
			set_success = 1;
			break;
		}
		sleep(1);
	}

	
	if ( atoi(argv[3]) == 1 )
	{
		if ( set_success == 1)
		{
			// at_send_sms(phonenum, "SET TOTAL TRIP OK and reset..");
			// sleep(3);
			// init_server_and_poweroff2();
			at_send_sms(phonenum, "SET TOTAL TRIP OK");
		}
		else 
			at_send_sms(phonenum, "SET TOTAL TRIP FAIL => setting fail try again.. ");
	}
	
	// 여기까지왔으면 그냥 끝낸다.
	req_trip_setting(-1);
	return 0;
}

extern long long last_total_trip;
extern unsigned int last_total_fuel;

int _sms_cmd_proc_get_total_trip(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int buff_len = 0;
	long long daliy_trip;
	unsigned int daliy_fuel;
	char itoa_tmp_buff[32] = {0,};
	
	unsigned char tmp_buff[80] = {0,};
	
//	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - car num / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	get_daliy_trip(last_total_trip);
	get_daliy_fuel(last_total_fuel);
	
	// get daily trip
	{
		if( last_total_trip >= 0 )
		{
			sprintf(itoa_tmp_buff, "%lld", last_total_trip/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "t_trip(%.11s) | ", itoa_tmp_buff);
			daliy_trip = get_daliy_trip(last_total_trip);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "t_trip(FAIL) | ");
			daliy_trip = -1;
		}
		
		if( daliy_trip >= 0 )
		{
			sprintf(itoa_tmp_buff, "%lld", daliy_trip/1000  );
			buff_len += sprintf(tmp_buff + buff_len, "d_trip(%.11s) | ", itoa_tmp_buff);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "d_trip(FAIL) | ", itoa_tmp_buff);
		}
	}
	
	// get daily fuel
	{
		if( last_total_fuel >= 0 )
		{
			sprintf(itoa_tmp_buff, "%d", last_total_fuel/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "t_fuel(%.11s) | ", itoa_tmp_buff);
			daliy_fuel = get_daliy_fuel(last_total_fuel);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "t_fuel(FAIL) | ");
			daliy_fuel = -1;
		}
		
		if( daliy_fuel >= 0 )
		{
			sprintf(itoa_tmp_buff, "%d", daliy_fuel/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "d_fuel(%.11s)", itoa_tmp_buff);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "d_fuel(FAIL)");
		}
	}
	

	at_send_sms(phonenum, tmp_buff);
	return 0;
}


int _sms_cmd_proc_set_reset_1(int argc, char* argv[], const char* phonenum)
{
	int i = MAX_WAIT_SETTING_SEC ;
	
	int set_success = 0;
	
	LOGI(LOG_TARGET, "SMS PROC : set - total trip / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}

	
	if ( atoi(argv[2]) == 1 )
	{
		at_send_sms(phonenum, "MODEM RESET NOW..");
	}
	
	sleep(1);
	
	mileage_write();

	while(1)
	{
		system("poweroff");
		sleep(1);
	}
	
	return 0;
}

int _sms_cmd_proc_set_reset_2(int argc, char* argv[], const char* phonenum)
{
int i = MAX_WAIT_SETTING_SEC ;
	
	int set_success = 0;
	
	LOGI(LOG_TARGET, "SMS PROC : set - total trip / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid\n");
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}

	
	if ( atoi(argv[2]) == 1 )
	{
		at_send_sms(phonenum, "CLEAR INFO AND RESET...");
	}
	
	sleep(1);
	init_server_and_poweroff();
	
	return 0;
}


//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
int _sms_cmd_proc_set_s_fota_dm_server(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	LOGI(LOG_TARGET, "SMS PROC : set - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 4 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
	}
	
	nettool_init_hostbyname_func();
	
	set_kt_fota_dm_server_ip_addr(argv[2]);
	set_kt_fota_dm_server_port(atoi(argv[3]));
	
	save_ini_kt_fota_svc_info();
	
	if ( atoi(argv[4]) == 1 )
	{
		at_send_sms(phonenum, "SET FOTA DM IP / PORT OK");
	}
	
	return 0;
}
#endif


//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
int _sms_cmd_proc_get_s_fota_dm_server(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
//	unsigned char tmp_buff[64] = {0,};
	unsigned char resp_buff[64] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	
	sprintf(resp_buff,"%s:%d", get_kt_fota_dm_server_ip_addr(), get_kt_fota_dm_server_port());
	
	at_send_sms(phonenum, resp_buff );
	
	return 0;
	
}
#endif

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
int _sms_cmd_proc_set_s_fota_qty_server(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	LOGI(LOG_TARGET, "SMS PROC : set - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 4 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	nettool_init_hostbyname_func();
	
	set_kt_fota_qty_server_ip_addr(argv[2]);
	set_kt_fota_qty_server_port(atoi(argv[3]));
	
	save_ini_kt_fota_svc_info();
	
	if ( atoi(argv[4]) == 1 )
	{
		at_send_sms(phonenum, "SET FOTA QTY IP / PORT OK");
	}
	
	return 0;
}
#endif


//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
int _sms_cmd_proc_get_s_fota_qty_server(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
//	unsigned char tmp_buff[64] = {0,};
	unsigned char resp_buff[64] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - ip,port / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	//get_server_ip(tmp_buff);
	
	sprintf(resp_buff,"%s:%d", get_kt_fota_qty_server_ip_addr(), get_kt_fota_qty_server_port());
	
	at_send_sms(phonenum, resp_buff);
	
	return 0;
	
}
#endif

int _sms_cmd_proc_test_case_1(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	//unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS TEST ==> CASE1 / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	{
		server_policy_t tmp_server_policy = {0,};
		
		tmp_server_policy.policy_num = 1818;
		
		tmp_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt = 2;
		tmp_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt = 2;
		tmp_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec = 30;
		tmp_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt = 2;
		
		tmp_server_policy.pkt_send_interval_sec = 60;
		tmp_server_policy.pkt_collect_interval_sec = 1;
		
		convert_factor_str_to_id("A000|A001|A002|A003|A004|A005|A006|A007|A008|A009|A010|A011|A012|A013|A014|A015|A016|A017|A018|A019|A020|A021|A022|A023|A024|A025|A026|A027|A028|A029|A030|A031|A032|A033", &tmp_server_policy.sdr_factor);
		
		//strcpy(tmp_server_policy.sdr_factor,);
		
		set_server_policy(&tmp_server_policy);
	}

	
	
	return 0;

}

extern int day_change;

int _sms_cmd_proc_test_case_2(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	//unsigned char tmp_buff[32] = {0,};
	long long fuel_set = 0;
	
	LOGI(LOG_TARGET, "SMS TEST ==> CASE2 / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	fuel_set = 1818 * 1000;
	set_seco_obd_total_fuel(fuel_set);
	
	return 0;

}



int _sms_cmd_proc_test_pwr_mode_on(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	//unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS TEST : set poweron mode / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);
	return 0;
	
}

int _sms_cmd_proc_test_pwr_mode_off(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	//unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS TEST : set poweroff mode / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
	return 0;
}





int _sms_cmd_proc_get_dev_stat(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
//	unsigned char tmp_buff[64] = {0,};
	unsigned char resp_buff[64] = {0,};
	unsigned char obd_stat_str[16] = {0,};

	
	
	LOGI(LOG_TARGET, "SMS PROC : get - dev stat/ from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	//get_server_ip(tmp_buff);
	
	if ( get_use_obd_device() == 0 )
		sprintf(obd_stat_str, "%s", "NON-OBD");
	else
		sprintf(obd_stat_str, "%s", (g_last_dev_stat.obd_stat == 0 )? "FAIL":"OK");
		
	sprintf(resp_buff,"server(%d) | obd(%s) | key(%s) | rpm(%d) | speed(%d)", 
						g_last_dev_stat.svr_ret_code, 
						obd_stat_str,
						(g_last_dev_stat.obd_key == -1)? "FAIL" : ((g_last_dev_stat.obd_key == 0)? "OFF":"ON"),
						g_last_dev_stat.obd_rpm,
						g_last_dev_stat.obd_speed/1000);
	
	at_send_sms(phonenum, resp_buff);
	
	return 0;
}



int _sms_cmd_proc_test_debug_1(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
//	unsigned char tmp_buff[64] = {0,};
	unsigned char resp_buff[64] = {0,};
	unsigned char obd_stat_str[16] = {0,};
	
	long long daliy_trip;
	unsigned int daliy_fuel;
	int buff_len = 0;
	char itoa_tmp_buff[32] = {0,};
	
	unsigned char tmp_buff[128] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get web debug  / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	//get_server_ip(tmp_buff);
	
	// ------------------------------------
	
	sprintf(resp_buff,"server(%d) | obd(%s) | key(%s) | rpm(%d) | speed(%d)", 
						g_last_dev_stat.svr_ret_code, 
						(g_last_dev_stat.obd_stat == 0 )? "FAIL":"OK",
						(g_last_dev_stat.obd_key == -1)? "FAIL" : ((g_last_dev_stat.obd_key == 0)? "OFF":"ON"),
						g_last_dev_stat.obd_rpm,
						g_last_dev_stat.obd_speed/1000);
	
	//at_send_sms(phonenum, resp_buff, 10);
	#ifndef KT_FOTA_TEST_SVR
	devel_webdm_send_log(resp_buff);
	#endif
	
	// ------------------------------------
	
	get_daliy_trip(last_total_trip);
	get_daliy_fuel(last_total_fuel);
	
	// get daily trip
	{
		if( last_total_trip >= 0 )
		{
			sprintf(itoa_tmp_buff, "%lld", last_total_trip/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "t_trip(%.11s) | ", itoa_tmp_buff);
			daliy_trip = get_daliy_trip(last_total_trip);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "t_trip(FAIL) | ");
			daliy_trip = -1;
		}
		
		if( daliy_trip >= 0 )
		{
			sprintf(itoa_tmp_buff, "%lld", daliy_trip/1000  );
			buff_len += sprintf(tmp_buff + buff_len, "d_trip(%.11s) | ", itoa_tmp_buff);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "d_trip(FAIL) | ", itoa_tmp_buff);
		}
	}
	
	// get daily fuel
	{
		if( last_total_fuel >= 0 )
		{
			sprintf(itoa_tmp_buff, "%d", last_total_fuel/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "t_fuel(%.11s) | ", itoa_tmp_buff);
			daliy_fuel = get_daliy_fuel(last_total_fuel);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "t_fuel(FAIL) | ");
			daliy_fuel = -1;
		}
		
		if( daliy_fuel >= 0 )
		{
			sprintf(itoa_tmp_buff, "%d", daliy_fuel/1000 );
			buff_len += sprintf(tmp_buff + buff_len, "d_fuel(%.11s)", itoa_tmp_buff);
		}
		else
		{
			buff_len += sprintf(tmp_buff + buff_len, "d_fuel(FAIL)");
		}
	}
	
	#ifndef KT_FOTA_TEST_SVR
	devel_webdm_send_log(tmp_buff);
	#endif

	// ------------------------------------
	memset(tmp_buff,0x00,sizeof(tmp_buff));
	buff_len = 0;
	
	buff_len += sprintf(tmp_buff + buff_len, "AC err ret [%d], code [%d]", g_last_dev_stat.last_set_gender_spec_ret, g_last_dev_stat.last_set_gender_err_code);

	#ifndef KT_FOTA_TEST_SVR
	devel_webdm_send_log(tmp_buff);
	#endif
	
	return 0;
}











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
#include "netcom.h"

#include "logd/logd_rpc.h"
#include "util/validation.h"

#include <at/at_util.h>
#include "sms.h"

#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#include "cl_rfid_tools.h"

extern RFID_DEV_INFO_T g_rfid_dev_info;

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static SMS_CMD_FUNC_T sms_cmd_func[] =
{
	{eSMS_CMD_GET__DEVICE_INFO, SMS_CMD_GET__DEVICE_INFO, _sms_cmd_proc_get_dev_info},
	{eSMS_CMD_SET__DEVICE_CLR_REDOWN, SMS_CMD_SET__DEVICE_CLR_REDOWN, _sms_cmd_proc_clear_redown_rfid},
	{eSMS_CMD_SET__DEVICE_RESET, SMS_CMD_SET__DEVICE_RESET, _sms_cmd_proc_device_reset},
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

	printf("parse_model_sms start [%s]\r\n", sms);
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
		printf("model_argv[0] [%s], sms_cmd_func[%d].cmd => [%s]\r\n", model_argv[0], i, sms_cmd_func[i].cmd);
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
	
	printf("parse_model_sms end\r\n");
	return ret;
}

// ---------------------------------------------------------------

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
	sprintf(result_buff,"dev[%s],rfid_conn[%s],rfid_user[%d],last_up[%s]", 
			rfid_tool__get_senario_stat_str(), 
			rfid_tool__get_rifd_dev_stat_str(),
			g_rfid_dev_info.total_passenger_cnt,
			g_rfid_dev_info.saved_timestamp);
	
	at_send_sms(phonenum, result_buff);
	
	return 0;
}


int _sms_cmd_proc_clear_redown_rfid(int argc, char* argv[], const char* phonenum)
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
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}

	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	sprintf(result_buff,"all rfid user clean and redownload start");
	kjtec_rfid_mgr__clr_all_user_data();

	if ( atoi(argv[2]) == 1 )
		at_send_sms(phonenum, result_buff);
	
	return 0;
}


int _sms_cmd_proc_device_reset(int argc, char* argv[], const char* phonenum)
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
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}

	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	sprintf(result_buff,"reset : 1 min after…");
	

	if ( atoi(argv[2]) == 1 )
		at_send_sms(phonenum, result_buff);
	
	poweroff(NULL,0);
	return 0;
}


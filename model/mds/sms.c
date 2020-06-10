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

#include "logd/logd_rpc.h"
#include "util/validation.h"


#include "at/at_util.h"
#include "at/at_log.h"

#include "sms.h"
#include "packet.h"
#include "mds.h"
#include "mds_ctx.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static SMS_CMD_FUNC_T sms_cmd_func[] =
{
    {eSMS_CMD_SET__IP_INFO, SMS_CMD_SET__IP_INFO, _sms_cmd_proc_set_ip_info},
	{eSMS_CMD_SET__MDT_RESET, SMS_CMD_SET__MDT_RESET, _sms_cmd_proc_set_reset},
	{eSMS_CMD_SET__SET_INTERVAL, SMS_CMD_SET__SET_INTERVAL, _sms_cmd_proc_set_packet_interval},
	{eSMS_CMD_GET__GET_SETTING, SMS_CMD_GET__GET_SETTING, _sms_cmd_proc_get_setting},
	{eSMS_CMD_GET__DTG_STATUS, SMS_CMD_GET__DTG_STATUS, _sms_cmd_proc_get_dtg_status},
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
		if  (!( strncmp ( model_argv[0], sms_cmd_func[i].cmd, strlen(sms_cmd_func[i].cmd) ) ))
		{
			found_cmd = 1;
			break;
		}
	}

	if (found_cmd)
	{
		sms_cmd_func[i].proc_func(model_argc, model_argv, phonenum);
	}
	
	return ret;
}



int _sms_cmd_proc_set_ip_info(int argc, char* argv[], const char* phonenum)
{
	int i;
	//int port_number;
	
	
	// sms cmd example
	// &IP,[IP��ȣ],[PORT��ȣ],0
	
	LOGI(LOG_TARGET, "SMS - set ip routine. \n");
	
	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	
	// argument count check..
	if ((argc+1) != 4)
	{
		LOGE(LOG_TARGET, " -- invalid argument count \n");
		return -1;
	}
	
	// validation ip
	if (validation_check_ip(argv[1],strlen(argv[1])) != DEFINES_MDS_OK)
	{
		LOGE(LOG_TARGET, " -- invalid ip \n");
		return -1;
	}
	
	if (validation_check_is_num(argv[2],strlen(argv[2])) != DEFINES_MDS_OK)
	{
		LOGE(LOG_TARGET, " -- invalid port num \n");
		return -1;
	}
	
	//port_number = atoi(argv[2]);
	
	LOGI(LOG_TARGET, " -- set : ip [%s] / port num [%s]\n",argv[1], argv[2]);
	
	if(save_config_user("user:report_ip", argv[1]) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : model:report_ip\n");
		return -1;
	}
	
	if(save_config_user("user:report_port", argv[2]) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : model:report_port\n");
		return -1;
	}
	
	sleep(1);
	
	mds_load_and_set_config();

	return 0;
}


int _sms_cmd_proc_set_reset(int argc, char* argv[], const char* phonenum)
{
	int i;
	
	// sms cmd example
	// &RESET,[device ID],0
	
	LOGI(LOG_TARGET, "SMS - reset routine. \n");
	
	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// argument count check..
	if ((argc+1) != 3)
	{
		LOGE(LOG_TARGET, " -- invalid argument count \n");
		return -1;
	}
		
	// device id check.
	if (1)
	{
		LOGE(LOG_TARGET, "SMS --> Power off!!! \n");
		poweroff("sms poweroff", strlen("sms poweroff"));
	}
	return 0;
}


int _sms_cmd_proc_set_packet_interval(int argc, char* argv[], const char* phonenum)
{
	int i;
	char ini_set_string[32] = {0,};
	int keyon_report_interval = 0 ;
	int keyoff_report_interval = 0 ;
	
	LOGI(LOG_TARGET, "SMS - set interval routine. \n");
	
	// sms cmd example
	// &TIME,[Key On �ֱ�(��)],[Key Off �ֱ�(��)],0
	
	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// argument count check..
	if ((argc+1) != 4)
	{
		LOGE(LOG_TARGET, " -- invalid argument count \n");
		return -1;
	}
	
	//  ----- get keyon data ---------------------------------------------
	
	// argument check : keyon valid number
	if (validation_check_is_num(argv[1],strlen(argv[1])) != DEFINES_MDS_OK)
	{
		LOGE(LOG_TARGET, " -- invalid number \n");
		return -1;
	}
	
	// keyon report insteval ==> min to sec
	keyon_report_interval = atoi(argv[1]) * 60;
	LOGE(LOG_TARGET, " -- set keyon report interval is [%d] \n", keyon_report_interval);
	
	//  ----- get keyoff data ---------------------------------------------
	
	// argument check : keyoff valid number
	if (validation_check_is_num(argv[2],strlen(argv[2])) != DEFINES_MDS_OK)
	{
		LOGE(LOG_TARGET, " -- invalid interval \n");
		return -1;
	}
	
	// keyoff report insteval ==> min to sec
	keyoff_report_interval = atoi(argv[2]) * 60;
	LOGE(LOG_TARGET, " -- set keyoff report interval is [%d] \n", keyoff_report_interval);
	
	// --------------- setting value  ---------------------------------------------
	
	memset(ini_set_string, 0x00, sizeof(ini_set_string));
	sprintf(ini_set_string, "%d", keyon_report_interval);
	
	if(save_config_user("user:report_interval_keyon", ini_set_string) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : model:report_interval_keyon\n");
		return -1;
	}
	
	memset(ini_set_string, 0x00, sizeof(ini_set_string));
	sprintf(ini_set_string, "%d", keyoff_report_interval);

	thread_network_set_warn_timeout(MAX(keyon_report_interval, keyoff_report_interval) * 2);
	
	if(save_config_user("user:report_interval_keyoff", ini_set_string) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : model:report_interval_keyoff\n");
		return -1;
	}
	
	if(save_config_user("user:collect_interval_keyoff", ini_set_string) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : model:collect_interval_keyoff\n");
		return -1;
	}
	
	sleep(1);
	
	mds_load_and_set_config();
	return 0;
}

int _sms_cmd_proc_get_setting(int argc, char* argv[], const char* phonenum)
{
	int i;
	char response_buff[80] = {0,};
	configurationModel_t *conf = get_config_model();
	
	// sms cmd example
	// &SHOW,[return dial],0
	
	LOGI(LOG_TARGET, "SMS - get setting routine. \n");
	
	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// argument count check..
	if ((argc+1) != 3)
	{
		LOGE(LOG_TARGET, " -- invalid argument count \n");
		return -1;
	}
	
	if (validation_check_phonenum(argv[1],strlen(argv[1])) != DEFINES_MDS_OK)
	{
		printf("not valid phonenum");
		return -1;
	}
		
	/*
	1,MDT,[������ IP��ȣ],[������PORT��ȣ],[MDT��������],[������ KEY ON �ֱ�],[������ KEY OFF �ֱ�],0
	*/
	
	
	sprintf(response_buff,"1,MDT,%s,%d,ver.%s(%s),%dmin,%dmin,0\r\n",
							conf->model.report_ip,
							conf->model.report_port,
							PRG_VER,DATE,
							get_ctx_keyon_send_to_data_interval()/60,
							get_ctx_keyoff_send_to_data_interval()/60);
							
	//LOGI(LOG_TARGET, "-- SMS req : [%s]\r\n",response_buff);
	
	at_send_sms(argv[1], response_buff);
	
	return 0;
}

int _sms_cmd_proc_get_dtg_status(int argc, char* argv[], const char* phonenum)
{
	int i;
	char response_buff[80] = {0,};
	//configurationModel_t *conf = get_config_model();
	
	// sms cmd example
	// &SHOW,[return dial],0
	
	LOGI(LOG_TARGET, "SMS - get dtg status routine. \n");
	
	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	
	// argument count check..
	if ((argc+1) != 3)
	{
		LOGE(LOG_TARGET, " -- invalid argument count \n");
		return -1;
	}
	
	if (validation_check_phonenum(argv[1],strlen(argv[1])) != DEFINES_MDS_OK)
	{
		LOGE(LOG_TARGET, " -- invalid phone num count \n");
		return -1;
	}
	
	sprintf(response_buff,"1,DTG,%s,0\r\n",
							"0");
	
	LOGI(LOG_TARGET, "-- SMS req : [%s]\r\n",response_buff);
	//printf("response_buff is [%s]\r\n",response_buff);
	
	at_send_sms(argv[1], response_buff);
	return 0;
}


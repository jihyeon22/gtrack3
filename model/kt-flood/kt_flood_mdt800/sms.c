#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/thread.h>
#include <at/at_util.h>
#include <board/power.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/poweroff.h>
#include "logd/logd_rpc.h"

#include "config.h"
#include "debug.h"

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#include "kt_fota_ver.h"
#endif

#include "sms.h"

#include "packet.h"
#include "gpsmng.h"



static int _server_ip_set(int argc, char **argv);
static int _rpt_cycle_keyon(int argc, char **argv);
static int _mdt_reset (int argc, char **argv);
static int _get_version  (int argc, char **argv);


int parse_model_sms(char *time, char *phonenum, char *sms)
{
	int sms_cmd = 0;
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};
	int len = 0;
	int i = 0, j = 0;
	char *base = 0;
	char sms_buf[80];

	//remove space ++
	len = strlen(sms);
	memset(sms_buf, 0x00, sizeof(sms_buf));
	for(i = 0, j = 0; i < len; i++) {
		if(sms[i] != 0x20)
			sms_buf[j++] = sms[i];
	}
	//remove space ++
	
	base = sms_buf;
	len = j;
	

	model_argv[model_argc++] = base;

	while(model_argc <= 32 && len--) {
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
	

	model_argv[0][0] = '0';
	sms_cmd = atoi(model_argv[0]);
	
	switch(sms_cmd) {
		case eSMS_IP_SETUP:
			ret = _server_ip_set(model_argc, model_argv);
			break;
		case eSMS_REPORT_CYCLE_SETUP:
			ret = _rpt_cycle_keyon(model_argc, model_argv);
			break;
		case eSMS_GET_VERSION:
			ret = _get_version(model_argc, model_argv);
			break;
		case eMSG_MDT_RESET:
			_mdt_reset(model_argc, model_argv);
			break;
		default:
			LOGE(LOG_TARGET, "rmc_sms_parsing ERROR = %d\n", sms_cmd);
			ret = -1;
	}

	LOGD(LOG_TARGET, "parse_model_sms = %d\n", ret);
	return ret;
}



static int _server_ip_set(int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_passrd;
	char *p_str_ip;
	char *p_str_port;
	char *p_str_response;

	if(argc != 4)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS format\n", __FUNCTION__);
		return -1;
	}
	
	p_str_passrd   = argv[1];
	p_str_ip       = argv[2];
	p_str_port     = argv[3];
	p_str_response = argv[4];
			
	LOGD(LOG_TARGET, "%s> setting PW = %s \n"  , __func__, p_str_passrd);
	LOGD(LOG_TARGET, "%s> setting IP = %s \n"  , __func__, p_str_ip);
	LOGD(LOG_TARGET, "%s> setting PORT = %s \n", __func__, p_str_port);
	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);

	if(strncmp(SMS_PW_SETIP, p_str_passrd, 8)!=0)
	{
		LOGE(LOG_TARGET, "<%s> SMS PASSWRD WRONG\n", __FUNCTION__);
		return -1;
	}
	strncpy(conf->model.report_ip, p_str_ip, sizeof(conf->model.report_ip));
	conf->model.report_port = atoi(p_str_port);

	if(save_config_user("user:report_ip", p_str_ip) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:report_port", p_str_port) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
		return -1;
	}

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
   		sender_add_data_to_buffer(eSMS_IP_SETUP, NULL, ePIPE_2);
		
		sprintf(smsmsg, "ip> %s:%s\n", argv[2],  argv[3]);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
   	}

   	return 0;
}

static int _rpt_cycle_keyon(int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_report_interval;
	char *p_str_collect_interval;
	char *p_str_response;

	unsigned int report_interval;
	unsigned int collect_interval;

	if(argc != 3)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	p_str_report_interval  = argv[1];
	p_str_collect_interval = argv[2];
	p_str_response         = argv[3];

	LOGD(LOG_TARGET, "%s> report cycle = %s \n", __func__, p_str_report_interval);
	LOGD(LOG_TARGET, "%s> create cycle = %s \n", __func__, p_str_collect_interval);
	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);

	report_interval = atoi(p_str_report_interval);
	collect_interval = atoi(p_str_collect_interval);
	
	if(collect_interval > report_interval) {
		LOGE(LOG_TARGET, "collection is greater report time", __FUNCTION__);
		return -1;
	}

	// if(validation_model_report_interval(collect_interval, report_interval) < 0)
	// {
	// 	LOGE(LOG_TARGET, "<%s> Validation interval SMS", __FUNCTION__);
	// 	return -1;
	// }

	conf->model.report_interval_keyon = report_interval;
	conf->model.collect_interval_keyon = collect_interval;

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);

	if(save_config_user("user:report_interval_keyon", p_str_report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:collect_interval_keyon", p_str_collect_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
		return -1;
	}

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eSMS_REPORT_CYCLE_SETUP, NULL, ePIPE_2);
		
		sprintf(smsmsg, "report> %u:%u\n", report_interval,  collect_interval);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}

	return 0;
}

static int _get_version  (int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();

	char *p_str_resp_num;
	char *p_str_response;


	char send_buff[80] = {0,};

	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS\n", __FUNCTION__);
		return -1;
	}
	p_str_resp_num = argv[1];
	p_str_response = argv[2];


	snprintf(send_buff,80, "%s,%s,%d,%d,%d",
			"01.01",
			conf->model.report_ip,
			conf->model.report_port,
			conf->model.report_interval_keyon,
			conf->model.collect_interval_keyon);

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		sender_add_data_to_buffer(eSMS_GET_VERSION, NULL, ePIPE_2);
	}
	
	at_send_sms(p_str_resp_num, send_buff);
	return 0;
}

static int _mdt_reset (int argc, char **argv)
{
	char *p_str_reset_password;
	char *p_str_response;


	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS\n", __FUNCTION__);
		return -1;
	}

	p_str_reset_password = argv[1];
	p_str_response       = argv[2];

	LOGD(LOG_TARGET, "%s> mdt_reset pw = %s \n", __func__, p_str_reset_password);
	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);

	if(strcmp(SMS_PW_RESET, p_str_reset_password)!=0)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect RESET PW SMS", __FUNCTION__);
		return -1;
	}

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eMSG_MDT_RESET, NULL, ePIPE_2);
	}

	////////////////////////////////////////////////////
	//dev reset
	////////////////////////////////////////////////////
	LOGI(LOG_TARGET, "MDT SMS Reset Command!!!!\n");
	LOGD(LOG_TARGET, "MDT must reset now!!!\n");
	
	poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	
	return 0;
}


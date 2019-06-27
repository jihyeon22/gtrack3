#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sms.h"

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/thread.h>
#include <at/at_util.h>
#include <board/power.h>
#include <config.h>
#include <validation.h>
#include <netcom.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/poweroff.h>
#include <util/validation.h>
#include "logd/logd_rpc.h"
#include "alloc_packet.h"
#include "debug.h"
#include "netcom.h"

#include <dm/update.h>
#include <dm/update_api.h>

#include <at/at_util.h>

void _deinit_essential_functions(void);
static int _server_ip_set(int argc, char **argv, char *phonenum);
static int _rpt_cycle_keyon(int argc, char **argv, char *phonenum);
static int _mdt_status (int argc, char **argv, char *phonenum);
static int _rpt_dm_setup (int argc, char **argv, char *phonenum);
static int _ftp_update (int argc, char **argv, char *phonenum);
static int _rpt_cycle2(int argc, char **argv, char *phonenum);

#ifdef BOARD_NEO_W200K
static int _get_version (char *pn);
static int _get_modem_status (char *pn);
#endif


int parse_model_sms(char *time, char *phonenum, char *sms)
{
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};
	int len = 0;
	int i = 0, j = 0;
	char *base = 0;
	char sms_buf[120] = {0};

	//remove space ++
	len = strlen(sms);
	memset(sms_buf, 0, sizeof(sms_buf));
	for(i = 0, j = 0; i < len; i++) {
		if(sms[i] != 0x20)
			sms_buf[j++] = sms[i];
	}
	//remove space ++

	printf("parse_model_sms\r\n"); 

	//LOGE(LOG_TARGET, "TEST........CODE++++++++++++++++");
	//extern void alloc_geo_fence_info_init();
	//alloc_geo_fence_info_init();
	//LOGE(LOG_TARGET, "TEST........CODE++++++++++++++++");
	
	base = sms_buf;
	len = j;

	if(strncmp(base, "(RRR)", 5) == 0)
	{
		_deinit_essential_functions();
		poweroff("Reset SMS CMD\n", sizeof("Reset SMS CMD\n"));
	}

	if(base[0] != '{' && base[0] != '(')
	{
		printf("abnormal sms : start character is abnormal %c\n", base[0]);
		return -1;
	}

	if(base[len-1] != '}' && base[len-1] != ')')
	{
		printf("abnormal sms : end character is abnormal %c\n", base[len-1]);
		return -1;
	}

	base++;
	
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
			case '}':
			case ')':
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

	if(!strcmp(model_argv[0], szSMS_IP_SETUP))
	{
		ret = _server_ip_set(model_argc, model_argv, phonenum);
	}
	else if(!strcmp(model_argv[0], szSMS_REPORT_CYCLE_SETUP))
	{
		ret = _rpt_cycle_keyon(model_argc, model_argv, phonenum);
	}
	else if(!strcmp(model_argv[0], szSMS_REPORT2_CYCLE_SETUP))
	{
		ret = _rpt_cycle2(model_argc, model_argv, phonenum);
	}
	else if(!strcmp(model_argv[0], szSMS_REQUEST_STATUS))
	{
		ret = _mdt_status(model_argc, model_argv, phonenum);
	}
	else if(!strcmp(model_argv[0], szSMS_REPORT_DM_SETUP))
	{
		ret = _rpt_dm_setup(model_argc, model_argv, phonenum);
	}
	else if(!strcmp(model_argv[0], szSMS_FTP_UPDATE))
	{
		ret = _ftp_update(model_argc, model_argv, phonenum);
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
	else if(!strcmp(model_argv[0], szSMS_reset_sms))
	{
		ret = at_send_initsms(model_argv[1]);
	}
	else
	{
		LOGE(LOG_TARGET, "rmc_sms_parsing ERROR = %.3s\n", model_argv[0]);
		ret = -1;
	}

	LOGD(LOG_TARGET, "parse_model_sms = %d\n", ret);
	return ret;
}

static int _server_ip_set(int argc, char **argv, char *phonenum)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_passrd;
	char *p_str_ip;
	char *p_str_port;
	char *p_str_response;
	int ret = 0;

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

	setting_network_param();

	ret = atoi(p_str_response);

	if(ret == SMS_CMD_RESPONSE_TCP)
	{
		LOGI(LOG_TARGET, "MAKE PKT : SET IP");
   		sender_add_data_to_buffer(PACKET_TYPE_RESP_SMS, NULL, ePIPE_2);
   	}
   	else if(ret == SMS_CMD_RESPONSE_SMS)
   	{
   		unsigned char *buff = NULL;

   		printf("RESP SMS\n");
   		mkpkt_sms_resp_sms_data(&buff);
   		// jhcho_compile
		//at_send_sms(phonenum, buff, 5);
		at_send_sms(phonenum, (const char *)buff);

   		free(buff);
   	}

   	return 0;
}

static int _rpt_cycle_keyon(int argc, char **argv, char *phonenum)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_report_interval;
	char *p_str_collect_interval;
	char *p_str_response;

	unsigned int report_interval;
	unsigned int collect_interval;
	int ret = 0;

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

	if(validation_model_report_interval(collect_interval, report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS", __FUNCTION__);
		return -1;
	}

	conf->model.report_interval_keyon = report_interval;
	conf->model.collect_interval_keyon = collect_interval;
	
	if(save_config_user("user:collect_interval_keyon", p_str_collect_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:report_interval_keyon", p_str_report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
		return -1;
	}

	ret = atoi(p_str_response);

	if(ret == SMS_CMD_RESPONSE_TCP)
	{
		LOGI(LOG_TARGET, "MAKE PKT : SET PERIOD REPORT");
   		sender_add_data_to_buffer(PACKET_TYPE_RESP_SMS, NULL, ePIPE_2);
   	}
   	else if(ret == SMS_CMD_RESPONSE_SMS)
   	{
   		unsigned char *buff = NULL;
   		mkpkt_sms_resp_sms_data(&buff);
   		// jhcho_compile
		//at_send_sms(phonenum, buff, 5);
		at_send_sms(phonenum, (const char *)buff);
   		free(buff);
   	}

	return 0;
}

static int _mdt_status (int argc, char **argv, char *phonenum)
{
	int ret = 0;

	if(argc != 3)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "mdt_status response = %s \n",argv[3]);

	if(atoi(argv[3]) == 0)
	{
		return 0;
	}
	
	ret = atoi(argv[1]);

	if(ret == 0)
	{
		LOGI(LOG_TARGET, "MAKE PKT : STATUS");
   		sender_add_data_to_buffer(PACKET_TYPE_RESP_SMS, NULL, ePIPE_2);
   	}
   	else if(ret == 1)
   	{   	
   		unsigned char *buff = NULL;

		if(validation_check_phonenum(argv[2], strlen(argv[2])) < 0)
		{
			return -1;
		}
   		
   		mkpkt_sms_resp_sms_data(&buff);
		// jhcho_compile
   		//at_send_sms(argv[2], buff, 5);
		at_send_sms(argv[2], (const char *)buff);
   		free(buff);
   	}

	return 0;
}
static int _rpt_dm_setup (int argc, char **argv, char *phonenum)
{
	int ret = 0;
	int setup = 0;
	unsigned char *buff = NULL;

	if(argc != 3)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "_rpt_dm_setup response = %s \n",argv[3]);

	ret = atoi(argv[1]);
	if(ret == 1)
	{
		setup = 1;
		save_config("webdm:enable", "1");
		save_config("webdm:tx_power", "1");
		save_config("webdm:tx_ignition","1");
		save_config("webdm:tx_report", "0"); // report 0 default
		save_config("webdm:tx_log", "1");
	}
	else if (ret == 0)
	{
		setup = 0;
		save_config("webdm:enable", "0");
		save_config("webdm:tx_power", "0");
		save_config("webdm:tx_ignition","0");
		save_config("webdm:tx_report", "0");
		save_config("webdm:tx_log", "0");
	}
	else
	{
		setup = 2;
		setup = load_config_base_webdm();
	}
	
	if(atoi(argv[3]) == 0)
	{
		return 0;
	}

	if(validation_check_phonenum(argv[2], strlen(argv[2])) < 0)
	{
		return -1;
	}

	mkpkt_sms_resp_dm_data(&buff, setup);
	at_send_sms(argv[2], (const char *)buff);
	free(buff);

	return 0;
}
static int _ftp_update (int argc, char **argv, char *phonenum)
{
	dm_res res = DM_FAIL;
	UPDATE_VERS update_data;
	update_data.version = UPDATE_VCMD;
	int port = 0;

	if(argc != 5)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}
	port = atoi(argv[2]);

	LOGD(LOG_TARGET, "_ftp_update ip = %s, port: %d, id : %s, file:%s \n",argv[1], port, argv[3], argv[5]);

	res =  dm_update_ftp_download(argv[1], port, argv[3], argv[4], argv[5]);

	if (res == DM_OK) {
		//_deinit_essential_functions();
		//terminate_app_callback();
		
		res = version_update(&update_data);
		if(res == success)
		{
			res = DM_OK;
		}
		else
		{
			printf("[dmlib] %s : update process failture\n", __func__);
			res = DM_FAIL;
		}
	}

	//poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	return 0;

}


static int _rpt_cycle2 (int argc, char **argv, char *phonenum)
{
	configurationModel_t * conf = get_config_model();
	unsigned int report_interval_keyon;
	unsigned int collect_interval_keyon;
	unsigned int report_interval_keyoff;
	unsigned int collect_interval_keyoff;

	char *p_str_report_interval_keyon;
	char *p_str_report_interval_keyoff;
	char *p_str_collect_interval_keyon;
	char *p_str_collect_interval_keyoff;
	char *p_str_response;
	int ret = 0;

	if(argc != 5)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	p_str_report_interval_keyon   = argv[1];
	p_str_collect_interval_keyon  = argv[2];
	p_str_report_interval_keyoff  = argv[3];
	p_str_collect_interval_keyoff = argv[4];
	p_str_response                = argv[5];

	report_interval_keyon   = atoi(p_str_report_interval_keyon);
	collect_interval_keyon  = atoi(p_str_collect_interval_keyon);
	report_interval_keyoff  = atoi(p_str_report_interval_keyoff);
	collect_interval_keyoff = atoi(p_str_collect_interval_keyoff);

	LOGD(LOG_TARGET, "%s> keyon report cycle = %d \n" , __func__, report_interval_keyon);
	LOGD(LOG_TARGET, "%s> keyon create cycle = %d \n" , __func__, collect_interval_keyon);
	LOGD(LOG_TARGET, "%s> keyoff report cycle = %d \n", __func__, report_interval_keyoff);
	LOGD(LOG_TARGET, "%s> keyoff create cycle = %d \n", __func__, collect_interval_keyoff);
	LOGD(LOG_TARGET, "%s> response = %s \n"           , __func__, p_str_response);

	if(validation_model_report_interval(collect_interval_keyon, report_interval_keyon) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS #1", __FUNCTION__);
		return -1;
	}

	if(validation_model_report_off_interval(collect_interval_keyoff, report_interval_keyoff) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS #2", __FUNCTION__);
		return -1;
	}

	conf->model.report_interval_keyon   = report_interval_keyon;
	conf->model.collect_interval_keyon  = collect_interval_keyon;
	conf->model.report_interval_keyoff  = report_interval_keyoff;
	conf->model.collect_interval_keyoff = collect_interval_keyoff;

	if(save_config_user("user:collect_interval_keyon", p_str_collect_interval_keyon) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:report_interval_keyon", p_str_report_interval_keyon) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
		return -1;
	}

	if(save_config_user("user:collect_interval_keyoff", p_str_collect_interval_keyoff) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #3\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:report_interval_keyoff", p_str_report_interval_keyoff) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #4\n", __FUNCTION__);
		return -1;
	}

	ret = atoi(p_str_response);

	if(ret == SMS_CMD_RESPONSE_TCP)
	{
		LOGI(LOG_TARGET, "MAKE PKT : SET PERIOD REPORT2");
   		sender_add_data_to_buffer(PACKET_TYPE_RESP_SMS, NULL, ePIPE_2);
   	}
   	else if(ret == SMS_CMD_RESPONSE_SMS)
   	{
   		unsigned char *buff = NULL;
   		
   		mkpkt_sms_resp_sms_data(&buff);
		// jhcho_compile
   		//at_send_sms(phonenum, buff, 5);
		at_send_sms(phonenum, (const char *)buff);
   		free(buff);
   	}

	return 0;
}


int _lat_nmea2float(char * buf, float * pf)
{
	*pf = 0;
	if(buf == NULL) {
		return -1;
	}
	if(strlen(buf) < 9) {
		return -1;
	}

	*pf = ((buf[0] - '0') * 10) + (buf[1] - '0') + (atof(buf + 2) / 60.0f);
	//*pf = ((buf[0] - '0') * 10) + (buf[1] - '0') + (atof(buf + 2) / 100.0f);
	//*pf = atof(buf); 
	return 0;
}

int _lon_nmea2float(char * buf, float * pf)
{
	*pf = 0;
	if(buf == NULL)
		return -1;

	if(strlen(buf) < 10)
		return -1;

	*pf = ((buf[0] - '0') * 100) + ((buf[1] - '0') * 10) + (buf[2] - '0') + (atof(buf + 3) / 60.0f);
	//*pf = ((buf[0] - '0') * 100) + ((buf[1] - '0') * 10) + (buf[2] - '0') + (atof(buf + 3) / 100.0f);
	//*pf = atof(buf);
	return 0;
}

#ifdef BOARD_NEO_W200K
static int _get_version (char *pn)
{
	at_send_sms(pn, KT_FOTA_VER, 3);
	return 0;
}

static int _get_modem_status (char *pn)
{
	char buff[512] = {0};
	int ret = 0;
	
	ret = get_modem_status(buff);
	if(ret >= 0)
	{
		at_send_sms(pn, buff, 3);
	}

	return 0;
}
#endif


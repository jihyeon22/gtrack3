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

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
#include "kt_fota_ver.h"
#endif

#include "sms.h"
#include "validation.h"

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/geofence.h>
#include <mdt800/gps_utill.h>
#include <mdt800/file_mileage.h>


static int _service_set(int argc, char **argv);
static int _server_ip_set(int argc, char **argv);
static int _rpt_cycle_keyon(int argc, char **argv);
static int _mileage_set (int argc, char **argv);
static int _mdt_status (int argc, char **argv);
static int _gpio_mode (int argc, char **argv);
static int _gpio_output (int argc, char **argv);
static int _fence_set (int argc, char **argv);
static int _rpt_cycle2(int argc, char **argv);
static int _mdt_reset (int argc, char **argv);
static int _report_type_cycle_set(int argc, char **argv);

#ifdef BOARD_NEO_W200K
#ifdef KT_FOTA_ENABLE
static int _get_version (char *pn);
#endif
static int _get_modem_status (char *pn);
#endif


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

	if(strcmp(model_argv[0], "&E") == 0)
	{
		sms_cmd = atoi(model_argv[1]);

		switch(sms_cmd) {
			case 1:
				ret = _service_set(model_argc, model_argv);
				break;
			case 2:
				ret = _report_type_cycle_set(model_argc, model_argv);
				break;
			default:
				LOGE(LOG_TARGET, "rmc_sms_parsing etrace ERROR = %d\n", sms_cmd);
				ret = -1;
		}
	}
	else
	{
		
		model_argv[0][0] = '0';
		sms_cmd = atoi(model_argv[0]);
		
		switch(sms_cmd) {
			case eSMS_IP_SETUP:
				ret = _server_ip_set(model_argc, model_argv);
				break;
			case eSMS_REPORT_CYCLE_SETUP:
				ret = _rpt_cycle_keyon(model_argc, model_argv);
				break;
			case eSMS_ODO_SETUP:
				ret = _mileage_set(model_argc, model_argv);
				break;
			case eSMS_MDT_STATUS:
				ret = _mdt_status(model_argc, model_argv);
				break;
			case eSMS_GPIO_MODE:
				ret = _gpio_mode(model_argc, model_argv);
				break;
			case eSMS_GPIO_OUTPUT:
				ret = _gpio_output(model_argc, model_argv);
				break;
			case eMSG_MDT_RESET:
				_mdt_reset(model_argc, model_argv);
				break;
			case eSMS_REPORT2_CYCLE_SETUP:
				_rpt_cycle2(model_argc, model_argv);
				break;
			case eSMS_GEO_FENCE_SET:
				ret = _fence_set(model_argc, model_argv);
				break;
#ifdef BOARD_NEO_W200K
#ifdef KT_FOTA_ENABLE
			case eSMS_GET_VERSION:
				ret = _get_version(phonenum);
				break;
#endif
			case eSMS_GET_MODEM_STATUS:
				ret = _get_modem_status(phonenum);
				break;
#endif
			default:
				LOGE(LOG_TARGET, "rmc_sms_parsing ERROR = %d\n", sms_cmd);
				ret = -1;
		}
	}

	LOGD(LOG_TARGET, "parse_model_sms = %d\n", ret);
	return ret;
}

static int _service_set(int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_sensor;
	char *p_str_service;
	char *p_str_response;

	printf("argc %d\n", argc);

//	if(argc != 4)
	if(argc < 3)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS format\n", __FUNCTION__);
		return -1;
	}
	
	p_str_response   = argv[2];
	p_str_service       = argv[3];
	p_str_sensor     = argv[4];
			
	LOGD(LOG_TARGET, "%s> response = %s \n"  , __func__, p_str_response);
	LOGD(LOG_TARGET, "%s> service  = %s \n"  , __func__, p_str_service);
	//LOGD(LOG_TARGET, "%s> sensor = %s \n", __func__, p_str_sensor);

	if(strcmp(p_str_service, "GENRL") == 0)
	{
		conf->model.button1 = 11;
		conf->model.button2 = 13;
	}
	else if(strcmp(p_str_service, "POWDR") == 0)
	{
		conf->model.button1 = 20;
		conf->model.button2 = 20;
	}
	else
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SERVICE type.\n", __FUNCTION__);
		return -1;
	}

	char str_button1[8] = {0};
	char str_button2[8] = {0};

	snprintf(str_button1, sizeof(str_button1)-1, "%d", conf->model.button1);
	snprintf(str_button2, sizeof(str_button2)-1, "%d", conf->model.button2);
	

	if(save_config_user("user:button1", str_button1) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
		return -1;
	}
	if(save_config_user("user:button2", str_button2) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
		return -1;
	}

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
//		char smsmsg[100] = {0,};
   		sender_add_data_to_buffer(eMDT_STATUS_EVC, NULL, ePIPE_2);
		
//		sprintf(smsmsg, "ip> %s:%s\n", argv[2],  argv[3]);
//		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
   	}

   	return 0;
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
   		sender_add_data_to_buffer(eIP_SETUP_EVC, NULL, ePIPE_2);
		
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

	if(validation_model_report_interval(collect_interval, report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS", __FUNCTION__);
		return -1;
	}

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
		sender_add_data_to_buffer(eREPORT_CYCLE_SETUP_EVT, NULL, ePIPE_2);
		
		sprintf(smsmsg, "report> %u:%u\n", report_interval,  collect_interval);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}

	return 0;
}

static int _report_type_cycle_set(int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();
	char *p_str_report_interval;
	char *p_str_collect_interval;
	char *p_str_packet_type;
	char *p_str_response;

	unsigned int packet_type;
	unsigned int report_interval;
	unsigned int collect_interval;

	if(argc != 5)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS argc[%d]", __FUNCTION__, argc);
		return -1;
	}

	
	p_str_response         = argv[2];
	p_str_packet_type	   = argv[3];
	p_str_collect_interval = argv[4];
	p_str_report_interval  = argv[5];

	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);
	LOGD(LOG_TARGET, "%s> packet type = %s \n", __func__, p_str_packet_type);
	LOGD(LOG_TARGET, "%s> report cycle = %s \n", __func__, p_str_report_interval);
	LOGD(LOG_TARGET, "%s> create cycle = %s \n", __func__, p_str_collect_interval);
	

	report_interval = atoi(p_str_report_interval);
	collect_interval = atoi(p_str_collect_interval);
	packet_type = atoi(p_str_packet_type);

	if( !(packet_type == 64 || packet_type == 65 || packet_type == 81 || packet_type == 82) ) 
	{
		LOGE(LOG_TARGET, "unknown report type", __FUNCTION__);
		return -1;
	}

	
	if(collect_interval > report_interval) {
		LOGE(LOG_TARGET, "collection is greater report time", __FUNCTION__);
		return -1;
	}

	if(validation_model_report_interval(collect_interval, report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS", __FUNCTION__);
		return -1;
	}

	conf->model.report_interval_keyon = report_interval;
	conf->model.collect_interval_keyon = collect_interval;
	conf->model.packet_type = packet_type;

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
	if(save_config_user("user:packet_type", p_str_packet_type) < 0)
	{
		LOGE(LOG_TARGET, "<%s> save config error #3\n", __FUNCTION__);
		return -1;
	}

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eMDT_STATUS_EVC, NULL, ePIPE_2);
		
		sprintf(smsmsg, "report> %u:%u\n", report_interval,  collect_interval);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}

	return 0;
}

static int _mileage_set (int argc, char **argv)
{
	char *p_str_dist;
	char *p_str_response;
	int dist;

	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}
	p_str_dist     = argv[1];
	p_str_response = argv[2];
	dist = atoi(p_str_dist);

	set_server_mileage(dist);
	set_gps_mileage(0);
	save_mileage_file(dist);

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eODO_SETUP_EVC, NULL, ePIPE_2);
		
		sprintf(smsmsg, "mileage> %d\n", dist);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}


	return 0;
}

static int _mdt_status (int argc, char **argv)
{
	if(argc != 1)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "mdt_status response = %s \n",argv[1]);
	
	if(atoi(argv[1]) == SMS_CMD_RESPONSE_NEED)
	{
		sender_add_data_to_buffer(eMDT_STATUS_EVC, NULL, ePIPE_2);
	}

	return 0;
}

static int _gpio_mode (int argc, char **argv)
{
	return 0;
}

static int _gpio_output (int argc, char **argv)
{
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

	////////////////////////////////////////////////////
	//dev reset
	////////////////////////////////////////////////////
	LOGI(LOG_TARGET, "MDT SMS Reset Command!!!!\n");
	LOGD(LOG_TARGET, "MDT must reset now!!!\n");
	
    save_user_data();
    
	poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	
	return 0;
}

static int _rpt_cycle2 (int argc, char **argv)
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
	
	if(collect_interval_keyon > report_interval_keyon) 
	{
		LOGE(LOG_TARGET, "key on collection is greater report time", __FUNCTION__);
		return -1;
	}

	if(collect_interval_keyoff > report_interval_keyoff)
	{
		LOGE(LOG_TARGET, "key off collection is greater report time", __FUNCTION__);
		return -1;
	}

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

	if(validation_model_report_interval(collect_interval_keyoff, report_interval_keyoff) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS #2", __FUNCTION__);
		return -1;
	}

	conf->model.report_interval_keyon   = report_interval_keyon;
	conf->model.collect_interval_keyon  = collect_interval_keyon;
	conf->model.report_interval_keyoff  = report_interval_keyoff;
	conf->model.collect_interval_keyoff = collect_interval_keyoff;

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
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

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eREPORT_CYCLE_NUM2_SETUP_EVT, NULL, ePIPE_2);
		
		sprintf(smsmsg, "report2> %u:%u:%u:%u\n", report_interval_keyon, collect_interval_keyon, report_interval_keyoff, collect_interval_keyoff);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
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


static int _fence_set (int argc, char **argv)
{
	lotte_gps_manager_t *pgps_mng;

	char *p_str_address;
	char *p_str_condition;
	char *p_str_lat;
	char *p_str_lon;
	char *p_str_range;
	char *p_str_response;

	int fence_cond;
	int fence_range;
	int fence_address;
	float sms_lat, sms_lon;
	gpsData_t cur_gpsdata;
	geo_fence_data_t fence;
	int dist;
	int ret;
	if(argc != 6)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS\n", __FUNCTION__);
		return -1;
	}

	p_str_address   = argv[1];
	p_str_condition = argv[2];
	p_str_lat       = argv[3];
	p_str_lon       = argv[4];
	p_str_range     = argv[5];
	p_str_response  = argv[6];

	fence_address = atoi(p_str_address);
	if( !(fence_address == 0 || fence_address == 1) )
	{
		LOGE(LOG_TARGET, "<%s> geo fence address error [%d]\n", __FUNCTION__, fence_address);
		return -1;
	}
	fence_cond = atoi(p_str_condition);
	if( !(fence_cond == 0 || fence_cond == 1 || fence_cond == 2) ) 
	{
		LOGE(LOG_TARGET, "<%s> geo fence condition error [%d]\n", __FUNCTION__, fence_cond);
		return -1;
	}

	if(_lat_nmea2float(&p_str_lat[1], &sms_lat) < 0)
	{
		LOGE(LOG_TARGET, "<%s> lat format error\n", __FUNCTION__);
		return -1;
	}
	if(_lon_nmea2float(&p_str_lon[1], &sms_lon) < 0)
	{
		LOGE(LOG_TARGET, "<%s> lon format error\n", __FUNCTION__);
		return -1;
	}
	
	fence_range = atoi(p_str_range);

	

	LOGD(LOG_TARGET, "save addres = %d \n",fence_address);
	LOGD(LOG_TARGET, "event cond = %d \n",fence_cond);
	LOGD(LOG_TARGET, "lat = %3.5f \n",sms_lat);
	LOGD(LOG_TARGET, "lon = %3.5f \n",sms_lon);
	LOGD(LOG_TARGET, "fence range = %d \n",fence_range);
	LOGD(LOG_TARGET, "response = %s \n",argv[6]);

	//TODO
	pgps_mng = load_gps_manager();

	if(pgps_mng->first_gps_active == eINACTIVE || pgps_mng->fixed_gpsdata.active  == eINACTIVE)
	{
		//current gps
		gps_get_curr_data(&cur_gpsdata);
		dist = get_distance_meter(cur_gpsdata.lat, sms_lat, cur_gpsdata.lon, sms_lon);
		
	}
	else
	{
		dist = get_distance_meter(pgps_mng->fixed_gpsdata.lat, sms_lat, pgps_mng->fixed_gpsdata.lon, sms_lon);
	}

	fence.enable   = eGEN_FENCE_ENABLE;
	fence.latitude = sms_lat;
	fence.longitude = sms_lon;
	fence.range = fence_range;

	switch(fence_cond)
	{
		case 0:
			fence.setup_fence_status = eFENCE_SETUP_ENTRY;
			break;
		case 1:
			fence.setup_fence_status = eFENCE_SETUP_EXIT;
			break;
		case 2:
			fence.setup_fence_status = eFENCE_SETUP_ENTRY_EXIT;
			break;
	}

	if(dist < 0)
		fence.cur_fence_status = eFENCE_STATUS_OUT;
	else if(dist < fence_range)
		fence.cur_fence_status = eFENCE_STATUS_IN;
	else
		fence.cur_fence_status = eFENCE_STATUS_OUT;

	if(fence_address == 0)
		ret = set_geo_fence0(fence); //in this function, auto geo fence contents update
	else
		ret = set_geo_fence1(fence); //in this function, auto geo fence contents update

	if(ret < 0) {
		LOGD(LOG_TARGET, "geo fence data = %s \n",argv[6]);
		return -1;
	}
		
	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		char smsmsg[100] = {0,};
		sender_add_data_to_buffer(eGEO_FENCE_SETUP, NULL, ePIPE_2);
		
		sprintf(smsmsg, "geo fence> %d:%d:%3.5f:%3.5f:%d\n", fence_address, fence_cond, sms_lat, sms_lon, fence_range);
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}

	return 0;
}

#ifdef BOARD_NEO_W200K
#ifdef KT_FOTA_ENABLE
static int _get_version (char *pn)
{
	at_send_sms(pn, KT_FOTA_VER, 3);
	return 0;
}
#endif

static int _get_modem_status (char *pn)
{
	char buff[512] = {0};
	int ret = 0;
	
	//ret = get_modem_status(buff);
	if(ret >= 0)
	{
		at_send_sms(pn, buff, 3);
	}

	return 0;
}
#endif

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

#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include "config.h"
#include "debug.h"

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#include "kt_fota_ver.h"
#endif

#include "sms.h"
#include "validation.h"

#include "packet.h"
#include "gpsmng.h"
#include "geofence.h"
#include "gps_utill.h"
#include "file_mileage.h"

#include "server_resp.h"


static int _server_ip_set(int argc, char **argv);
static int _rpt_cycle_keyon(int argc, char **argv);
static int _mileage_set (int argc, char **argv);
static int _mdt_status (int argc, char **argv);
static int _gpio_mode (int argc, char **argv);
static int _gpio_output (int argc, char **argv);
static int _fence_set (int argc, char **argv);
static int _rpt_cycle2(int argc, char **argv);
static int _mdt_reset (int argc, char **argv);
static int _invoice_set (int argc, char **argv);
static int _invoice_set_2 (int argc, char **argv);

#ifdef BOARD_TL500K
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
	char *model_argv[128] = {0};
	int len = 0;
	int i = 0, j = 0;
	char *base = 0;

	char sms_buf[80];
	char sms_buf2[SERVER_RESP_MAX_LEN];

	FILE *fp = NULL;
	FILE *log_fd = NULL;
	char log_file_name[64] ={0,};

	struct timeval tv;
	struct tm *ptm;

	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);

	sprintf(log_file_name, "/var/log/ctlsms.%02d%02d%02d.log", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	fp = fopen(log_file_name, "w");

	if(fp == NULL)
		log_fd = stderr;
	else
		log_fd = fp;

	fprintf(log_fd, "[%s]\r\n", sms);

	devel_webdm_send_log("CTR SMS [%s]", sms);

	//remove space ++
	len = strlen(sms);
	memset(sms_buf, 0x00, sizeof(sms_buf));
	
	if ( strcmp(SERVER_RESP_PROC_PHONENUM, phonenum) == 0 )
	{
		LOGD(LOG_TARGET, ">>> SMS CASE WEB PARSING\r\n");
		for(i = 0, j = 0; i < len; i++) {
			if(sms[i] != 0x20)
			sms_buf2[j++] = sms[i];
		}
		//remove space ++
		base = sms_buf2;
	}
	else
	{
		LOGD(LOG_TARGET, ">>> SMS CASE SMS PARSING\r\n");
		for(i = 0, j = 0; i < len; i++) {
			if(sms[i] != 0x20)
				sms_buf[j++] = sms[i];
		}
		//remove space ++
		base = sms_buf;
	}

	len = j;
	

	model_argv[model_argc++] = base;

	while(model_argc <= 128 && len--) {
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
		printf( " >> %d %s\n", i, model_argv[i]);
	}

	if(model_argv[0] == NULL)
	{
		return -1;
	}
	
	model_argv[0][0] = '0';
	sms_cmd = atoi(model_argv[0]);
	
	printf(">>> SMS CMD %d\n", sms_cmd);

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
		case eSMS_INVOICE_INFO:
#ifdef SERVER_ABBR_NIS0
			ret = _invoice_set(model_argc, model_argv);
#elif SERVER_ABBR_NIS1
			ret = _invoice_set_2(model_argc, model_argv);
#endif
			break;
#ifdef BOARD_TL500K
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

	LOGD(LOG_TARGET, "parse_model_sms = %d\n", ret);

	if(fp != NULL)
		fclose(fp);

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
   		sender_add_data_to_buffer(eIP_SETUP_EVC, NULL, ePIPE_1);
		
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
		sender_add_data_to_buffer(eREPORT_CYCLE_SETUP_EVT, NULL, ePIPE_1);
		
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
		sender_add_data_to_buffer(eODO_SETUP_EVC, NULL, ePIPE_1);
		
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
		sender_add_data_to_buffer(eMDT_STATUS_EVC, NULL, ePIPE_1);
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
	
	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		sender_add_data_to_buffer(eMDM_DEV_RESET, NULL, ePIPE_1);
	}
	sleep(10);
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
		sender_add_data_to_buffer(eSMS_TRANSFER_EVT, NULL, ePIPE_1);
		
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
//	if(strlen(buf) < 9) {
//		return -1;
//	}

	//*pf = ((buf[0] - '0') * 10) + (buf[1] - '0') + (atof(buf + 2) / 60.0f);
	//*pf = ((buf[0] - '0') * 10) + (buf[1] - '0') + (atof(buf + 2) / 100.0f);
	*pf = atof(buf); 
	return 0;
}

int _lon_nmea2float(char * buf, float * pf)
{
	*pf = 0;
	if(buf == NULL)
		return -1;

//	if(strlen(buf) < 10)
//		return -1;

	//*pf = ((buf[0] - '0') * 100) + ((buf[1] - '0') * 10) + (buf[2] - '0') + (atof(buf + 3) / 60.0f);
	//*pf = ((buf[0] - '0') * 100) + ((buf[1] - '0') * 10) + (buf[2] - '0') + (atof(buf + 3) / 100.0f);
	*pf = atof(buf);
	return 0;
}

#define GEO_FENCE_CMD__DEFAULT_CMD_CNT	3
#define GEO_FENCE_CMD__GEOFENCE_CMD_START_IDX	2
#define GEO_FENCE_CMD__GEOFENCE_CMD_CNT	5

static int _fence_set (int argc, char **argv)
{
	lotte_gps_manager_t *pgps_mng;

	char *p_str_address;
	char *p_str_condition;
	char *p_str_lat;
	char *p_str_lon;
	char *p_str_range;
//	char *p_str_response;

	int fence_address;

	gpsData_t cur_gpsdata;
	geo_fence_data_mgr_t parse_fence[MAX_FENCE_DATA_CNT]; // max support geofence count :: 5
//	geo_fence_data_t fence; // max support geofence count :: 5
	
	int ret;

//	int i = 0;
	int j = 0;
	int geofence_data_idx = 0;
//	int genfence_data_cnt = 0;

	int argc_remain_cnt = 0;
	int arg_geofence_cnt = 0;

	int first_event_send = get_geo_first_event();

	// real argument cnt ^^;;;
	argc++;

	LOGI(LOG_TARGET, " >> SMS GEOF ::  geo fence sms proc [%d]\n", argc);
	//for( i = 0 ; i <= argc ; i ++ )
	//	printf(" >>>> [%d] => [%s]\r\n", i, argv[i]);

	if ( argc < 4 )
	{
		LOGE(LOG_TARGET, "<%s> error invalid argument case 1 [%d]\n", __FUNCTION__, argc);
		return -1;
	}

	argc_remain_cnt = argc - GEO_FENCE_CMD__DEFAULT_CMD_CNT;
	if (( (argc_remain_cnt / GEO_FENCE_CMD__GEOFENCE_CMD_CNT) <= 0 ) || ( (argc_remain_cnt % GEO_FENCE_CMD__GEOFENCE_CMD_CNT) != 0 ))
	{
		LOGE(LOG_TARGET, "<%s> error invalid argument case 2 [%d]\n", __FUNCTION__, argc);
		return -1;
	}
	
	arg_geofence_cnt = (argc_remain_cnt / GEO_FENCE_CMD__GEOFENCE_CMD_CNT);
	LOGI(LOG_TARGET, " >> SMS GEOF :: arg_geofence_cnt is [%d]/[%d]\r\n", argc_remain_cnt, arg_geofence_cnt);

	if ( arg_geofence_cnt <= 0 )
		return 0;
	
	//clear_geo_fence();
		
	for( j = 0 ; j < arg_geofence_cnt ; j++ )
	{
		geofence_data_idx = j * GEO_FENCE_CMD__GEOFENCE_CMD_CNT + GEO_FENCE_CMD__GEOFENCE_CMD_START_IDX;
		printf("geofence_data_idx is [%d]\r\n", geofence_data_idx);
		{
			p_str_address   = argv[geofence_data_idx + 0];
			p_str_condition = argv[geofence_data_idx + 1];
			p_str_lat       = argv[geofence_data_idx + 2];
			p_str_lon       = argv[geofence_data_idx + 3];
			p_str_range     = argv[geofence_data_idx + 4];
	
			fence_address = atoi(p_str_address);
			parse_fence[j].idx = fence_address;
		
			switch(atoi(p_str_condition))
			{
				case 0:
					parse_fence[j].fence_data.setup_fence_status = eFENCE_SETUP_ENTRY;
					break;
				case 1:
					parse_fence[j].fence_data.setup_fence_status = eFENCE_SETUP_EXIT;
					break;
				case 2:
					parse_fence[j].fence_data.setup_fence_status = eFENCE_SETUP_ENTRY_EXIT;
					break;
			}

			if(_lat_nmea2float(&p_str_lat[0], &(parse_fence[j].fence_data.latitude)) < 0)
			{
				LOGE(LOG_TARGET, "<%s> lat format error\n", __FUNCTION__);
				parse_fence[j].fence_data.latitude = 0;
			}

			if(_lon_nmea2float(&p_str_lon[0], &(parse_fence[j].fence_data.longitude)) < 0)
			{
				LOGE(LOG_TARGET, "<%s> lon format error\n", __FUNCTION__);
				parse_fence[j].fence_data.latitude = 0;
			}
			
			parse_fence[j].fence_data.range = atoi(p_str_range);
			
			if  ( ( parse_fence[j].fence_data.latitude == 0 ) || (parse_fence[j].fence_data.latitude == 0) )
			{
				parse_fence[j].fence_data.enable = eGEN_FENCE_DISABLE;
			}
			else
			{
				parse_fence[j].fence_data.enable = eGEN_FENCE_ENABLE;
			}
		}
	}
	
	for( j = 0 ; j < arg_geofence_cnt ; j++ )
	{
		printf("parse_fence[%d].idx => [%d]\r\n", j, parse_fence[j].idx);
		printf("parse_fence[%d].enable => [%d]\r\n", j, parse_fence[j].fence_data.enable);
		printf("parse_fence[%d].latitude => [%3.5f]\r\n", j, parse_fence[j].fence_data.latitude);
		printf("parse_fence[%d].longitude => [%3.5f]\r\n", j, parse_fence[j].fence_data.longitude);
		printf("parse_fence[%d].range => [%d]\r\n", j, parse_fence[j].fence_data.range);
		printf("parse_fence[%d].cur_fence_status => [%d]\r\n", j, parse_fence[j].fence_data.cur_fence_status);
		printf("parse_fence[%d].setup_fence_status => [%d]\r\n", j, parse_fence[j].fence_data.setup_fence_status);

/*
		LOGD(LOG_TARGET, "save addres = %d \n",fence_address);
		LOGD(LOG_TARGET, "event cond = %d \n",fence_cond);
		LOGD(LOG_TARGET, "lat = %3.5f \n",sms_lat);
		LOGD(LOG_TARGET, "lon = %3.5f \n",sms_lon);
		LOGD(LOG_TARGET, "fence range = %d \n",fence_range);
		LOGD(LOG_TARGET, "response = %s \n",argv[6]);
		*/
	}

	for( j = 0 ; j < arg_geofence_cnt ; j++ )
	{
		int dist = 0;
		pgps_mng = load_gps_manager();
		
		if(pgps_mng->first_gps_active == eINACTIVE || pgps_mng->fixed_gpsdata.active  == eINACTIVE)
		{
			//current gps
			gps_get_curr_data(&cur_gpsdata);
			dist = get_distance_meter(cur_gpsdata.lat, parse_fence[j].fence_data.latitude, cur_gpsdata.lon, parse_fence[j].fence_data.longitude);
		}
		else
		{
			dist = get_distance_meter(pgps_mng->fixed_gpsdata.lat, parse_fence[j].fence_data.latitude, pgps_mng->fixed_gpsdata.lon, parse_fence[j].fence_data.longitude);
		}

		
		if(dist < 0)
			parse_fence[j].fence_data.cur_fence_status = eFENCE_STATUS_OUT;
		else if(dist < parse_fence[j].fence_data.range)
			parse_fence[j].fence_data.cur_fence_status = eFENCE_STATUS_IN;
		else
			parse_fence[j].fence_data.cur_fence_status = eFENCE_STATUS_OUT;

		// parse_fence[j].fence_data.cur_fence_status = eFENCE_NONE_NOTIFICATION;

		// need to first event check.
		if ( (dist < parse_fence[j].fence_data.range) && first_event_send )
		{
			parse_fence[j].fence_data.cur_fence_status = eFENCE_NONE_NOTIFICATION;
			switch(parse_fence[j].idx)
			{
				case 0 :
					//sender_add_data_to_buffer(eGEO_FENCE_NUM0_ENTRY_EVT , NULL, ePIPE_1);
					break;
				case 1 :
					//sender_add_data_to_buffer(eGEO_FENCE_NUM1_ENTRY_EVT , NULL, ePIPE_1);
					break;
				case 2 :
					//sender_add_data_to_buffer(eGEO_FENCE_NUM2_ENTRY_EVT , NULL, ePIPE_1);
					break;
				case 3 : 
					//sender_add_data_to_buffer(eGEO_FENCE_NUM3_ENTRY_EVT , NULL, ePIPE_1);
					break;
				case 4 : 
					//sender_add_data_to_buffer(eGEO_FENCE_NUM4_ENTRY_EVT , NULL, ePIPE_1);
					break;
				default:
					break;
			}
		}

		//clear_geo_fence(parse_fence[j].idx, 0);
		if ( parse_fence[j].fence_data.enable == 1)
			devel_webdm_send_log("gf[%d]=[%f][%f]", parse_fence[j].idx, parse_fence[j].fence_data.latitude, parse_fence[j].fence_data.longitude);

		switch(parse_fence[j].idx)
		{
			case 0:
				ret = set_geo_fence0(parse_fence[j].fence_data); //in this function, auto geo fence contents update
				break;
			case 1:
				ret = set_geo_fence1(parse_fence[j].fence_data); //in this function, auto geo fence contents update
				break;
			case 2:
				ret = set_geo_fence2(parse_fence[j].fence_data); //in this function, auto geo fence contents update
				break;
			case 3:
				ret = set_geo_fence3(parse_fence[j].fence_data); //in this function, auto geo fence contents update
				break;
			case 4:
				ret = set_geo_fence4(parse_fence[j].fence_data); //in this function, auto geo fence contents update
				break;
			default:
				ret = -1;
				break;
		}
	}
	//TODO
	// save_geo_fence_data();

	if(atoi(argv[argc-1]) == SMS_CMD_RESPONSE_NEED)
	{
//		char 597[100] = {0,};
		sender_add_data_to_buffer(eGEO_FENCE_SETUP, NULL, ePIPE_1);
		
		//sprintf(smsmsg, "geo fence> %d:%d:%3.5f:%3.5f:%d\n", fence_address, fence_cond, sms_lat, sms_lon, fence_range);
		//devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	}

	return 0;
}


static int _invoice_set (int argc, char **argv)
{
	char *p_str_invoice_info;
	char *p_str_response;


	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS\n", __FUNCTION__);
		return -1;
	}

	p_str_invoice_info   = argv[1];
	p_str_response       = argv[2];
	LOGD(LOG_TARGET, "%s> invoice msg pw = %s \n", __func__, p_str_invoice_info);
	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);

	set_nisso_pkt__invoice_info(atoi(p_str_invoice_info));

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		sender_add_data_to_buffer(eINVOCE_RECV_EVT, NULL, ePIPE_1);
	}
	
	return 0;
}


static int _invoice_set_2 (int argc, char **argv)
{
	char *p_str_invoice_info;
    char* p_str_invoice_date;
    char* p_str_invoice_info_num_1;
    char* p_str_invoice_info_num_2;
	char *p_str_response;


	if(argc != 5)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS\n", __FUNCTION__);
		return -1;
	}

	p_str_invoice_info   = argv[1];
    p_str_invoice_date       = argv[2];
    p_str_invoice_info_num_1       = argv[3];
    p_str_invoice_info_num_2       = argv[4];
	p_str_response       = argv[5];
    

	LOGD(LOG_TARGET, "%s> invoice msg = %s \n", __func__, p_str_invoice_info);
    LOGD(LOG_TARGET, "%s> p_str_invoice_date msg = %s \n", __func__, p_str_invoice_date);
    LOGD(LOG_TARGET, "%s> p_str_invoice_info_num_1 msg = %s \n", __func__, p_str_invoice_info_num_1);
    LOGD(LOG_TARGET, "%s> p_str_invoice_info_num_2 msg = %s \n", __func__, p_str_invoice_info_num_2);
	LOGD(LOG_TARGET, "%s> response = %s \n"    , __func__, p_str_response);

    {
        invoice_info_2_t invoice_info = {0,};
        //init_nisso_pkt__invoice_info_2();
        invoice_info.invoice = atoi(p_str_invoice_info);
        sprintf(invoice_info.p_str_invoice_date, "%8s", p_str_invoice_date);
        sprintf(invoice_info.p_str_invoice_info_num_1, "%8s", p_str_invoice_info_num_1);
        sprintf(invoice_info.p_str_invoice_info_num_2, "%8s", p_str_invoice_info_num_2);
        set_nisso_pkt__invoice_info_2(&invoice_info);
        print_nisso_pkt__invoice_info_2();
    }

	if(atoi(p_str_response) == SMS_CMD_RESPONSE_NEED)
	{
		sender_add_data_to_buffer(eINVOCE_RECV_EVT, NULL, ePIPE_1);
	}
	
	return 0;
}



#ifdef BOARD_TL500K
#ifdef KT_FOTA_ENABLE
static int _get_version (char *pn)
{
	at_send_sms(pn, KT_FOTA_VER, 3);
	return 0;
}
#endif

static int _get_modem_status (char *pn)
{
	/*
	char buff[512] = {0};
	int ret = 0;
	
	ret = get_modem_status(buff);
	if(ret >= 0)
	{
		at_send_sms(pn, buff);
	}
*/
	return 0;
}
#endif

#include <stdlib.h>
#include <string.h>

#include <at/at_util.h>
#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/gpstool.h>
#include <base/thread.h>
#include <board/power.h>
#include <util/nettool.h>
#include <util/validation.h>
#include <util/poweroff.h>

#include <board/board_system.h>

#include <config.h>
#include <validation.h>
#include <report.h>
#include <netcom.h>
#include <callback.h>
#include "logd/logd_rpc.h"
#include "sms.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

void _deinit_essential_functions(void);

static int _server_set(int argc, char **argv, ciprmcSmsType_t type);
static int _rpt_cycle(int argc, char **argv);
static int _mileage_set (int argc, char **argv);
static int _mdt_status (int argc, char **argv);
static int _gpio_mode (int argc, char **argv);
static int _gpio_output (int argc, char **argv);
static int _mdt_reset (int argc, char **argv);
static int _cycle_term (int argc, char **argv);
static int _fence_set (int argc, char **argv);
static int _sys_get_status(char *phonenum, int argc, char **argv);

int parse_model_sms(char *time, char *phonenum, char *sms)
{
	int msg_type = 0;
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};
	char *base = 0;
	int len = 0;
	int i = 0;

	base = sms;
	len = strlen(sms);

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
	msg_type = atoi(model_argv[0]);
	
	switch(msg_type) {
		case eMSG_IP_SET:
			ret = _server_set(model_argc, model_argv, eMSG_IP_SET);
			break;
		case eMSG_RPT_CYCLE:
			ret = _rpt_cycle(model_argc, model_argv);
			break;
		case eMSG_COT_DISTANCE:
			ret = _mileage_set(model_argc, model_argv);
			break;
		case eMSG_MDT_STATUS:
			ret = _mdt_status(model_argc, model_argv);
			break;
		case eMSG_GPIO_MODE:
			ret = _gpio_mode(model_argc, model_argv);
			break;
		case eMSG_GPIO_OUTPUT:
			ret = _gpio_output(model_argc, model_argv);
			break;
		case eMSG_MDT_RESET:
			ret = _mdt_reset(model_argc, model_argv);
			break;
		case eMSG_CYCLE_TERM:
			ret = _cycle_term(model_argc, model_argv);
			break;
		case eMSG_FENCE_SET:
			ret = _fence_set(model_argc, model_argv);
			break;
		case eMSG_IP_SET_REQ:
			ret = _server_set(model_argc, model_argv, eMSG_IP_SET_REQ);
			break;
		case eMSG_SYS_STATUS_REQ:
			ret = _sys_get_status(phonenum, model_argc, model_argv);
		default:
			LOGE(LOG_TARGET, "rmc_sms_parsing ERROR = %d", msg_type);
			ret = -1;
	}

	return ret;
}

static int _server_set(int argc, char **argv, ciprmcSmsType_t type)
{
	configurationModel_t * conf = get_config_model();
	int powerType = 0;

	if(argc != 4)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}
	
	LOGD(LOG_TARGET, "ini_ip_setting PW = %s \n",argv[1]);
	LOGD(LOG_TARGET, "ini_ip_setting IP = %s \n",argv[2]);
	LOGD(LOG_TARGET, "ini_ip_setting PORT = %s \n",argv[3]);
	LOGD(LOG_TARGET, "ini_ip_setting response = %s \n",argv[4]);
	
	if(strcmp(SMS_PW_SETIP, argv[1])!=0)
	{
		return -1;
	}

	if(type == eMSG_IP_SET)
	{
		strncpy(conf->model.report_ip, argv[2], sizeof(conf->model.report_ip));
		conf->model.report_port = atoi(argv[3]);

		if(save_config_user("user:report_ip", argv[2]) < 0)
		{
			return -1;
		}
		if(save_config_user("user:report_port", argv[3]) < 0)
		{
			return -1;
		}
	}
	else
	{
		strncpy(conf->model.request_ip, argv[2], sizeof(conf->model.request_ip));
		conf->model.request_port = atoi(argv[3]);
		
		if(save_config_user("user:request_ip", argv[2]) < 0)
		{
			return -1;
		}
		if(save_config_user("user:request_port", argv[3]) < 0)
		{
			return -1;
		}
	}
	
	setting_network_param();

	if(atoi(argv[4]) == 1)
	{
		if(power_get_power_source() == POWER_SRC_DC)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
    		sender_add_data_to_buffer(REPORT_SET_IP, &powerType, ePIPE_2);
    	}

    	return 0;
}

static int _rpt_cycle(int argc, char **argv)
{
	configurationModel_t * conf = get_config_model();
	unsigned int report_interval = atoi(argv[1]);
	unsigned int collect_interval = atoi(argv[2]);
	int powerType = 0;

	if(argc != 3)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "report cycle = %s \n",argv[1]);
	LOGD(LOG_TARGET, "create cycle = %s \n",argv[2]);
	LOGD(LOG_TARGET, "response = %s \n",argv[3]);

	if(validation_model_report_interval(collect_interval, report_interval) < 0)
	{
		LOGE(LOG_TARGET, "<%s> Validation interval SMS", __FUNCTION__);
		return -1;
	}

	conf->model.report_interval_keyon = report_interval;
	conf->model.collect_interval_keyon = collect_interval;
	conf->model.collect_interval_keyoff = report_interval;
	conf->model.report_interval_keyoff = collect_interval;

	if(save_config_user("user:collect_interval_keyon", argv[2]) < 0)
	{
		return -1;
	}
	if(save_config_user("user:report_interval_keyon", argv[1]) < 0)
	{
		return -1;
	}
	if(save_config_user("user:collect_interval_keyoff", argv[2]) < 0)
	{
		return -1;
	}
	if(save_config_user("user:report_interval_keyoff", argv[1]) < 0)
	{
		return -1;
	}

	if(atoi(argv[3]) == 1)
	{
		if(power_get_power_source() == POWER_SRC_DC)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_SET_INTERVAL, &powerType, ePIPE_2);
	}

	return 0;
}

static int _mileage_set (int argc, char **argv)
{
	unsigned int mileage = 0;
	int powerType = 0;

	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "total distance  = %s \n",argv[1]);
	LOGD(LOG_TARGET, "response = %s \n",argv[2]);

	mileage = atoi(argv[1]);
	if( mileage > 1000000000 )
	{
		return -1;
	}
	mileage_set_m(mileage);
	mileage_write();

	if(atoi(argv[2]) == 1)
	{
		if(power_get_power_source() == POWER_SRC_DC)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_SET_MILEAGE, &powerType, ePIPE_2);
	}

	return 0;
}

static int _mdt_status (int argc, char **argv)
{
	int powerType = 0;

	if(argc != 1)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "mdt_status response = %s \n",argv[1]);
	
	if(atoi(argv[1]) == 1)
	{
		if(power_get_power_source() == POWER_SRC_DC)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_STATUS, &powerType, ePIPE_2);
	}

	return 0;
}

static int _gpio_mode (int argc, char **argv)
{
	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "gpio_mode = %s \n",argv[1]);
	LOGD(LOG_TARGET, "response = %s \n",argv[2]);

	return 0;
}

static int _gpio_output (int argc, char **argv)
{
	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}
	
	LOGD(LOG_TARGET, "gpio_output = %s \n",argv[1]);
	LOGD(LOG_TARGET, "response = %s \n",argv[2]);

	return 0;
}

static int _mdt_reset (int argc, char **argv)
{
	int powerType = 0;

	if(argc != 2)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "mdt_reset pw = %s \n",argv[1]);
	LOGD(LOG_TARGET, "response = %s \n",argv[2]);

	if(strcmp(SMS_PW_RESET, argv[1])!=0)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect RESET PW SMS", __FUNCTION__);
		return -1;
	}
    	
	if(atoi(argv[2]) == 1)
	{
		if(power_get_power_source() == POWER_SRC_DC)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_RESET, &powerType, ePIPE_2);
	}

	_deinit_essential_functions();
	terminate_app_callback();
	poweroff(__FUNCTION__, sizeof(__FUNCTION__));

	return 0;
}

static int _cycle_term (int argc, char **argv)
{
	if(argc != 5)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "cycle_term = %s \n",argv[1]);
	LOGD(LOG_TARGET, "create cycle  = %s \n",argv[2]);
	LOGD(LOG_TARGET, "report power save mode = %s \n",argv[3]);
	LOGD(LOG_TARGET, "create power save mode = %s \n",argv[4]);
	LOGD(LOG_TARGET, "response = %s \n",argv[5]);

	return 0;
}

static int _fence_set (int argc, char **argv)
{
	if(argc != 6)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	LOGD(LOG_TARGET, "save addres = %s \n",argv[1]);
	LOGD(LOG_TARGET, "event case = %s \n",argv[2]);
	LOGD(LOG_TARGET, "lat = %s \n",argv[3]);
	LOGD(LOG_TARGET, "long = %s \n",argv[4]);
	LOGD(LOG_TARGET, "fence set = %s \n",argv[5]);
	LOGD(LOG_TARGET, "response = %s \n",argv[6]);

	return 0;
}

static int _sys_get_status(char *phonenum, int argc, char **argv)
{
	char buff[181] = {0};
	gpsData_t gpsdata;
	int mode = 0;

	char st_pwr, st_key, st_net, st_gps;

	configurationModel_t * conf = get_config_model();

	if(argc != 1)
	{
		LOGE(LOG_TARGET, "<%s> Incorrect SMS", __FUNCTION__);
		return -1;
	}

	if(power_get_power_source() == POWER_SRC_DC)
	{
		st_pwr = 'O';
	}
	else
	{
		st_pwr = 'X';
	}

	if(power_get_ignition_status() == POWER_IGNITION_ON)
	{
		st_key = 'O';
	}
	else
	{
		st_key = 'X';
	}

	if(nettool_get_state() == DEFINES_MDS_OK)
	{
		st_net = 'O';
	}
	else
	{
		st_net = 'X';
	}

	gps_get_curr_data(&gpsdata);
	if(gpsdata.active == 1)
	{
		st_gps = 'O';
	}
	else
	{
		st_gps = 'X';
	}

	snprintf(buff, sizeof(buff)-1, "PWR-%c,KEY-%c,NET-%c,GPS-%c,%f,%f,%s:%d,%d/%d",
		st_pwr, st_key, st_net, st_gps, gpsdata.lat, gpsdata.lon,
		conf->model.report_ip, conf->model.report_port,
		conf->model.collect_interval_keyon, conf->model.report_interval_keyon);

	if(validation_check_phonenum(phonenum, strlen(phonenum)) == DEFINES_MDS_NOK)
	{
		return -1;
	}
	
	mode = atoi(argv[1]);

	if(mode == 1)
	{
		at_send_sms(phonenum, buff);
	}
	else if(mode == 2)
	{
		devel_webdm_send_log(buff);
	}

	return 0;
}


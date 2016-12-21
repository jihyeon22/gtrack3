#include <stdio.h>
#include <unistd.h>

#include <iniparser.h>

#include <base/config.h>
#include <util/tools.h>
#include <logd_rpc.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE


#ifdef DEBUG
static configurationBase_t config =
{
	.noti.turn_on_sms_enable = 1,
	.noti.target_num = NOTI_DEFAULT_PHONNUM
};
#else
static configurationBase_t config =
{
	.common.bootstrap = 0,
	.common.initial_turnoff = 1,
	.common.first_pwr_status_on= 1,
	.common.time_hold_power = 15,
	.common.time_hold_ign = 10,
	.common.log_poweroff = 0,
	.common.sms_passwd = "default",
	.common.alive_time_sec = 15,
#ifdef USE_GPS_MODEL
	.gps.gps_time_zone = 9,
	.gps.adjust_gps = 1,
	.gps.gps_err_enable = 1,
	.gps.gps_err_speed_kms = 200,
	.gps.gps_err_dist_kms = 150,
	.gps.gps_err_ignore_dist_m = 1500,
	.gps.gps_err_first_dist_m = 2000,
	.gps.gps_err_max_dist_m = 12000,

	.mileage.enable = 1,
	.mileage.daily_mileage = 0,
#endif
	.webdm.enable = 1,
	.webdm.tx_power = 1,
	.webdm.tx_ignition = 1,
	.webdm.tx_report = 0,
	.webdm.tx_log = 1,

	.noti.turn_on_sms_enable = 0,
};
#endif

configurationBase_t* load_config_base(void)
{
	dictionary *inf = NULL;
	int ntry = 0;

	if(tools_check_exist_file(CONFIG_FILE_PATH, 10) < 0)
	{
		return NULL;
	}

	while(++ntry <= INI_RETRY_CNT)
	{
		if(inf != NULL)
		{
			iniparser_freedict(inf);
			inf = NULL;
		}
		
		if(!(inf = iniparser_load(CONFIG_FILE_PATH)))
		{
			inf = NULL;
			continue;
		}

		if(get_value_ini_int(inf, "webdm:enable", &config.webdm.enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "webdm:tx_power", &config.webdm.tx_power) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "webdm:tx_ignition", &config.webdm.tx_ignition) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "webdm:tx_report", &config.webdm.tx_report) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "webdm:tx_log", &config.webdm.tx_log) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "noti:turn_on_sms", &config.noti.turn_on_sms_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "noti:target_num", config.noti.target_num, sizeof(config.noti.target_num)) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "common:bootstrap", &config.common.bootstrap) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:initial_turnoff", &config.common.initial_turnoff) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:first_pwr_status_on", &config.common.first_pwr_status_on) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:time_hold_power", &config.common.time_hold_power) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:time_hold_ign", &config.common.time_hold_ign) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:log_poweroff", &config.common.log_poweroff) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "common:sms_passwd", config.common.sms_passwd, sizeof(config.common.sms_passwd)) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "common:alive_time_sec", &config.common.alive_time_sec) < 0)
		{
			continue;
		}
#ifdef USE_GPS_MODEL
		if(get_value_ini_int(inf, "gps:gps_time_zone", &config.gps.gps_time_zone) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:adjust_gps", &config.gps.adjust_gps) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_enable", &config.gps.gps_err_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_speed_kms", &config.gps.gps_err_speed_kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_dist_kms", &config.gps.gps_err_dist_kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_ignore_dist_m", &config.gps.gps_err_ignore_dist_m) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_first_dist_m", &config.gps.gps_err_first_dist_m) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "gps:gps_err_max_dist_m", &config.gps.gps_err_max_dist_m) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "mileage:enable", &config.mileage.enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "mileage:daily_mileage", &config.mileage.daily_mileage) < 0)
		{
			continue;
		}
#endif
		break; //if success, break loop.
	}

	if(inf != NULL)
	{
		iniparser_freedict(inf);
		inf = NULL;
	}

	if(ntry > INI_RETRY_CNT)
	{
		return NULL;
	}
	else
	{
		return &config;
	}
}

/*===========================================================================================*/
configurationBase_t* get_config_base(void)
{
	return &config;
}

/*===========================================================================================*/

int save_config(const char *name, char *msg) {
	dictionary  *ini ;

	if(tools_check_exist_file(CONFIG_FILE_PATH, 10) < 0)
	{
		return -1;
	}	

	ini = iniparser_load(CONFIG_FILE_PATH);
	if(ini == NULL) {
		LOGE(LOG_TARGET, "cannot parse file: %s\n", CONFIG_FILE_PATH);
		return -1;
	}
	
	if(iniparser_set(ini, name, msg) < 0)
	{
		iniparser_freedict(ini);
		return -1;
	}

	iniparser_store(ini, CONFIG_FILE_PATH);

	iniparser_freedict(ini);
	
	return 0;
}

int save_config_array(iniData_t *data) {
	dictionary  *ini ;
	int i = 0;

	if(tools_check_exist_file(CONFIG_FILE_PATH, 10) < 0)
	{
		return -1;
	}	

	ini = iniparser_load(CONFIG_FILE_PATH);
	if(ini == NULL) {
		LOGE(LOG_TARGET, "cannot parse file: %s\n", CONFIG_FILE_PATH);
		return -1;
	}

	for(i=0 ; i<INI_MAX_SAVE_ARRAY; i++)
	{
		if(data[i].name == NULL || data[i].msg == NULL)
		{
			break;
		}
		
		if(iniparser_set(ini, data[i].name, data[i].msg) < 0)
		{
			iniparser_freedict(ini);
			return -1;
		}
	}
	
	iniparser_store(ini, CONFIG_FILE_PATH);

	iniparser_freedict(ini);
	
	return 0;
}

int save_config_user(const char *name, char *msg)
{
	dictionary  *ini ;

	if(tools_check_exist_file(CONFIG_USER_FILE_PATH, 10) < 0)
	{
		if(tools_cp(CONFIG_USER_ORG_FILE_PATH, CONFIG_USER_FILE_PATH, 1) < 0)
		{
			return -1;
		}
	}

	ini = iniparser_load(CONFIG_USER_FILE_PATH);
	if(ini == NULL) {
		LOGE(LOG_TARGET, "cannot parse file: %s\n", CONFIG_USER_FILE_PATH);
		return -1;
	}
	
	if(iniparser_set(ini, name, msg) < 0)
	{
		iniparser_freedict(ini);
		return -1;
	}

	iniparser_store(ini, CONFIG_USER_FILE_PATH);

	iniparser_freedict(ini);
	
	return 0;
}

int save_config_array_user(iniData_t *data) {
	dictionary  *ini ;
	int i = 0;

	if(tools_check_exist_file(CONFIG_USER_FILE_PATH, 10) < 0)
	{
		return -1;
	}	

	ini = iniparser_load(CONFIG_USER_FILE_PATH);
	if(ini == NULL) {
		LOGE(LOG_TARGET, "cannot parse file: %s\n", CONFIG_USER_FILE_PATH);
		return -1;
	}

	for(i=0 ; i<INI_MAX_SAVE_ARRAY; i++)
	{
		if(data[i].name == NULL || data[i].msg == NULL)
		{
			break;
		}
		
		if(iniparser_set(ini, data[i].name, data[i].msg) < 0)
		{
			iniparser_freedict(ini);
			return -1;
		}
	}
	
	iniparser_store(ini, CONFIG_USER_FILE_PATH);

	iniparser_freedict(ini);
	
	return 0;
}

int get_value_ini_int(dictionary *d, const char *key, int *var)
{
	int ret = 0;
	
	ret = iniparser_getint(d, key, INI_INVALID_KEY_INT);
	if(ret == INI_INVALID_KEY_INT)
	{
		return -1;
	}
	else
	{
		*var = ret;
	}
	
	return 0;
}

int get_value_ini_str(dictionary *d, const char *key, char *buff, const int buff_len)
{
	char *ret = 0;

	ret = iniparser_getstring(d, key, INI_INVALID_KEY_STRING);
	if(ret == INI_INVALID_KEY_STRING)
	{
		return -1;
	}
	else
	{
		strncpy(buff, ret, buff_len);
	}
	return 0;
}


#include <stdio.h>
#include <unistd.h>

#include <iniparser.h>
#include <dictionary.h>

#include <base/config.h>
#include <util/tools.h>
#include "config.h"
#include "debug.h"
#include "logd/logd_rpc.h"

static configurationModel_t config;

configurationModel_t* load_config_model(void)
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

		if(get_value_ini_int(inf, "model:tempature_enable", &config.model.tempature_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "model:tempature_device", config.model.tempature_device, sizeof(config.model.tempature_device)) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "model:tempature_cycle", &config.model.tempature_cycle) < 0)
		{
			continue;
		}
		
		//dist filter
		if(get_value_ini_int(inf, "dist_gps_filter:enable", &config.model.dist_filter_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "dist_gps_filter:value", &config.model.dist_filter_value) < 0)
		{
			continue;
		}
		
		//sat filter
		if(get_value_ini_int(inf, "sat_gps_filter:enable", &config.model.sat_filter_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "sat_gps_filter:value", &config.model.sat_filter_value) < 0)
		{
			continue;
		}

		//hdop filter
		if(get_value_ini_int(inf, "hdop_gps_filter:enable", &config.model.hdop_filter_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "hdop_gps_filter:value", &config.model.hdop_filter_value) < 0)
		{
			continue;
		}

		//speed filter
		if(get_value_ini_int(inf, "speed_gps_filter:enable", &config.model.speed_filter_enable) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "speed_gps_filter:value", &config.model.speed_filter_value) < 0)
		{
			continue;
		}

		//model name
		if(get_value_ini_str(inf, "model:model_name", config.model.model_name, sizeof(config.model.model_name)) < 0)
		{
			continue;
		}

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

configurationModel_t* get_config_model(void)
{
	return &config;
}

configurationModel_t* load_config_user(void)
{
	dictionary *inf = NULL;
	int ntry = 0;
	char org_model_name[16] = {0};
	int org_setting_date = 0;
	char cur_model_name[16] = {0};
	int cur_setting_date = 0;

	if(tools_check_exist_file(CONFIG_USER_FILE_PATH, 5) < 0)
	{
		if(tools_cp(CONFIG_USER_ORG_FILE_PATH, CONFIG_USER_FILE_PATH, 1) < 0)
		{
			return NULL;
		}
	}

	while(++ntry <= INI_RETRY_CNT)
	{		
		if(inf != NULL)
		{
			iniparser_freedict(inf);
			inf = NULL;
		}
		
		if(!(inf = iniparser_load(CONFIG_USER_ORG_FILE_PATH)))
		{
			inf = NULL;
			continue;
		}
		
		if(get_value_ini_str(inf, "user:model_name", org_model_name, sizeof(org_model_name)) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "user:date", &org_setting_date) < 0)
		{
			continue;
		}

		break;
	}

	if(inf != NULL)
	{
		iniparser_freedict(inf);
		inf = NULL;
	}

	if(ntry > INI_RETRY_CNT)
	{
		printf("org read error\n");
		return NULL;
	}

	ntry = 0;
	while(++ntry <= INI_RETRY_CNT)
	{		
		if(inf != NULL)
		{
			iniparser_freedict(inf);
			inf = NULL;
		}
		
		if(!(inf = iniparser_load(CONFIG_USER_FILE_PATH)))
		{
			inf = NULL;
			continue;
		}
		
		if(get_value_ini_str(inf, "user:model_name", cur_model_name, sizeof(cur_model_name)) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "user:date", &cur_setting_date) < 0)
		{
			continue;
		}

		break;
	}

	printf("config setting name diff : %s %s\n", org_model_name, cur_model_name);
	printf("config setting date diff : %d %d\n", org_setting_date, cur_setting_date);
	if(ntry > INI_RETRY_CNT || strcmp(org_model_name, cur_model_name)!=0 || cur_setting_date < org_setting_date)
	{
		if(tools_cp(CONFIG_USER_ORG_FILE_PATH, CONFIG_USER_FILE_PATH, 1) < 0)
		{
			return NULL;
		}
	}

	ntry = 0;	
	while(++ntry <= INI_RETRY_CNT)
	{
		if(inf != NULL)
		{
			iniparser_freedict(inf);
			inf = NULL;
		}
		
		if(!(inf = iniparser_load(CONFIG_USER_FILE_PATH)))
		{
			inf = NULL;
			continue;
		}

		if(get_value_ini_str(inf, "user:report_ip", config.model.report_ip, sizeof(config.model.report_ip)) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:report_port", &config.model.report_port) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:collect_interval_keyon", &config.model.collect_interval_keyon) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:collect_interval_keyoff", &config.model.collect_interval_keyoff) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:report_interval_keyon", &config.model.report_interval_keyon) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:report_interval_keyoff", &config.model.report_interval_keyoff) < 0)
		{
			continue;
		}

		//model name
		if(get_value_ini_str(inf, "user:model_name", config.model.model_name, sizeof(config.model.model_name)) < 0)
		{
			continue;
		}

		break; //if success, break loop.
	}

	LOGT(LOG_TARGET, "MDT report_ip %s\n", config.model.report_ip);
	LOGT(LOG_TARGET, "MDT port %d\n", config.model.report_port);
	LOGT(LOG_TARGET, "MDT key on collect %d\n", config.model.collect_interval_keyon);
	LOGT(LOG_TARGET, "MDT key on report %d\n", config.model.report_interval_keyon);
	LOGT(LOG_TARGET, "MDT key off collect %d\n", config.model.collect_interval_keyoff);
	LOGT(LOG_TARGET, "MDT key off report %d\n", config.model.report_interval_keyoff);

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

void load_config_base_default(void)
{
	configurationBase_t *conf_base = get_config_base();

	conf_base->common.bootstrap = 0;
	conf_base->common.initial_turnoff = 1;
	conf_base->common.first_pwr_status_on= 1;
	conf_base->common.time_hold_power = 15;
	conf_base->common.time_hold_ign = 10;
	conf_base->common.log_poweroff = 0;
	strcpy(conf_base->common.sms_passwd, "moram");
	conf_base->gps.adjust_gps = 1;
	conf_base->gps.gps_time_zone = 9;
	conf_base->gps.gps_err_enable = 1;
	conf_base->gps.gps_err_speed_kms = 200;
	conf_base->gps.gps_err_dist_kms = 150;
	conf_base->gps.gps_err_ignore_dist_m = 0;
	conf_base->gps.gps_err_first_dist_m = 2000;
	conf_base->gps.gps_err_max_dist_m = 12000;
	conf_base->mileage.enable = 0;
	conf_base->mileage.daily_mileage = 0;
	conf_base->webdm.enable = 1;
	conf_base->webdm.tx_ignition = 1;
	conf_base->webdm.tx_report = 0;
	conf_base->noti.turn_on_sms_enable = 0;
}

void load_config_model_default(void)
{
#ifdef SERVER_ABBR_MRM
	config.model.tempature_enable = 1;
	strcpy(config.model.tempature_device, "/dev/ttyHSL2");
	config.model.tempature_cycle = 180;
#elif  defined(SERVER_ABBR_MRM0)
	config.model.tempature_enable = 0;
	strcpy(config.model.tempature_device, "/dev/ttyHSL2");
	config.model.tempature_cycle = 180;
#elif  defined(SERVER_ABBR_MRM1)
	config.model.tempature_enable = 1;
	strcpy(config.model.tempature_device, "/dev/ttyHSL2");
	config.model.tempature_cycle = 180;
#else
	config.model.tempature_enable = 1;
	strcpy(config.model.tempature_device, "/dev/ttyHSL2");
	config.model.tempature_cycle = 180;
#endif
	config.model.dist_filter_enable = 1;
	config.model.dist_filter_value = 50;
	config.model.sat_filter_enable = 0;
	config.model.sat_filter_value = 5;
	config.model.hdop_filter_enable = 0;
	config.model.hdop_filter_value = 5;
	config.model.speed_filter_enable = 1;
	config.model.speed_filter_value = 200;
}

void load_config_user_default(void)
{

#ifdef SERVER_ABBR_MRM
	strcpy(config.model.report_ip, "218.232.104.190");
	config.model.report_port = 14100;

	config.model.collect_interval_keyon = 60;
	config.model.collect_interval_keyoff = 60;
	config.model.report_interval_keyon = 60;
	config.model.report_interval_keyoff = 60;

	strcpy(config.model.model_name, "moram");
#elif  defined(SERVER_ABBR_MRM0)
	strcpy(config.model.report_ip, "218.232.104.186");
	config.model.report_port = 14100;

	config.model.collect_interval_keyon = 60;
	config.model.collect_interval_keyoff = 60;
	config.model.report_interval_keyon = 60;
	config.model.report_interval_keyoff = 60;

	strcpy(config.model.model_name, "moram0");

#elif  defined(SERVER_ABBR_MRM1)
	strcpy(config.model.report_ip, "218.232.104.186");
	config.model.report_port = 14100;

	config.model.collect_interval_keyon = 60;
	config.model.collect_interval_keyoff = 60;
	config.model.report_interval_keyon = 60;
	config.model.report_interval_keyoff = 60;

	strcpy(config.model.model_name, "moram1");
#else
	strcpy(config.model.report_ip, "218.232.104.190");
	config.model.report_port = 14100;

	config.model.collect_interval_keyon = 60;
	config.model.collect_interval_keyoff = 60;
	config.model.report_interval_keyon = 60;
	config.model.report_interval_keyoff = 60;

	strcpy(config.model.model_name, "moram");
#endif
	
}


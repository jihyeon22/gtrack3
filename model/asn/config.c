#include <stdio.h>
#include <unistd.h>

#include <iniparser.h>
#include <dictionary.h>

#include <base/config.h>
#include <util/tools.h>
#include "config.h"

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

		config.model.tcp_connect_retry_count = 3;
		config.model.tcp_send_retry_count = 3;
		config.model.tcp_receive_retry_count = 3;
		config.model.tcp_timeout_secs = 60;

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
		//model name
		if(get_value_ini_str(inf, "user:model_name", config.model.model_name, sizeof(config.model.model_name)) < 0)
		{
			continue;
		}
		//model name
		if(get_value_ini_str(inf, "user:HUBID", config.model.hubid, sizeof(config.model.hubid)) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:NetNotUseWarnTime", &config.model.network_not_use_warning_time) < 0)
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

void load_config_base_default(void)
{
	configurationBase_t *conf_base = get_config_base();

	strcpy(conf_base->common.sms_passwd, "asn");

//These values are the same as default value in base/config.h.
#if 0
	conf_base->common.initial_turnoff = 1;
	conf_base->common.first_pwr_status_on= 1;
	conf_base->common.time_hold_power = 15;
	conf_base->common.time_hold_ign = 10;
	conf_base->common.log_poweroff = 0;
	conf_base->common.alive_time_sec = 15;
	conf_base->gps.adjust_gps = 1;
	conf_base->gps.gps_time_zone = 9;
	conf_base->gps.gps_err_enable = 1;
	conf_base->gps.gps_err_speed_kms = 200;
	conf_base->gps.gps_err_dist_kms = 150;
	conf_base->gps.gps_err_ignore_dist_m = 1500;
	conf_base->gps.gps_err_first_dist_m = 2000;
	conf_base->gps.gps_err_max_dist_m = 12000;
	conf_base->mileage.enable = 1;
	conf_base->mileage.daily_mileage = 0;
	conf_base->webdm.enable = 1;
	conf_base->webdm.tx_power = 1;
	conf_base->webdm.tx_ignition = 1;
	conf_base->webdm.tx_report = 0;
	conf_base->noti.turn_on_sms_enable = 0;
#endif
}

void load_config_model_default(void)
{
	config.model.tcp_connect_retry_count = 3;
	config.model.tcp_send_retry_count = 3;
	config.model.tcp_receive_retry_count = 3;
	config.model.tcp_timeout_secs = 60;
}

void load_config_user_default(void)
{
	strncpy(config.model.report_ip, "118.32.191.131", sizeof("118.32.191.131"));
	config.model.report_port = 14300;

	strncpy(config.model.hubid, "12345678", sizeof(config.model.hubid)-1);
	strncpy(config.model.model_name, "asn", sizeof(config.model.model_name)-1);
	config.model.network_not_use_warning_time = 3600;
}


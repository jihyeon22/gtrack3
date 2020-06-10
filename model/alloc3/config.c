#include <stdio.h>
#include <unistd.h>

#include <iniparser.h>
#include <dictionary.h>

#include <base/config.h>
#include <util/tools.h>
#include "config.h"
#include "debug.h"

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
		if(get_value_ini_str(inf, "user:ftp_ip", config.model.ftp_ip, sizeof(config.model.report_ip)) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "user:ftp_port", config.model.ftp_port, sizeof(config.model.ftp_port)) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "user:ftp_id", config.model.ftp_id, sizeof(config.model.ftp_id)) < 0)
		{
			continue;
		}
		if(get_value_ini_str(inf, "user:ftp_pw", config.model.ftp_pw, sizeof(config.model.ftp_pw)) < 0)
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

	strcpy(conf_base->common.sms_passwd, "alloc");
}

void load_config_model_default(void)
{
	config.model.tempature_enable = 0;

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
	strcpy(config.model.report_ip, "61.106.0.92");
	config.model.report_port = 7431;

	config.model.collect_interval_keyon = 30;
	config.model.report_interval_keyon = 30;
	config.model.collect_interval_keyoff = 30;
	config.model.report_interval_keyoff = 30;

	strcpy(config.model.model_name, "alloc");
}

int load_config_base_webdm(void)
{
	configurationBase_t *conf_base = get_config_base();

	printf("conf_base->webdm.enable : %d \n", conf_base->webdm.enable);

	return conf_base->webdm.enable;
}
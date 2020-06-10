#include <stdio.h>
#include <unistd.h>

#include <iniparser.h>
#include <dictionary.h>

#include <base/config.h>
#include <util/tools.h>
#include "config.h"
#include "logd/logd_rpc.h"

#define LOG_TARGET eSVC_MODEL

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
		if(get_value_ini_str(inf, "user:request_ip", config.model.request_ip, sizeof(config.model.request_ip)) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:request_port", &config.model.request_port) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "user:interval_time", &config.model.interval_time) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:max_packet", &config.model.max_packet) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "user:stop_time", &config.model.stop_time) < 0)
		{
			continue;
		}

		if(get_value_ini_int(inf, "user:section_10kms", &config.model.section_10kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_20kms", &config.model.section_20kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_30kms", &config.model.section_30kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_40kms", &config.model.section_40kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_50kms", &config.model.section_50kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_60kms", &config.model.section_60kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_70kms", &config.model.section_70kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_80kms", &config.model.section_80kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_90kms", &config.model.section_90kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_100kms", &config.model.section_100kms) < 0)
		{
			continue;
		}
		if(get_value_ini_int(inf, "user:section_110kms", &config.model.section_110kms) < 0)
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

	strncpy(conf_base->common.sms_passwd, "cl", sizeof("cl"));
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
	strncpy(config.model.report_ip, "218.54.45.50", sizeof("218.54.45.50"));
	config.model.report_port = 887;
	strncpy(config.model.request_ip, "218.54.45.50", sizeof("218.54.45.50"));
	config.model.request_port = 887;

	config.model.interval_time = 60;
	config.model.max_packet = 3;

	config.model.stop_time = 60;

	config.model.tcp_connect_retry_count = 5;
	config.model.tcp_send_retry_count = 5;
	config.model.tcp_receive_retry_count = 3;
	config.model.tcp_timeout_secs = 30;

	config.model.section_10kms = 0;
	config.model.section_20kms = 160;
	config.model.section_30kms = 250;
	config.model.section_40kms = 330;
	config.model.section_50kms = 410;
	config.model.section_60kms = 500;
	config.model.section_70kms = 580;
	config.model.section_80kms = 660;
	config.model.section_90kms = 750;
	config.model.section_100kms = 830;
	config.model.section_110kms = 0;

	strncpy(config.model.model_name, "cl", sizeof("cl"));
}


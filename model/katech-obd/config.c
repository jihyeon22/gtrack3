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

	strcpy(conf_base->common.sms_passwd, "skeleton");

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
	strncpy(config.model.report_ip, "219.251.4.178", sizeof("219.251.4.178"));
	config.model.report_port = 30004;
	strncpy(config.model.request_ip, "219.251.4.178", sizeof("219.251.4.178"));
	config.model.request_port = 30004;
	config.model.collect_interval_keyon = 10;
	config.model.collect_interval_keyoff = 10;
	config.model.report_interval_keyon = 60;
	config.model.report_interval_keyoff = 60;

	strncpy(config.model.model_name, "skeleton", sizeof("skeleton"));
}

int get_collection_interval()
{
    int interval_time;
    //int ign_on = power_get_ignition_status();
    configurationModel_t *conf = get_config_model();

    //if(ign_on == POWER_IGNITION_OFF)
    //    interval_time = conf->model.collect_interval_keyoff;
    //else
        interval_time = conf->model.collect_interval_keyon;

    printf("get collection interval_time = [%d]\n", interval_time);
    return interval_time;
}

int get_report_interval()
{
    int interval_time;
    //int ign_on = power_get_ignition_status();
    configurationModel_t *conf = get_config_model();

    //if(ign_on == POWER_IGNITION_OFF)
    //    interval_time = conf->model.report_interval_keyoff;
    //else
        interval_time = conf->model.report_interval_keyon;

    // printf("get report interval_time = [%d]\n", interval_time);

    return interval_time;
}


int set_collection_interval(int sec)
{
    configurationModel_t *conf = get_config_model();
	char tmp_save[128] = {0,};
	
	if ( conf->model.collect_interval_keyon == sec )
	{
		LOGD(LOG_TARGET, "Skip SAVE :: collect interval [%d] \n", sec);
		return 0;
	}
	
	LOGD(LOG_TARGET, "New :: set collect interval [%d] \n", sec);
	
	conf->model.collect_interval_keyoff = sec;
    conf->model.collect_interval_keyon = sec;
	// save config file
	
	//printf("set collection interval_time = [%d]\n", conf->model.collect_interval_keyon);
	sprintf(tmp_save,"%d", conf->model.collect_interval_keyon);
	
	if(save_config_user("user:collect_interval_keyon", tmp_save) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : user:collect_interval_keyon\n");
		return -1;
	}
	
	return sec;
}

int set_report_interval(int sec)
{
    configurationModel_t *conf = get_config_model();
	char tmp_save[128] = {0,};
	
	if ( conf->model.report_interval_keyon == sec )
	{
		LOGD(LOG_TARGET, "Skip SAVE :: report interval [%d] \n", sec);
		return 0;
	}
	
	LOGD(LOG_TARGET, "New :: set report interval [%d] \n", sec);
	
	conf->model.report_interval_keyon = sec;
	conf->model.report_interval_keyoff = sec;
	
	// save config file
	//printf("set report interval_time = [%d]\n", conf->model.report_interval_keyon );
	sprintf(tmp_save,"%d", conf->model.report_interval_keyon);
	
	if(save_config_user("user:report_interval_keyon", tmp_save) < 0)
	{
		LOGE(LOG_TARGET, " -- set config fail : user:report_interval_keyon\n");
		return -1;
	}
	
	return sec;
	
}

int get_server_ip(char* buff)
{
	configurationModel_t *conf = get_config_model();
	
	strncpy(buff, conf->model.report_ip, strlen(conf->model.report_ip));
	return 0;
}

int set_server_ip(const char* buff)
{
	configurationModel_t *conf = get_config_model();
	
	int ret = 0;
	
	if ( strcmp(conf->model.report_ip, buff) == 0 )
	{
		LOGI(LOG_TARGET, " -- set config server addr : skip save [%s]\n", buff);
		ret = 0;
	}
	else
	{	
		LOGI(LOG_TARGET, " -- set config server addr : save [%s]\n", buff);
		
		// 바로 적용하지 않는다. 
		// runtime 에서 적용되면 곤란하다 ㅋㅋㅋ
		//strcpy(conf->model.report_ip, buff);
		if(save_config_user("user:report_ip", buff) < 0)
		{
			LOGE(LOG_TARGET, " -- set config fail : user:report_ip\n");
			ret = -1;
		}
	}
	
	return ret;
}

int get_server_port()
{
	configurationModel_t *conf = get_config_model();
	return conf->model.report_port;
}

int set_server_port(int port)
{
	configurationModel_t *conf = get_config_model();
	char tmp_save[128] = {0,};
	
	int ret = 0;
	
	if ( conf->model.report_port == port )
	{
		LOGI(LOG_TARGET, " -- set config server port : save skip [%d]\n", port);
		ret = 0;
	}
	else
	{
		LOGI(LOG_TARGET, " -- set config server port : save [%d]\n", port);
		
		// 바로 적용하지 않는다. 
		// runtime 에서 적용되면 곤란하다 ㅋㅋㅋ
		// conf->model.report_port = port;
		sprintf(tmp_save,"%d", port);
		
		if(save_config_user("user:report_port", tmp_save) < 0)
		{
			LOGE(LOG_TARGET, " -- set config fail : user:report_port\n");
			ret =  -1;
		}
	}

	return ret;
}


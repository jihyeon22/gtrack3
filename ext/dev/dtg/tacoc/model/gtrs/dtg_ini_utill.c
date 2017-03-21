#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <iniparser.h>
#include <dictionary.h>

#include "dtg_ini_utill.h"
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"

#include <wrapper/dtg_log.h>

static dictionary  *g_ini_handle = NULL;

int save_default_init()
{
	//char tmp[50] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_mdt_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_server_port());
	iniparser_set(g_ini_handle, "mdt_server_config:port", tmp);
	iniparser_set(g_ini_handle, "mdt_server_config:server_ip", get_mdt_server_ip_addr());

	iniparser_store(g_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_ini_handle, "dtg_server_config:port", tmp);
	iniparser_set(g_ini_handle, "dtg_server_config:server_ip", get_server_ip_addr());

	iniparser_store(g_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_period_info()
{
	char tmp[50] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_dtg_report_period());
	iniparser_set(g_ini_handle, "dtg_config:report_period", tmp);

	iniparser_store(g_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_mdt_period_info()
{
	char tmp[50] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_report_period());
	iniparser_set(g_ini_handle, "mdt_config:report_period", tmp);

	sprintf(tmp, "%d", get_mdt_create_period());
	iniparser_set(g_ini_handle, "mdt_config:create_period", tmp);

	iniparser_store(g_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++");
	char cmd_copy_file[512];
	
	int num;
	char *str;
	int retry = 0;

    while ((g_ini_handle == NULL) && (retry < 5)) {
		g_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
		if (g_ini_handle == NULL){
			DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
			sleep(2);
			retry++;
		}
    }

	if(g_ini_handle == NULL) 
	{
		sprintf(cmd_copy_file, "cp %s %s", DTG_CONFIG_FILE_PATH_ORG, DTG_CONFIG_FILE_PATH);
		system(cmd_copy_file);
		sleep(1);
		retry = 0;

		 while ((g_ini_handle == NULL) && (retry < 5)) {
			g_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
			if (g_ini_handle == NULL){
				DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
				sleep(2);
				retry++;
			}
		}
	}

	if (g_ini_handle==NULL)
		return -1;

	str = iniparser_getstring(g_ini_handle, "dtg_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_server_ip_addr(str);

	num = iniparser_getint(g_ini_handle, "dtg_server_config:port", -1);
	if(num == -1) return -1;
	set_server_port(num);

	num = iniparser_getint(g_ini_handle, "dtg_config:report_period", -1);
	if(num == -1) return -1;
	set_dtg_report_period(num);

	str = iniparser_getstring(g_ini_handle, "mdt_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_mdt_server_ip_addr(str);

	num = iniparser_getint(g_ini_handle, "mdt_server_config:port", -1);
	if(num == -1) return -1;
	set_mdt_server_port(num);

	num = iniparser_getint(g_ini_handle, "mdt_config:report_period", -1);
	if(num == -1) return -1;
	set_mdt_report_period(num);

	num = iniparser_getint(g_ini_handle, "mdt_config:create_period", -1);
	if(num == -1) return -1;
	set_mdt_create_period(num);

	DTG_LOGD("load_ini_file --");
	return 1;
}

void free_ini_file()
{
	if(g_ini_handle != NULL) 
	{
		iniparser_freedict(g_ini_handle);
		g_ini_handle = NULL;
	}
}

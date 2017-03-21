#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <iniparser.h>
#include <dictionary.h>

#include "dtg_ini_utill.h"
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_ini_utill.h>
#endif

#include <wrapper/dtg_log.h>

static dictionary  *g_ini_handle = NULL;

int save_default_init()
{
	char tmp[50] = {0,};

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_server_setting_info()
{
	char tmp[10] = {0,};
	int flag =1;

    if (g_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_ini_handle, "server_config:port", tmp);
	iniparser_set(g_ini_handle, "server_config:server_ip", get_server_ip_addr());

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

	str = iniparser_getstring(g_ini_handle, "server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_server_ip_addr(str);

	num = iniparser_getint(g_ini_handle, "server_config:port", -1);
	if(num == -1) return -1;
	set_server_port(num);

	num = iniparser_getint(g_ini_handle, "dtg_config:report_period", -1);
	if(num == -1) return -1;
	set_dtg_report_period(num);

#ifdef ENABLE_VOLTAGE_USED_SCINARIO
	num = iniparser_getint(g_ini_handle, "dtg_config:volt_report_key_on_period", -1);
	if(num == -1) return -1;
	set_volt_key_on_report_period(num);
	
	num = iniparser_getint(g_ini_handle, "dtg_config:volt_report_key_off_period", -1);
	if(num == -1) return -1;
	set_volt_key_off_report_period(num);
#endif

	#if defined(BOARD_TL500K)
		if(kt_fota_load_ini_file(KT_FOTA_CONFIG_FILE) < 0)
			return -1;
	#endif

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

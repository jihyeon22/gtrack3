<<<<<<< HEAD
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

static dictionary  *g_dtg_ini_handle = NULL;

int save_default_init()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_ctrl_ip_setting_info()
{
	char tmp[10] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_ctrl_server_port());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:server_ip", get_ctrl_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_ip_setting_info()
{
	char tmp[10] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_dtg_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_dtg_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_ctrl_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_ctrl_report_period());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:report_period", tmp);
	sprintf(tmp, "%d", get_ctrl_create_period());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_dtg_report_period());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:report_period", tmp);
	sprintf(tmp, "%d", get_dtg_create_period());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++");
	
	int num;
	char *str;
	int retry = 0;

    while ((g_dtg_ini_handle == NULL) && (retry < 5)) {
		g_dtg_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
		if (g_dtg_ini_handle == NULL){
			DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
			sleep(2);
			retry++;
		}
    }
	if (g_dtg_ini_handle==NULL)
		return -1;

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_server_config:server_ip", NULL);
	if(str != NULL) set_dtg_server_ip_addr(str);
	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:port", -1);
	set_dtg_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:report_period", -1);
	set_dtg_report_period(num);
	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:create_period", -1);
	set_dtg_create_period(num);

	str = iniparser_getstring(g_dtg_ini_handle, "ctrl_server_config:server_ip", NULL);
	if(str != NULL) set_ctrl_server_ip_addr(str);
	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:port", -1);
	set_ctrl_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:report_period", -1);
	set_ctrl_report_period(num);
	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:create_period", -1);
	set_ctrl_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_reg_server_config:port", -1);
	if(num != -1) set_reg_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_reg_server_config:server_ip", NULL);
	if(str != NULL) set_reg_server_ip_addr(str);

	/* Summary */
	num = iniparser_getint(g_dtg_ini_handle, "dtg_summary_data_config:port", -1);
	set_summary_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_summary_data_config:server_ip", NULL);
	if(str != NULL) set_summary_server_ip_addr(str);

	DTG_LOGD("load_ini_file --");
	return 1;
}
=======
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

static dictionary  *g_dtg_ini_handle = NULL;

int save_default_init()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_ctrl_ip_setting_info()
{
	char tmp[10] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_ctrl_server_port());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:server_ip", get_ctrl_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_ip_setting_info()
{
	char tmp[10] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_dtg_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_dtg_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_ctrl_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_ctrl_report_period());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:report_period", tmp);
	sprintf(tmp, "%d", get_ctrl_create_period());
	iniparser_set(g_dtg_ini_handle, "ctrl_server_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_dtg_report_period());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:report_period", tmp);
	sprintf(tmp, "%d", get_dtg_create_period());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++");
	
	int num;
	char *str;
	int retry = 0;

    while ((g_dtg_ini_handle == NULL) && (retry < 5)) {
		g_dtg_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
		if (g_dtg_ini_handle == NULL){
			DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
			sleep(2);
			retry++;
		}
    }
	if (g_dtg_ini_handle==NULL)
		return -1;

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_server_config:server_ip", NULL);
	if(str != NULL) set_dtg_server_ip_addr(str);
	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:port", -1);
	set_dtg_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:report_period", -1);
	set_dtg_report_period(num);
	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:create_period", -1);
	set_dtg_create_period(num);

	str = iniparser_getstring(g_dtg_ini_handle, "ctrl_server_config:server_ip", NULL);
	if(str != NULL) set_ctrl_server_ip_addr(str);
	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:port", -1);
	set_ctrl_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:report_period", -1);
	set_ctrl_report_period(num);
	num = iniparser_getint(g_dtg_ini_handle, "ctrl_server_config:create_period", -1);
	set_ctrl_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_reg_server_config:port", -1);
	if(num != -1) set_reg_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_reg_server_config:server_ip", NULL);
	if(str != NULL) set_reg_server_ip_addr(str);

	/* Summary */
	num = iniparser_getint(g_dtg_ini_handle, "dtg_summary_data_config:port", -1);
	set_summary_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_summary_data_config:server_ip", NULL);
	if(str != NULL) set_summary_server_ip_addr(str);

	DTG_LOGD("load_ini_file --");
	return 1;
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

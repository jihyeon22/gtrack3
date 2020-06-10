#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <iniparser.h>
#include <dictionary.h>

#include <wrapper/dtg_tools.h>
#include <wrapper/dtg_log.h>
#include "dtg_ini_utill.h"
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_utill.h"

static dictionary  *g_dtg_ini_handle = NULL;

int save_ini_server_ip_and_port()
{
	char tmp[128] = {0};

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:server_ip", get_server_ip_addr());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_packet_sending_period()
{
	char tmp[128] = {0};

	sprintf(tmp, "%d", get_interval()/60);
	iniparser_set(g_dtg_ini_handle, "dtg_device_operation_config:period", tmp);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_err_report_phonenum(char *pn)
{
	iniparser_set(g_dtg_ini_handle, "common:error_report_phonenum", pn);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++\n");
	
	int num;
	char *str;
	
	g_dtg_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load '%s", DTG_CONFIG_FILE_PATH);
        return -1 ;
    }
	
	num = iniparser_getint(g_dtg_ini_handle, "dtg_device_net_config:port", -1);
	set_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_device_net_config:server_ip", NULL);
	if(str != NULL) set_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_device_operation_config:period", -1);
	set_interval(num*60); //�⺻������ ���̴�.
	
	num = iniparser_getint(g_dtg_ini_handle, "dtg_summary_data_config:port", -1);
	set_summary_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_summary_data_config:server_ip", NULL);
	if(str != NULL) set_summary_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_reg_server_config:port", -1);
	if(num != -1) set_reg_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_reg_server_config:server_ip", NULL);
	if(str != NULL) set_reg_server_ip_addr(str);

	/* Common : Error Report */
	str = iniparser_getstring(g_dtg_ini_handle, "common:error_report_phonenum", NULL);
	if (*str == NULL || isphonenum(str))
		set_err_report_phonenum(NULL);
	else
		set_err_report_phonenum(str);
	
	DTG_LOGD("load_ini_file --\n");
	return 1;
}

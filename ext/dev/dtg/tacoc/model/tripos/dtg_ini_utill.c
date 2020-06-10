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
	char tmp[128];
	sprintf(tmp, "rm %s", DTG_CONFIG_FILE_PATH);
	system(tmp);
	sprintf(tmp, "cp %s %s", CONFIG_RECOVERY_FILE_PATH, DTG_CONFIG_FILE_PATH);
	system(tmp);
	
/*
	char tmp[50] = {0,};
	int tmp_val = 0;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", tmp_val);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_tripo_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:tripo_mode", tmp);

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:port", tmp);
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:server_ip", get_server_ip_addr());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:pwd", get_server_pwd());

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);
	sprintf(tmp, "%d", get_psave_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", tmp);
	sprintf(tmp, "%d", get_psave_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_create_period", tmp);

	sprintf(tmp, "%d", get_cumulative_distance());
	iniparser_set(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", get_gpio_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", get_gpio_output());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
*/
}

/*
int save_ini_enable()
{
	char tmp[50] = {0,};
	int flag =1;

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}
*/

int save_ini_report2_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);

	sprintf(tmp, "%d", get_psave_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", tmp);
	sprintf(tmp, "%d", get_psave_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


int save_ini_gpio_output()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", get_gpio_output());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_company_code()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_company_code:company_code", get_company_code());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}


int save_ini_gpio_mode()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", get_gpio_mode());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_cumulative_distance()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_cumulative_distance());
	iniparser_set(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", tmp);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_ip_setting_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:port", tmp);
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:server_ip", get_server_ip_addr());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:pwd", get_server_pwd());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_report1_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


int save_ini_tripo_mode()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_tripo_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:tripo_mode", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


/*
int save_ini_server_ip_and_port()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:server_ip", get_server_ip_addr());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_packet_seding_period()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_interval()/60);
	iniparser_set(g_dtg_ini_handle, "dtg_device_operation_config:period", tmp);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_error_report_phone_number()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_interval()/60);
	iniparser_set(g_dtg_ini_handle, "dtg_device_operation_config:error_report_number", get_error_report_phone_number());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}
*/

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

    num = iniparser_getint(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", -1);
	if(num < 0)		return -1;

    if(num == 0)   	return -1;

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:tripo_mode", -1);
	if(num < 0)		return -1;
	set_tripo_mode(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_ip:server_ip", NULL);
	if(str == NULL)	return - 1;
	set_server_ip_addr(str);
		

	num = iniparser_getint(g_dtg_ini_handle, "tripo_set_ip:port", -1);
	if(num < 0)		return -1;
	set_server_port(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_ip:pwd", NULL);
	if(str == NULL) return -1;
	set_server_pwd(str);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", -1);
	if(num < 0)		return -1;
	set_normal_trans_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:normal_create_period", -1);
	if(num < 0)		return -1;
	set_normal_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", -1);
	if(num < 0)		return -1;
	set_psave_trans_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:psave_create_period", -1);
	if(num < 0)		return -1;
	set_psave_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", -1);
	if(num < 0)		return -1;
	set_cumulative_distance(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", NULL);
	if(str == NULL) return -1;
	set_gpio_mode(str);
	
	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", NULL);
	if(str == NULL) return -1;
	set_gpio_output(str);
	
	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_company_code:company_code", NULL);
	if(str == NULL) return -1;
	set_company_code(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_reg_server_config:port", -1);
	if(num < 0)		return -1;
	set_reg_server_port(num);

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_reg_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_reg_server_ip_addr(str);

	/* Summary */
	num = iniparser_getint(g_dtg_ini_handle, "dtg_summary_data_config:port", -1);
	set_summary_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_summary_data_config:server_ip", NULL);
	if(str != NULL) set_summary_server_ip_addr(str);

	DTG_LOGD("load_ini_file --");

	return 1;
}

int deload_ini_file()
{
	if(g_dtg_ini_handle != NULL) {
		iniparser_freedict(g_dtg_ini_handle);
		g_dtg_ini_handle = NULL;
	}
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
	char tmp[128];
	sprintf(tmp, "rm %s", DTG_CONFIG_FILE_PATH);
	system(tmp);
	sprintf(tmp, "cp %s %s", CONFIG_RECOVERY_FILE_PATH, DTG_CONFIG_FILE_PATH);
	system(tmp);
	
/*
	char tmp[50] = {0,};
	int tmp_val = 0;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", tmp_val);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_tripo_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:tripo_mode", tmp);

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:port", tmp);
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:server_ip", get_server_ip_addr());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:pwd", get_server_pwd());

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);
	sprintf(tmp, "%d", get_psave_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", tmp);
	sprintf(tmp, "%d", get_psave_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_create_period", tmp);

	sprintf(tmp, "%d", get_cumulative_distance());
	iniparser_set(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", get_gpio_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", get_gpio_output());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
*/
}

/*
int save_ini_enable()
{
	char tmp[50] = {0,};
	int flag =1;

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}
*/

int save_ini_report2_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);

	sprintf(tmp, "%d", get_psave_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", tmp);
	sprintf(tmp, "%d", get_psave_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:psave_create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


int save_ini_gpio_output()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", get_gpio_output());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_company_code()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_company_code:company_code", get_company_code());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}


int save_ini_gpio_mode()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	iniparser_set(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", get_gpio_mode());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_cumulative_distance()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_cumulative_distance());
	iniparser_set(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", tmp);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;

	return 0;
}

int save_ini_ip_setting_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:port", tmp);
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:server_ip", get_server_ip_addr());
	iniparser_set(g_dtg_ini_handle, "tripo_set_ip:pwd", get_server_pwd());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_report1_info()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_normal_trans_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", tmp);
	sprintf(tmp, "%d", get_normal_create_period());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:normal_create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


int save_ini_tripo_mode()
{
	char tmp[50] = {0,};
	int flag =1;

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", flag);
	iniparser_set(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", tmp);

	sprintf(tmp, "%d", get_tripo_mode());
	iniparser_set(g_dtg_ini_handle, "tripo_report_period:tripo_mode", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}


/*
int save_ini_server_ip_and_port()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_device_net_config:server_ip", get_server_ip_addr());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_packet_seding_period()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_interval()/60);
	iniparser_set(g_dtg_ini_handle, "dtg_device_operation_config:period", tmp);
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}

int save_ini_error_report_phone_number()
{
	char tmp[128];

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_interval()/60);
	iniparser_set(g_dtg_ini_handle, "dtg_device_operation_config:error_report_number", get_error_report_phone_number());
	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	
	return 0;
}
*/

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

    num = iniparser_getint(g_dtg_ini_handle, "tripo_ini_isload:ini_enable", -1);
	if(num < 0)		return -1;

    if(num == 0)   	return -1;

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:tripo_mode", -1);
	if(num < 0)		return -1;
	set_tripo_mode(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_ip:server_ip", NULL);
	if(str == NULL)	return - 1;
	set_server_ip_addr(str);
		

	num = iniparser_getint(g_dtg_ini_handle, "tripo_set_ip:port", -1);
	if(num < 0)		return -1;
	set_server_port(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_ip:pwd", NULL);
	if(str == NULL) return -1;
	set_server_pwd(str);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:normal_trans_period", -1);
	if(num < 0)		return -1;
	set_normal_trans_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:normal_create_period", -1);
	if(num < 0)		return -1;
	set_normal_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:psave_trans_period", -1);
	if(num < 0)		return -1;
	set_psave_trans_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_report_period:psave_create_period", -1);
	if(num < 0)		return -1;
	set_psave_create_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "tripo_set_cumulative_distance:cumulative_distance", -1);
	if(num < 0)		return -1;
	set_cumulative_distance(num);

	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_gpio:gpio_mode", NULL);
	if(str == NULL) return -1;
	set_gpio_mode(str);
	
	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_gpio:gpio_output", NULL);
	if(str == NULL) return -1;
	set_gpio_output(str);
	
	str = iniparser_getstring(g_dtg_ini_handle, "tripo_set_company_code:company_code", NULL);
	if(str == NULL) return -1;
	set_company_code(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_reg_server_config:port", -1);
	if(num < 0)		return -1;
	set_reg_server_port(num);

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_reg_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_reg_server_ip_addr(str);

	/* Summary */
	num = iniparser_getint(g_dtg_ini_handle, "dtg_summary_data_config:port", -1);
	set_summary_server_port(num);
	str = iniparser_getstring(g_dtg_ini_handle, "dtg_summary_data_config:server_ip", NULL);
	if(str != NULL) set_summary_server_ip_addr(str);

	DTG_LOGD("load_ini_file --");

	return 1;
}

int deload_ini_file()
{
	if(g_dtg_ini_handle != NULL) {
		iniparser_freedict(g_dtg_ini_handle);
		g_dtg_ini_handle = NULL;
	}
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

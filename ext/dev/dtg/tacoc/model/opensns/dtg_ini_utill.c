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
	//char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_mdt_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_server_port());
	iniparser_set(g_dtg_ini_handle, "mdt_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "mdt_server_config:server_ip", get_mdt_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_server_ip_addr());

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
	iniparser_set(g_dtg_ini_handle, "dtg_config:report_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_mdt_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_report_period());
	iniparser_set(g_dtg_ini_handle, "mdt_config:report_period", tmp);

	sprintf(tmp, "%d", get_mdt_create_period());
	iniparser_set(g_dtg_ini_handle, "mdt_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

void check_init_update()
{
	int save_flag = 0;
	char tmp[10];
	dictionary  *ini_handle_new = NULL;
	
	int num_old, num_new;
	char *str;
	int retry = 0;

	while ((ini_handle_new == NULL) && (retry < 5)) {
		ini_handle_new = iniparser_load(DTG_CONFIG_FILE_PATH_ORG);
		if (ini_handle_new == NULL){
			DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH_ORG, __func__);
			sleep(2);
			retry++;
		}
	}
	if(ini_handle_new == NULL)
		return;

	num_old = iniparser_getint(g_dtg_ini_handle, "mdt_server_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "mdt_server_config:ver", -1);
printf("mdt_server_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		str = iniparser_getstring(ini_handle_new, "mdt_server_config:server_ip", NULL);
		if(str != NULL)
			set_mdt_server_ip_addr(str);

		num_new = iniparser_getint(ini_handle_new, "mdt_server_config:port", -1);
		if(num_new != -1)
			set_mdt_server_port(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_server_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:ver", tmp);


		sprintf(tmp, "%d", get_mdt_server_port());
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:port", tmp);
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:server_ip", get_mdt_server_ip_addr());



	}

	num_old = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "dtg_server_config:ver", -1);
printf("dtg_server_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		str = iniparser_getstring(ini_handle_new, "dtg_server_config:server_ip", NULL);
		if(str != NULL)
			set_server_ip_addr(str);

		num_new = iniparser_getint(ini_handle_new, "dtg_server_config:port", -1);
		if(num_new != -1)
			set_server_port(num_new);

		num_new = iniparser_getint(ini_handle_new, "dtg_server_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:ver", tmp);

		sprintf(tmp, "%d", get_server_port());
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_server_ip_addr());
	}

	num_old = iniparser_getint(g_dtg_ini_handle, "mdt_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "mdt_config:ver", -1);
printf("mdt_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		num_new = iniparser_getint(ini_handle_new, "mdt_config:report_period", -1);
		if(num_new != -1)
			set_mdt_report_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_config:create_period", -1);
		if(num_new != -1)
			set_mdt_create_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "mdt_config:ver", tmp);


		sprintf(tmp, "%d", get_mdt_report_period());
		iniparser_set(g_dtg_ini_handle, "mdt_config:report_period", tmp);

		sprintf(tmp, "%d", get_mdt_create_period());
		iniparser_set(g_dtg_ini_handle, "mdt_config:create_period", tmp);

	}

	num_old = iniparser_getint(g_dtg_ini_handle, "dtg_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "dtg_config:ver", -1);
printf("dtg_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		num_new = iniparser_getint(ini_handle_new, "dtg_config:report_period", -1);
		if(num_new != -1)
			set_dtg_report_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "dtg_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "dtg_config:ver", tmp);

		sprintf(tmp, "%d", get_dtg_report_period());
		iniparser_set(g_dtg_ini_handle, "dtg_config:report_period", tmp);
	}

	if(save_flag == 1)
		iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH);

	iniparser_freedict(ini_handle_new);
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++");
	char cmd_copy_file[512];
	
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

	if(g_dtg_ini_handle == NULL) 
	{
		sprintf(cmd_copy_file, "cp %s %s", DTG_CONFIG_FILE_PATH_ORG, DTG_CONFIG_FILE_PATH);
		system(cmd_copy_file);
		sleep(1);
		retry = 0;

		 while ((g_dtg_ini_handle == NULL) && (retry < 5)) {
			g_dtg_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
			if (g_dtg_ini_handle == NULL){
				DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
				sleep(2);
				retry++;
			}
		}
	}
		

	if (g_dtg_ini_handle==NULL) {
		return -1;
	}
	else {
		check_init_update();
	}

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:port", -1);
	if(num == -1) return -1;
	set_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_config:report_period", -1);
	if(num == -1) return -1;
	set_dtg_report_period(num);

	str = iniparser_getstring(g_dtg_ini_handle, "mdt_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_mdt_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_server_config:port", -1);
	if(num == -1) return -1;
	set_mdt_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_config:report_period", -1);
	if(num == -1) return -1;
	set_mdt_report_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_config:create_period", -1);
	if(num == -1) return -1;
	set_mdt_create_period(num);

	DTG_LOGD("load_ini_file --");
	return 1;
}

void free_ini_file()
{
	if(g_dtg_ini_handle != NULL) 
	{
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
	//char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	return 0;
}

int save_ini_mdt_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_server_port());
	iniparser_set(g_dtg_ini_handle, "mdt_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "mdt_server_config:server_ip", get_mdt_server_ip_addr());

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_dtg_server_setting_info()
{
	char tmp[10] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_server_port());
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
	iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_server_ip_addr());

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
	iniparser_set(g_dtg_ini_handle, "dtg_config:report_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

int save_ini_mdt_period_info()
{
	char tmp[50] = {0,};

    if (g_dtg_ini_handle==NULL) {
		DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
        return -1 ;
    }

	sprintf(tmp, "%d", get_mdt_report_period());
	iniparser_set(g_dtg_ini_handle, "mdt_config:report_period", tmp);

	sprintf(tmp, "%d", get_mdt_create_period());
	iniparser_set(g_dtg_ini_handle, "mdt_config:create_period", tmp);

	iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH) ;
	return 0;
}

void check_init_update()
{
	int save_flag = 0;
	char tmp[10];
	dictionary  *ini_handle_new = NULL;
	
	int num_old, num_new;
	char *str;
	int retry = 0;

	while ((ini_handle_new == NULL) && (retry < 5)) {
		ini_handle_new = iniparser_load(DTG_CONFIG_FILE_PATH_ORG);
		if (ini_handle_new == NULL){
			DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH_ORG, __func__);
			sleep(2);
			retry++;
		}
	}
	if(ini_handle_new == NULL)
		return;

	num_old = iniparser_getint(g_dtg_ini_handle, "mdt_server_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "mdt_server_config:ver", -1);
printf("mdt_server_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		str = iniparser_getstring(ini_handle_new, "mdt_server_config:server_ip", NULL);
		if(str != NULL)
			set_mdt_server_ip_addr(str);

		num_new = iniparser_getint(ini_handle_new, "mdt_server_config:port", -1);
		if(num_new != -1)
			set_mdt_server_port(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_server_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:ver", tmp);


		sprintf(tmp, "%d", get_mdt_server_port());
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:port", tmp);
		iniparser_set(g_dtg_ini_handle, "mdt_server_config:server_ip", get_mdt_server_ip_addr());



	}

	num_old = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "dtg_server_config:ver", -1);
printf("dtg_server_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		str = iniparser_getstring(ini_handle_new, "dtg_server_config:server_ip", NULL);
		if(str != NULL)
			set_server_ip_addr(str);

		num_new = iniparser_getint(ini_handle_new, "dtg_server_config:port", -1);
		if(num_new != -1)
			set_server_port(num_new);

		num_new = iniparser_getint(ini_handle_new, "dtg_server_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:ver", tmp);

		sprintf(tmp, "%d", get_server_port());
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:port", tmp);
		iniparser_set(g_dtg_ini_handle, "dtg_server_config:server_ip", get_server_ip_addr());
	}

	num_old = iniparser_getint(g_dtg_ini_handle, "mdt_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "mdt_config:ver", -1);
printf("mdt_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		num_new = iniparser_getint(ini_handle_new, "mdt_config:report_period", -1);
		if(num_new != -1)
			set_mdt_report_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_config:create_period", -1);
		if(num_new != -1)
			set_mdt_create_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "mdt_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "mdt_config:ver", tmp);


		sprintf(tmp, "%d", get_mdt_report_period());
		iniparser_set(g_dtg_ini_handle, "mdt_config:report_period", tmp);

		sprintf(tmp, "%d", get_mdt_create_period());
		iniparser_set(g_dtg_ini_handle, "mdt_config:create_period", tmp);

	}

	num_old = iniparser_getint(g_dtg_ini_handle, "dtg_config:ver", -1);
	num_new = iniparser_getint(ini_handle_new, "dtg_config:ver", -1);
printf("dtg_config:ver [%d/%d]\n", num_old, num_new);
	if( (num_old == -1) || (num_old != num_new))
	{
		save_flag = 1;
		num_new = iniparser_getint(ini_handle_new, "dtg_config:report_period", -1);
		if(num_new != -1)
			set_dtg_report_period(num_new);

		num_new = iniparser_getint(ini_handle_new, "dtg_config:ver", -1);
		sprintf(tmp, "%d", num_new);
		iniparser_set(g_dtg_ini_handle, "dtg_config:ver", tmp);

		sprintf(tmp, "%d", get_dtg_report_period());
		iniparser_set(g_dtg_ini_handle, "dtg_config:report_period", tmp);
	}

	if(save_flag == 1)
		iniparser_store(g_dtg_ini_handle, DTG_CONFIG_FILE_PATH);

	iniparser_freedict(ini_handle_new);
}

int load_ini_file()
{
	DTG_LOGD("load_ini_file ++");
	char cmd_copy_file[512];
	
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

	if(g_dtg_ini_handle == NULL) 
	{
		sprintf(cmd_copy_file, "cp %s %s", DTG_CONFIG_FILE_PATH_ORG, DTG_CONFIG_FILE_PATH);
		system(cmd_copy_file);
		sleep(1);
		retry = 0;

		 while ((g_dtg_ini_handle == NULL) && (retry < 5)) {
			g_dtg_ini_handle = iniparser_load(DTG_CONFIG_FILE_PATH);
			if (g_dtg_ini_handle == NULL){
				DTG_LOGE("Can't load %s at %s", DTG_CONFIG_FILE_PATH, __func__);
				sleep(2);
				retry++;
			}
		}
	}
		

	if (g_dtg_ini_handle==NULL) {
		return -1;
	}
	else {
		check_init_update();
	}

	str = iniparser_getstring(g_dtg_ini_handle, "dtg_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_server_config:port", -1);
	if(num == -1) return -1;
	set_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "dtg_config:report_period", -1);
	if(num == -1) return -1;
	set_dtg_report_period(num);

	str = iniparser_getstring(g_dtg_ini_handle, "mdt_server_config:server_ip", NULL);
	if(str == NULL) return -1;
	set_mdt_server_ip_addr(str);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_server_config:port", -1);
	if(num == -1) return -1;
	set_mdt_server_port(num);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_config:report_period", -1);
	if(num == -1) return -1;
	set_mdt_report_period(num);

	num = iniparser_getint(g_dtg_ini_handle, "mdt_config:create_period", -1);
	if(num == -1) return -1;
	set_mdt_create_period(num);

	DTG_LOGD("load_ini_file --");
	return 1;
}

void free_ini_file()
{
	if(g_dtg_ini_handle != NULL) 
	{
		iniparser_freedict(g_dtg_ini_handle);
		g_dtg_ini_handle = NULL;
	}
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

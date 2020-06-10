<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"

DTG_management g_dtg_config;

#define DTG_NO_ERROR				0
#define DTG_CAN_NOT_DEVICE_LOAD		1
#define DTG_CAN_NOT_SEVER_CONNECT	2
#define DTG_CAN_NOT_SEVER_SEND		3

void lock_mutex()
{
	pthread_mutex_lock(&g_dtg_config.mutex);
}
void unlock_mutex()
{
	pthread_mutex_unlock(&g_dtg_config.mutex);
}

void wait_for_singleobject()
{
	pthread_cond_wait(&g_dtg_config.thread_cond, &g_dtg_config.mutex);
}
void set_signal()
{
	pthread_cond_signal(&g_dtg_config.thread_cond);
}

int default_init()
{
	g_dtg_config.action_working = 0;

	g_dtg_config.dtg_conf.report_period = 180;
	g_dtg_config.server_conf.port = SERVER_PORT;
	strcpy(g_dtg_config.server_conf.server_ip, SERVER_IP);

	g_dtg_config.mdt_conf.report_period = 60;
	g_dtg_config.mdt_conf.create_period = 60;
	g_dtg_config.mdt_svr_conf.port = MDT_SERVER_PORT;
	strcpy(g_dtg_config.mdt_svr_conf.server_ip, MDT_SERVER_IP);

	return 0;
}

int init_configuration_data()
{	
	int ini_retry = 3;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
	memset(&g_dtg_config, 0x00, sizeof(DTG_management));
	pthread_mutex_init(&g_dtg_config.mutex, NULL);
	pthread_cond_init(&g_dtg_config.thread_cond, NULL);

	default_init();
	DTG_LOGD("%s: %s() : ini_retry[%d]", __FILE__, __func__, ini_retry);
	while(ini_retry-- >= 0)
	{
		DTG_LOGD("%s: %s() : load_ini_file call", __FILE__, __func__, ini_retry);
		if(load_ini_file() >= 0)
			break;
		sleep(1);
		free_ini_file();
	}
	if(ini_retry < 0)
		save_default_init();

fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);

#if 1
	DTG_LOGD("SERVER IP=[%s]", get_server_ip_addr());
	DTG_LOGD("SERVER_PORT=[%d]", get_server_port());
	DTG_LOGD("DTG REPORT PERIOD=[%d]sec", get_dtg_report_period());

	DTG_LOGD("MDT_SERVER IP=[%s]", get_mdt_server_ip_addr());
	DTG_LOGD("MDT_SERVER_PORT=[%d]", get_mdt_server_port());
	DTG_LOGD("MDT REPORT PERIOD=[%d]sec", get_mdt_report_period());
	DTG_LOGD("MDT CREATE PERIOD=[%d]sec", get_mdt_create_period());
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);

	return 0;
}

void set_dtg_report_period(s32 num)
{
	g_dtg_config.dtg_conf.report_period = num;
}

s32 get_dtg_report_period()
{
	return g_dtg_config.dtg_conf.report_period;
}

u8 is_working_action()
{
	return g_dtg_config.action_working;
}

void set_working_action(u8 flag)
{
	g_dtg_config.action_working = flag;
}

/*=========================================*/

s8* get_server_ip_addr()
{
	return g_dtg_config.server_conf.server_ip;
}
void set_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.server_conf.server_ip, ip_addr);
}

u16 get_server_port()
{
	return g_dtg_config.server_conf.port;
}
void set_server_port(s32 port)
{
	g_dtg_config.server_conf.port = port;
}

//MDT configuration
s8* get_mdt_server_ip_addr()
{
	return g_dtg_config.mdt_svr_conf.server_ip;
}
void set_mdt_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.mdt_svr_conf.server_ip, ip_addr);
}

u16 get_mdt_server_port()
{
	return g_dtg_config.mdt_svr_conf.port;
}
void set_mdt_server_port(s32 port)
{
	g_dtg_config.mdt_svr_conf.port = port;
}

void set_mdt_report_period(s32 num)
{
	g_dtg_config.mdt_conf.report_period = num;
}
s32 get_mdt_report_period()
{
	return g_dtg_config.mdt_conf.report_period;
}

void set_mdt_create_period(s32 num)
{
	g_dtg_config.mdt_conf.create_period = num;
}
s32 get_mdt_create_period()
{
	return g_dtg_config.mdt_conf.create_period;
}


#if defined(DEVICE_MODEL_INNOCAR) || defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
unsigned short get_k_factor()
{
	return g_dtg_config.K_Factor;
}
unsigned short get_rmp_factor()
{
	return g_dtg_config.RPM_Factor;
}
unsigned short get_weight1()
{
	return g_dtg_config.weight1;
}
unsigned short get_weight2()
{
	return g_dtg_config.weight2;
}

char * get_dtg_version()
{
	if(g_dtg_config.dtg_ver[0] == 0x00)
		return "NONE";

	return g_dtg_config.dtg_ver;
}

void set_factor_value(unsigned short k_facotr, char rmp_factor, char weight1, char weight2, char *dtg_ver)
{
	g_dtg_config.K_Factor = k_facotr;
	g_dtg_config.RPM_Factor = rmp_factor;
	g_dtg_config.weight1 = weight1;
	g_dtg_config.weight2 = weight2;

printf("dtg_ver : [%s]\n", dtg_ver);

	memset(g_dtg_config.dtg_ver, 0x00, sizeof(g_dtg_config.dtg_ver));
	if(dtg_ver == NULL)
		strcpy(g_dtg_config.dtg_ver, "N/A");
	else
		strncpy(g_dtg_config.dtg_ver, dtg_ver, sizeof(g_dtg_config.dtg_ver));

printf("g_dtg_config.dtg_ver : [%s]\n", g_dtg_config.dtg_ver);
}
#endif


static int g_dtg_setting_flag_enable = 0;
int get_innoca_dtg_setting_enable()
{
	return g_dtg_setting_flag_enable;
}

void set_innoca_dtg_setting_enable(int flag)
{
	g_dtg_setting_flag_enable = flag;
}

static int g_dtg_mdt_period_update_enable = 0;
void set_dtg_mdt_period_update_enable(int flag)
{
	g_dtg_mdt_period_update_enable = flag;
}

int get_dtg_mdt_period_update_enable()
{
	return g_dtg_mdt_period_update_enable;
=======
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"

DTG_management g_dtg_config;

#define DTG_NO_ERROR				0
#define DTG_CAN_NOT_DEVICE_LOAD		1
#define DTG_CAN_NOT_SEVER_CONNECT	2
#define DTG_CAN_NOT_SEVER_SEND		3

void lock_mutex()
{
	pthread_mutex_lock(&g_dtg_config.mutex);
}
void unlock_mutex()
{
	pthread_mutex_unlock(&g_dtg_config.mutex);
}

void wait_for_singleobject()
{
	pthread_cond_wait(&g_dtg_config.thread_cond, &g_dtg_config.mutex);
}
void set_signal()
{
	pthread_cond_signal(&g_dtg_config.thread_cond);
}

int default_init()
{
	g_dtg_config.action_working = 0;

	g_dtg_config.dtg_conf.report_period = 180;
	g_dtg_config.server_conf.port = SERVER_PORT;
	strcpy(g_dtg_config.server_conf.server_ip, SERVER_IP);

	g_dtg_config.mdt_conf.report_period = 60;
	g_dtg_config.mdt_conf.create_period = 60;
	g_dtg_config.mdt_svr_conf.port = MDT_SERVER_PORT;
	strcpy(g_dtg_config.mdt_svr_conf.server_ip, MDT_SERVER_IP);

	return 0;
}

int init_configuration_data()
{	
	int ini_retry = 3;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
	memset(&g_dtg_config, 0x00, sizeof(DTG_management));
	pthread_mutex_init(&g_dtg_config.mutex, NULL);
	pthread_cond_init(&g_dtg_config.thread_cond, NULL);

	default_init();
	DTG_LOGD("%s: %s() : ini_retry[%d]", __FILE__, __func__, ini_retry);
	while(ini_retry-- >= 0)
	{
		DTG_LOGD("%s: %s() : load_ini_file call", __FILE__, __func__, ini_retry);
		if(load_ini_file() >= 0)
			break;
		sleep(1);
		free_ini_file();
	}
	if(ini_retry < 0)
		save_default_init();

fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);

#if 1
	DTG_LOGD("SERVER IP=[%s]", get_server_ip_addr());
	DTG_LOGD("SERVER_PORT=[%d]", get_server_port());
	DTG_LOGD("DTG REPORT PERIOD=[%d]sec", get_dtg_report_period());

	DTG_LOGD("MDT_SERVER IP=[%s]", get_mdt_server_ip_addr());
	DTG_LOGD("MDT_SERVER_PORT=[%d]", get_mdt_server_port());
	DTG_LOGD("MDT REPORT PERIOD=[%d]sec", get_mdt_report_period());
	DTG_LOGD("MDT CREATE PERIOD=[%d]sec", get_mdt_create_period());
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);

	return 0;
}

void set_dtg_report_period(s32 num)
{
	g_dtg_config.dtg_conf.report_period = num;
}

s32 get_dtg_report_period()
{
	return g_dtg_config.dtg_conf.report_period;
}

u8 is_working_action()
{
	return g_dtg_config.action_working;
}

void set_working_action(u8 flag)
{
	g_dtg_config.action_working = flag;
}

/*=========================================*/

s8* get_server_ip_addr()
{
	return g_dtg_config.server_conf.server_ip;
}
void set_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.server_conf.server_ip, ip_addr);
}

u16 get_server_port()
{
	return g_dtg_config.server_conf.port;
}
void set_server_port(s32 port)
{
	g_dtg_config.server_conf.port = port;
}

//MDT configuration
s8* get_mdt_server_ip_addr()
{
	return g_dtg_config.mdt_svr_conf.server_ip;
}
void set_mdt_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.mdt_svr_conf.server_ip, ip_addr);
}

u16 get_mdt_server_port()
{
	return g_dtg_config.mdt_svr_conf.port;
}
void set_mdt_server_port(s32 port)
{
	g_dtg_config.mdt_svr_conf.port = port;
}

void set_mdt_report_period(s32 num)
{
	g_dtg_config.mdt_conf.report_period = num;
}
s32 get_mdt_report_period()
{
	return g_dtg_config.mdt_conf.report_period;
}

void set_mdt_create_period(s32 num)
{
	g_dtg_config.mdt_conf.create_period = num;
}
s32 get_mdt_create_period()
{
	return g_dtg_config.mdt_conf.create_period;
}


#if defined(DEVICE_MODEL_INNOCAR) || defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
unsigned short get_k_factor()
{
	return g_dtg_config.K_Factor;
}
unsigned short get_rmp_factor()
{
	return g_dtg_config.RPM_Factor;
}
unsigned short get_weight1()
{
	return g_dtg_config.weight1;
}
unsigned short get_weight2()
{
	return g_dtg_config.weight2;
}

char * get_dtg_version()
{
	if(g_dtg_config.dtg_ver[0] == 0x00)
		return "NONE";

	return g_dtg_config.dtg_ver;
}

void set_factor_value(unsigned short k_facotr, char rmp_factor, char weight1, char weight2, char *dtg_ver)
{
	g_dtg_config.K_Factor = k_facotr;
	g_dtg_config.RPM_Factor = rmp_factor;
	g_dtg_config.weight1 = weight1;
	g_dtg_config.weight2 = weight2;

printf("dtg_ver : [%s]\n", dtg_ver);

	memset(g_dtg_config.dtg_ver, 0x00, sizeof(g_dtg_config.dtg_ver));
	if(dtg_ver == NULL)
		strcpy(g_dtg_config.dtg_ver, "N/A");
	else
		strncpy(g_dtg_config.dtg_ver, dtg_ver, sizeof(g_dtg_config.dtg_ver));

printf("g_dtg_config.dtg_ver : [%s]\n", g_dtg_config.dtg_ver);
}
#endif


static int g_dtg_setting_flag_enable = 0;
int get_innoca_dtg_setting_enable()
{
	return g_dtg_setting_flag_enable;
}

void set_innoca_dtg_setting_enable(int flag)
{
	g_dtg_setting_flag_enable = flag;
}

static int g_dtg_mdt_period_update_enable = 0;
void set_dtg_mdt_period_update_enable(int flag)
{
	g_dtg_mdt_period_update_enable = flag;
}

int get_dtg_mdt_period_update_enable()
{
	return g_dtg_mdt_period_update_enable;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}
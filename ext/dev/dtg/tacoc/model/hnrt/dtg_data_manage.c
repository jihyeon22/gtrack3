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

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_config.h>
#endif

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

	g_dtg_config.dtg_conf.create_period = 1;
	g_dtg_config.dtg_conf.report_period = 180;
	g_dtg_config.mdt_conf.create_period = 600;
	g_dtg_config.mdt_conf.report_period = 600;

	g_dtg_config.maintain_info.trip_num = 0;
	g_dtg_config.maintain_info.cumul_dist = 0;

	g_dtg_config.server_conf.port = SERVER_PORT;
	strcpy(g_dtg_config.server_conf.server_ip, SERVER_IP);

	/* Registration */
	g_dtg_config.reg_conf.port = REG_SERVER_PORT;
	strcpy(g_dtg_config.reg_conf.server_ip, REG_SERVER_IP);

#if defined(BOARD_TL500K)
	kt_fota_default_init(	KT_FOTA_DM_SVC_IP, 
							KT_FOTA_DM_SVC_PORT, 
							KT_FOTA_QTY_SVC_IP, 
							KT_FOTA_QTY_SVC_PORT, 
							ACC_QTY_REPORT, 
							ACC_FOTA_REQ_REPORT);
#endif


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

/*
	if (load_package_file(PACKAGE_FILE) < 0) {
		DTG_LOGE("load_package_file %s error\n", PACKAGE_FILE);
	}
*/
fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);

#if 1
	DTG_LOGD("SERVER IP=[%s]", get_server_ip_addr());
	DTG_LOGD("SERVER_PORT=[%d]", get_server_port());
	DTG_LOGD("MDT REPORT PERIOD=[%d]sec", get_mdt_report_period());
	DTG_LOGD("MDT CREATE PERIOD=[%d]sec", get_mdt_create_period());
	DTG_LOGD("DTG REPORT PERIOD=[%d]sec", get_dtg_report_period());
	DTG_LOGD("DTG CREATE PERIOD=[%d]sec", get_dtg_create_period());
	DTG_LOGD("TRIP NUMBER=[%d]", get_trip_number());
	DTG_LOGD("CUMULATIVE DISTANCE=[%d]m", get_cumulative_dist());
/*
	DTG_LOGD("PACKAGE TYPE=[%d]", get_package_type());
	DTG_LOGD("PACKAGE VERSION=[%d.%d]", get_package_major_version(), get_package_minor_version());
*/
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);

	return 0;
}

void set_dtg_report_period(s32 num)
{
	g_dtg_config.dtg_conf.report_period = num;
}
void set_dtg_create_period(s32 num)
{
	g_dtg_config.dtg_conf.create_period = num;
}
void set_mdt_report_period(s32 num)
{
	g_dtg_config.mdt_conf.report_period = num;
}
void set_mdt_create_period(s32 num)
{
	g_dtg_config.mdt_conf.create_period = num;
}

s32 get_dtg_report_period()
{
	return g_dtg_config.dtg_conf.report_period;
}
s32 get_dtg_create_period()
{
	return g_dtg_config.dtg_conf.create_period;
}
s32 get_mdt_report_period()
{
	return g_dtg_config.mdt_conf.report_period;
}
s32 get_mdt_create_period()
{
	return g_dtg_config.mdt_conf.create_period;
}

void set_trip_number(s32 num) {
	g_dtg_config.maintain_info.trip_num = num;
}

void set_cumulative_dist(s32 num) {
	g_dtg_config.maintain_info.cumul_dist = num;
}

s32 get_trip_number() {
	return g_dtg_config.maintain_info.trip_num;
}

s32 get_cumulative_dist()
{
	return g_dtg_config.maintain_info.cumul_dist;
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


/* Package */
unsigned short get_package_type(void)
{
	return g_dtg_config.package_type;
}

void set_package_type(unsigned short type)
{
	g_dtg_config.package_type = type;
}

unsigned char get_package_major_version()
{
	return g_dtg_config.package_version_major;
}
unsigned char get_package_minor_version()
{
	return g_dtg_config.package_version_minor;
}
void set_package_version(unsigned char major, unsigned char minor)
{
	g_dtg_config.package_version_major = major;
	g_dtg_config.package_version_minor = minor;
}

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

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_config.h>
#endif

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

	g_dtg_config.dtg_conf.create_period = 1;
	g_dtg_config.dtg_conf.report_period = 180;
	g_dtg_config.mdt_conf.create_period = 600;
	g_dtg_config.mdt_conf.report_period = 600;

	g_dtg_config.maintain_info.trip_num = 0;
	g_dtg_config.maintain_info.cumul_dist = 0;

	g_dtg_config.server_conf.port = SERVER_PORT;
	strcpy(g_dtg_config.server_conf.server_ip, SERVER_IP);

	/* Registration */
	g_dtg_config.reg_conf.port = REG_SERVER_PORT;
	strcpy(g_dtg_config.reg_conf.server_ip, REG_SERVER_IP);

#if defined(BOARD_TL500K)
	kt_fota_default_init(	KT_FOTA_DM_SVC_IP, 
							KT_FOTA_DM_SVC_PORT, 
							KT_FOTA_QTY_SVC_IP, 
							KT_FOTA_QTY_SVC_PORT, 
							ACC_QTY_REPORT, 
							ACC_FOTA_REQ_REPORT);
#endif


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

/*
	if (load_package_file(PACKAGE_FILE) < 0) {
		DTG_LOGE("load_package_file %s error\n", PACKAGE_FILE);
	}
*/
fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);

#if 1
	DTG_LOGD("SERVER IP=[%s]", get_server_ip_addr());
	DTG_LOGD("SERVER_PORT=[%d]", get_server_port());
	DTG_LOGD("MDT REPORT PERIOD=[%d]sec", get_mdt_report_period());
	DTG_LOGD("MDT CREATE PERIOD=[%d]sec", get_mdt_create_period());
	DTG_LOGD("DTG REPORT PERIOD=[%d]sec", get_dtg_report_period());
	DTG_LOGD("DTG CREATE PERIOD=[%d]sec", get_dtg_create_period());
	DTG_LOGD("TRIP NUMBER=[%d]", get_trip_number());
	DTG_LOGD("CUMULATIVE DISTANCE=[%d]m", get_cumulative_dist());
/*
	DTG_LOGD("PACKAGE TYPE=[%d]", get_package_type());
	DTG_LOGD("PACKAGE VERSION=[%d.%d]", get_package_major_version(), get_package_minor_version());
*/
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);

	return 0;
}

void set_dtg_report_period(s32 num)
{
	g_dtg_config.dtg_conf.report_period = num;
}
void set_dtg_create_period(s32 num)
{
	g_dtg_config.dtg_conf.create_period = num;
}
void set_mdt_report_period(s32 num)
{
	g_dtg_config.mdt_conf.report_period = num;
}
void set_mdt_create_period(s32 num)
{
	g_dtg_config.mdt_conf.create_period = num;
}

s32 get_dtg_report_period()
{
	return g_dtg_config.dtg_conf.report_period;
}
s32 get_dtg_create_period()
{
	return g_dtg_config.dtg_conf.create_period;
}
s32 get_mdt_report_period()
{
	return g_dtg_config.mdt_conf.report_period;
}
s32 get_mdt_create_period()
{
	return g_dtg_config.mdt_conf.create_period;
}

void set_trip_number(s32 num) {
	g_dtg_config.maintain_info.trip_num = num;
}

void set_cumulative_dist(s32 num) {
	g_dtg_config.maintain_info.cumul_dist = num;
}

s32 get_trip_number() {
	return g_dtg_config.maintain_info.trip_num;
}

s32 get_cumulative_dist()
{
	return g_dtg_config.maintain_info.cumul_dist;
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


/* Package */
unsigned short get_package_type(void)
{
	return g_dtg_config.package_type;
}

void set_package_type(unsigned short type)
{
	g_dtg_config.package_type = type;
}

unsigned char get_package_major_version()
{
	return g_dtg_config.package_version_major;
}
unsigned char get_package_minor_version()
{
	return g_dtg_config.package_version_minor;
}
void set_package_version(unsigned char major, unsigned char minor)
{
	g_dtg_config.package_version_major = major;
	g_dtg_config.package_version_minor = minor;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

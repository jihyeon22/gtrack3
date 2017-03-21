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

	g_dtg_config.dtg_conf.create_period = 1;
	g_dtg_config.dtg_conf.report_period = 180;
	g_dtg_config.ctrl_conf.create_period = 600;
	g_dtg_config.ctrl_conf.report_period = 600;

	g_dtg_config.dtg_conf.port = DTG_SERVER_IP;
	strcpy(g_dtg_config.dtg_conf.server_ip, DTG_SERVER_IP);

	g_dtg_config.ctrl_conf.port = CTRL_SERVER_IP;
	strcpy(g_dtg_config.ctrl_conf.server_ip, CTRL_SERVER_IP);

	/* Registration */
	g_dtg_config.reg_conf.port = REG_SERVER_PORT;
	strcpy(g_dtg_config.reg_conf.server_ip, REG_SERVER_IP);

	/* Summary */
	g_dtg_config.summary_conf.port = SUMMARY_SERVER_PORT;
	strcpy(g_dtg_config.summary_conf.server_ip, SUMMARY_SERVER_IP);

	return 0;
}

int init_configuration_data()
{	
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);

	memset(&g_dtg_config, 0x00, sizeof(DTG_management));
	pthread_mutex_init(&g_dtg_config.mutex, NULL);
	pthread_cond_init(&g_dtg_config.thread_cond, NULL);

	default_init();
	
	if (load_ini_file() < 0)
		save_default_init();

	if (load_package_file(PACKAGE_FILE) < 0) {
		DTG_LOGE("load_package_file %s error\n", PACKAGE_FILE);
	}

#if 1
	DTG_LOGD("DTG_SERVER IP=[%s]", get_dtg_server_ip_addr());
	DTG_LOGD("DTG_SERVER_PORT=[%d]", get_dtg_server_port());
	DTG_LOGD("CONTROL SERVER PORT=[%d]", get_ctrl_server_port());
	DTG_LOGD("CONTROL SERVER IP=[%s]", get_ctrl_server_ip_addr());
	DTG_LOGD("DTG REPORT PERIOD=[%d]sec", get_dtg_report_period());
	DTG_LOGD("DTG CREATE PERIOD=[%d]sec", get_dtg_create_period());
	DTG_LOGD("CONTROL REPORT PERIOD=[%d]sec", get_ctrl_report_period());
	DTG_LOGD("CONTROL CREATE PERIOD=[%d]sec", get_ctrl_create_period());
	DTG_LOGD("REGISTRATION SERVER PORT=[%d]", get_reg_server_port());
	DTG_LOGD("REGISTRATION SERVER IP=[%s]", get_reg_server_ip_addr());
	DTG_LOGD("SUMMARY SERVER PORT=[%d]", get_summary_server_port());
	DTG_LOGD("SUMMARY SERVER IP=[%s]", get_summary_server_ip_addr());
	DTG_LOGD("PACKAGE TYPE=[%d]", get_package_type());
	DTG_LOGD("PACKAGE VERSION=[%d.%d]", get_package_major_version(), get_package_minor_version());
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

void set_dtg_report_period(s32 num)
{
	g_dtg_config.dtg_conf.report_period = num;
}
void set_dtg_create_period(s32 num)
{
	g_dtg_config.dtg_conf.create_period = num;
}
void set_ctrl_report_period(s32 num)
{
	g_dtg_config.ctrl_conf.report_period = num;
}
void set_ctrl_create_period(s32 num)
{
	g_dtg_config.ctrl_conf.create_period = num;
}

s32 get_dtg_report_period()
{
	return g_dtg_config.dtg_conf.report_period;
}
s32 get_dtg_create_period()
{
	return g_dtg_config.dtg_conf.create_period;
}
s32 get_ctrl_report_period()
{
	return g_dtg_config.ctrl_conf.report_period;
}
s32 get_ctrl_create_period()
{
	return g_dtg_config.ctrl_conf.create_period;
}

u8 is_working_action(void)
{
	return g_dtg_config.action_working;
}
void set_working_action(u8 flag)
{
	g_dtg_config.action_working = flag;
}

/*=========================================*/

s8* get_dtg_server_ip_addr()
{
	return g_dtg_config.dtg_conf.server_ip;
}
void set_dtg_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.dtg_conf.server_ip, ip_addr);
}

u16 get_dtg_server_port()
{
	return g_dtg_config.dtg_conf.port;
}
void set_dtg_server_port(s32 port)
{
	g_dtg_config.dtg_conf.port = port;
}

s8* get_ctrl_server_ip_addr()
{
	return g_dtg_config.ctrl_conf.server_ip;
}
void set_ctrl_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.ctrl_conf.server_ip, ip_addr);
}

u16 get_ctrl_server_port()
{
	return g_dtg_config.ctrl_conf.port;
}
void set_ctrl_server_port(s32 port)
{
	g_dtg_config.ctrl_conf.port = port;
}

s8* get_reg_server_ip_addr()
{
	return g_dtg_config.reg_conf.server_ip;
}
void set_reg_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.reg_conf.server_ip, ip_addr);
}

u16 get_reg_server_port()
{
	return g_dtg_config.reg_conf.port;
}
void set_reg_server_port(u16 port)
{
	g_dtg_config.reg_conf.port = port;
}

s8* get_summary_server_ip_addr()
{
	return g_dtg_config.summary_conf.server_ip;
}
void set_summary_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.summary_conf.server_ip, ip_addr);
}

u16 get_summary_server_port()
{
	return g_dtg_config.summary_conf.port;
}
void set_summary_server_port(u16 port)
{
	g_dtg_config.summary_conf.port = port;
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


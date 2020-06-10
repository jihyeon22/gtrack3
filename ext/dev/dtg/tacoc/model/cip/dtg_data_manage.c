#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <wrapper/dtg_log.h>

#include <package.h>
#include "dtg_type.h"
#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"

DTG_management g_dtg_config;

int init_configuration_data()
{
	char szCar_Num[30];
	//DEBUG_FUNC_TRACE("%s ++\n", __func__);
	
	memset(&g_dtg_config, 0x00, sizeof(DTG_management));
	pthread_mutex_init(&g_dtg_config.mutex, NULL);
	pthread_cond_init(&g_dtg_config.thread_cond, NULL);

	g_dtg_config.action_working = 0;

	if(load_ini_file() < 0) {
		g_dtg_config.interval_get_data_from_dtg = DEFAULT_INTERVAL;
		strcpy(g_dtg_config.net_conf.server_ip, SERVER_IP);
		g_dtg_config.net_conf.port = SERVER_PORT;
	}

	if (load_package_file(PACKAGE_FILE) < 0) {
		DTG_LOGE("load_package_file %s error", PACKAGE_FILE);
	}

	strcpy(g_dtg_config.action_working_caller, "None");
	//Netwokr Configuration
	DTG_LOGT("PACKAGE TYPE    =  [%d]", get_package_type());
	DTG_LOGT("PACKAGE VERSION = [%d.%02d]", get_package_major_version(), get_package_minor_version());
	DTG_LOGT("SERVER_PORT = [%d]", get_server_port());
	DTG_LOGT("SERVER IP   = [%s]", get_server_ip_addr());
	DTG_LOGT("PERIOD TO GET DATA FROM DTG = [%d]sec", get_interval());
	DTG_LOGT("REGISTRATION PORT = [%d]", get_reg_server_port());
	DTG_LOGT("REGISTRATION IP   = [%s]", get_reg_server_ip_addr());
	DTG_LOGT("SUMMARY PORT = [%d]", get_summary_server_port());
	DTG_LOGT("SUMMARY IP   = [%s]", get_summary_server_ip_addr());
	DTG_LOGT("ERROR REPORT PHONE NUMBER = [%s]\n",
			get_err_report_phonenum() ? get_err_report_phonenum() : "None");
}

/* Control Server */
s8* get_server_ip_addr()
{
	return g_dtg_config.net_conf.server_ip;
}
void set_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.net_conf.server_ip, ip_addr);
}

u16 get_server_port()
{
	return g_dtg_config.net_conf.port;
}
void set_server_port(u16 port)
{
	g_dtg_config.net_conf.port = port;
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

/* Ucar Summary Server */
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

/* Control */
s32 get_interval()
{
	return g_dtg_config.interval_get_data_from_dtg;
}
void set_interval(s32 sec)
{
	g_dtg_config.interval_get_data_from_dtg = sec;
}

u8 is_working_action()
{
	return g_dtg_config.action_working;
}

char* working_action_caller()
{
	return g_dtg_config.action_working_caller;
}
int  working_action_caller_line() 
{
	return g_dtg_config.action_working_line;
}
void set_working_action(u8 flag, char *func, int line)
{
	g_dtg_config.action_working = flag;

	if(func != NULL)
		strcpy(g_dtg_config.action_working_caller, func);
	g_dtg_config.action_working_line = line;	
}

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

/* Error Report */
void set_err_report_phonenum(const char *phonenum)
{
	if (g_dtg_config.err_report_phonenum != NULL) 
		free(g_dtg_config.err_report_phonenum);

	if (phonenum == NULL) {
		g_dtg_config.err_report_phonenum = NULL;
	} else {
		g_dtg_config.err_report_phonenum = strdup(phonenum);
	}
}

char *get_err_report_phonenum()
{
	return g_dtg_config.err_report_phonenum;
}

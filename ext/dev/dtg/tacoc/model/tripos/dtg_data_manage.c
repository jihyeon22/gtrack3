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

int default_init()
{
	g_dtg_config.serial_port_mode = UT1_2_MODE;
	//g_dtg_config.serial_port_mode = UT_DEFAULT_MODE;
	g_dtg_config.ps_mode_enable = 0;
	memset(g_dtg_config.company_code, 0, 10);
	memcpy(g_dtg_config.company_code, "10000", 5);
	set_current_ctime();
	g_dtg_config.pw_status = 0;
	g_dtg_config.action_working = 0;

	g_dtg_config.report_conf.total_ncreate_period = 0;
	g_dtg_config.report_conf.total_pcreate_period = 0;

	g_dtg_config.report_conf.normal_create_period = 60;
	g_dtg_config.report_conf.normal_trans_period = 60;
	g_dtg_config.report_conf.psave_create_period = 60;
	g_dtg_config.report_conf.psave_trans_period = 60;
	strcpy(g_dtg_config.net_conf.server_ip, SERVER_IP);
	strcpy(g_dtg_config.net_conf.pwd, SERVER_PWD);
	g_dtg_config.net_conf.port = SERVER_PORT;
	g_dtg_config.cumulative_dist = 0;
	strcpy(g_dtg_config.gpio_conf.mode, "000000");
	strcpy(g_dtg_config.gpio_conf.ouput, "XXXXXX");

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
	int i;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);

	memset(&g_dtg_config, 0x00, sizeof(DTG_management));
	pthread_mutex_init(&g_dtg_config.mutex, NULL);
	pthread_cond_init(&g_dtg_config.thread_cond, NULL);
	reset_report_msg_info();

	default_init();
	
	//jwrho ++
	for(i = 0; i < 3; i++) {
		if(load_ini_file() > 0) {
			DTG_LOGD("%s load success!!", DTG_CONFIG_FILE_PATH);
			break;
		}
		sleep(1);
	}
	if (i == 3)	{
		DTG_LOGD("%s load fail", DTG_CONFIG_FILE_PATH);
		save_default_init();
	}
	//jwrho --

	if (load_package_file(PACKAGE_FILE) < 0) {
		DTG_LOGE("load_package_file %s error\n", PACKAGE_FILE);
	}

#if 1
	DTG_LOGD("SERVER IP=[%s]", get_server_ip_addr());
	DTG_LOGD("SERVER_PORT=[%d]", get_server_port());
	DTG_LOGD("SERVER PWD : %s", get_server_pwd());
	DTG_LOGD("PERIOD1=[%d]sec", get_normal_trans_period());
	DTG_LOGD("PERIOD2=[%d]sec", get_normal_create_period());
	DTG_LOGD("PERIOD3=[%d]sec", get_psave_trans_period());
	DTG_LOGD("PERIOD4=[%d]sec", get_psave_create_period());
	DTG_LOGD("cumulative distance=%d", get_cumulative_distance());
	DTG_LOGD("gpio_mode=%s", get_gpio_mode());
	DTG_LOGD("gpio_output=%s", get_gpio_output());
	DTG_LOGD("REGISTRATION SERVER PORT=[%d]", get_reg_server_port());
	DTG_LOGD("REGISTRATION SERVER IP=[%s]", get_reg_server_ip_addr());
	DTG_LOGD("SUMMARY SERVER PORT=[%d]", get_summary_server_port());
	DTG_LOGD("SUMMARY SERVER IP=[%s]", get_summary_server_ip_addr());
	DTG_LOGD("PACKAGE TYPE=[%d]", get_package_type());
	DTG_LOGD("PACKAGE VERSION=[%d.%d]", get_package_major_version(), get_package_minor_version());
#endif

	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

void set_pw_status(int arg)
{
	g_dtg_config.pw_status = arg;
}

s32 get_pw_status()
{
	return g_dtg_config.pw_status;
}


s8* get_company_code()
{
	return g_dtg_config.company_code;
}

void set_company_code(s8* company_code)
{
	memset(g_dtg_config.company_code, 0, 10);
	memcpy(g_dtg_config.company_code, company_code, 5);
}

void set_current_ctime()
{
	time_t     current_time;
	struct tm *struct_time;
	current_time = time(NULL);
	struct_time = localtime( &current_time);

	g_dtg_config.ctime_conf.year = struct_time->tm_year+1900;
	g_dtg_config.ctime_conf.mon = struct_time->tm_mon+1;
	g_dtg_config.ctime_conf.day = struct_time->tm_mday;
	g_dtg_config.ctime_conf.hour = struct_time->tm_hour;
	g_dtg_config.ctime_conf.min = struct_time->tm_min;
	g_dtg_config.ctime_conf.sec = struct_time->tm_sec;
}

u16 get_ctime_year()
{
	return g_dtg_config.ctime_conf.year;
}

s32 get_ctime_mon()
{
	return g_dtg_config.ctime_conf.mon;
}

s32 get_ctime_day()
{
	return g_dtg_config.ctime_conf.day;
}

s32 get_ctime_hour()
{
	return g_dtg_config.ctime_conf.hour;
}

s32 get_ctime_min()
{
	return g_dtg_config.ctime_conf.min;
}

s32 get_ctime_sec()
{
	return g_dtg_config.ctime_conf.sec;
}

s32* get_report_msg_count()
{
	return &(g_dtg_config.status_msg_conf.msg_count);
}

s32* get_report_msg_len()
{
	return g_dtg_config.status_msg_conf.msg_len;
}

s8 (*get_report_msg())[MAX_REPORT_MSG_LEN]
{
	return g_dtg_config.status_msg_conf.report_msg;
}

void reset_report_msg_info()
{	
	g_dtg_config.status_msg_conf.msg_count = 0;
	memset(g_dtg_config.status_msg_conf.msg_len, 0, sizeof(g_dtg_config.status_msg_conf.msg_len));	
	memset(g_dtg_config.status_msg_conf.report_msg, 0, sizeof(g_dtg_config.status_msg_conf.report_msg));
}

void set_serial_port_mode(int mode)
{
	g_dtg_config.serial_port_mode = mode;
}

s32 get_serial_port_mode()
{
	return g_dtg_config.serial_port_mode;
}

void reset_total_ncreate_period()
{
	g_dtg_config.report_conf.total_ncreate_period =0;
}

void reset_total_pcreate_period()
{
	g_dtg_config.report_conf.total_pcreate_period =0;
}

void reset_total_create_period()
{
	if(is_ps_mode())
		reset_total_pcreate_period();
	else
		reset_total_ncreate_period();
}

void update_total_ncreate_period()
{
	if(g_dtg_config.report_conf.total_ncreate_period < get_normal_trans_period())
		g_dtg_config.report_conf.total_ncreate_period += get_normal_create_period();
	else
		g_dtg_config.report_conf.total_ncreate_period = get_normal_trans_period();
}

void update_total_pcreate_period()
{
	if(g_dtg_config.report_conf.total_pcreate_period < get_psave_trans_period())
		g_dtg_config.report_conf.total_pcreate_period += get_psave_create_period();
	else
		g_dtg_config.report_conf.total_pcreate_period = get_psave_trans_period();
}

void update_total_create_period()
{
	if(is_ps_mode())
		update_total_pcreate_period();
	else
		update_total_ncreate_period();
}

s32 get_total_create_period()
{
	if(is_ps_mode())
		return get_total_pcreate_period();
	else
		return get_total_ncreate_period();
}

s32 get_total_ncreate_period()
{
	return g_dtg_config.report_conf.total_ncreate_period;
}

s32 get_total_pcreate_period()
{
	return g_dtg_config.report_conf.total_pcreate_period;
}

void set_ps_mode(int val)
{
	g_dtg_config.ps_mode_enable = val;
}

int is_ps_mode()
{
	return g_dtg_config.ps_mode_enable;
}

void set_gpio_mode(s8* str)
{
	strcpy(g_dtg_config.gpio_conf.mode, str);
}
void set_gpio_output(s8* str)
{
	strcpy(g_dtg_config.gpio_conf.ouput, str);
}

s8* get_gpio_mode()
{
	return g_dtg_config.gpio_conf.mode;
}
s8* get_gpio_output()
{
	return g_dtg_config.gpio_conf.ouput;
}

s8* get_dev_phone_number() 
{
	return g_dtg_config.phone_num;
}

void set_dev_phone_number(s8* num)
{
	strcpy(g_dtg_config.phone_num, num);
}

s8* get_server_ip_addr()
{
	return g_dtg_config.net_conf.server_ip;
}
void set_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.net_conf.server_ip, ip_addr);
}

s8* get_server_pwd()
{
	return g_dtg_config.net_conf.pwd;
}
void set_server_pwd(s8* pwd)
{
	strcpy(g_dtg_config.net_conf.pwd, pwd);
}

void set_normal_trans_period(s32 num)
{
	g_dtg_config.report_conf.normal_trans_period = num;
}
void set_normal_create_period(s32 num)
{
	g_dtg_config.report_conf.normal_create_period = num;
}
void set_psave_trans_period(s32 num)
{
	g_dtg_config.report_conf.psave_trans_period = num;
}
void set_psave_create_period(s32 num)
{
	g_dtg_config.report_conf.psave_create_period = num;
}

s32 get_trans_period()
{
	if(is_ps_mode())
		return get_psave_trans_period();
	else
		return get_normal_trans_period();
}

s32 get_create_period()
{
	if(is_ps_mode())
		return get_psave_create_period();
	else
		return get_normal_create_period();
}

void set_cumulative_distance(s32 num)
{
	g_dtg_config.cumulative_dist = num;
}

s32 get_cumulative_distance()
{
	return g_dtg_config.cumulative_dist;
}

s32 get_normal_trans_period()
{
	return g_dtg_config.report_conf.normal_trans_period;
}
s32 get_normal_create_period()
{
	return g_dtg_config.report_conf.normal_create_period;
}
s32 get_psave_trans_period()
{
	return g_dtg_config.report_conf.psave_trans_period;
}
s32 get_psave_create_period()
{
	return g_dtg_config.report_conf.psave_create_period;
}

u16 get_server_port()
{
	return g_dtg_config.net_conf.port;
}
void set_server_port(s32 port)
{
	g_dtg_config.net_conf.port = port;
}

void set_tripo_mode(s32 mode)
{
	g_dtg_config.serial_port_mode = mode;
}

s32 get_tripo_mode()
{
	return g_dtg_config.serial_port_mode;
}

u8 is_working_action()
{
	return g_dtg_config.action_working;
}
void set_working_action(u8 flag)
{
	g_dtg_config.action_working = flag;
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

//////////////////////////////

#if 0

int get_version()
{
	return DTG_CLIENT_SW_VERSION;
}


s32 get_interval()
{
	return g_dtg_config.interval_get_data_from_dtg;
}
void set_interval(s32 sec)
{
	g_dtg_config.interval_get_data_from_dtg = sec;
}

void set_car_number(char *car_num)
{
	if(car_num != NULL) {
		strcpy(g_dtg_config.car_number, car_num);
	}
	else
		g_dtg_config.car_number[0] = 0x00;
}
char *get_car_number()
{
	if(g_dtg_config.car_number[0] == 0x00)
		return NULL;

	return g_dtg_config.car_number;
}

s8* get_update_server_ip_addr()
{
	return g_dtg_config.update_conf.server_ip;
}
void set_update_server_ip_addr(s8* ip_addr)
{
	strcpy(g_dtg_config.update_conf.server_ip, ip_addr);
}
#endif

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


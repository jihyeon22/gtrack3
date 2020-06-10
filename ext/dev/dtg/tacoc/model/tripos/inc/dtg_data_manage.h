#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#if (1)
	//#Logis All
	#define SERVER_IP				"1.209.94.192"
	#define SERVER_PORT				9269
#elif (0) 
	//# S-food
	#define SERVER_IP				"115.68.25.236"
	#define SERVER_PORT				15081
#else
	//Default #Logis All
	#define SERVER_IP				"1.209.94.192"
	#define SERVER_PORT				9269
#endif

#define SERVER_PWD				"mdt800ip"
#define DTG_CONFIG_FILE_PATH		"/system/mds/system/bin/tripos.ini"
#define CONFIG_RECOVERY_FILE_PATH		"/system/mds/system/bin/tripos_initial.ini"

//SUMMARY SERVER CONFIG
#define REG_SERVER_IP			"m2m.mdstec.com"
#define REG_SERVER_PORT			30007

//SUMMARY SERVER CONFIG
#define SUMMARY_SERVER_IP		"210.16.217.7"
#define SUMMARY_SERVER_PORT		5833

#pragma pack(push, 1)
struct msg_mdt
{
	unsigned char protocol_id;
	unsigned char message_id;
	char terminal_id[15];
	unsigned char event_code;
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char position_type;
	signed long gps_x;
	signed long gps_y;
	unsigned char azimuth;
	unsigned short speed;
	unsigned long accumulative_distance;
	signed short temperature_a;
	signed short temperature_b;
	signed short temperature_c;
	unsigned short report_period;		//set on tacoc
	unsigned char gpio_input;
	unsigned char power_type;
	unsigned short create_period;		//set on tacoc
}__attribute__((packed));
typedef struct msg_mdt msg_mdt_t;
#pragma pack(pop)

#define RECORD_HDR_LEN	79
#define RECORD_DATA_LEN	103
#define MAX_RECORD_NUM 255
#define MAX_RECORD_DATA_LEN MAX_RECORD_NUM*RECORD_DATA_LEN

typedef struct
{
	char server_ip[64];
	char pwd[10];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	int normal_trans_period;
	int normal_create_period;
	int total_ncreate_period;
	int psave_trans_period;
	int psave_create_period;
	int total_pcreate_period;
}DTG_REPORT_CONF;


typedef struct
{
	unsigned short year;
	int mon;
	int day;
	int hour;
	int min;
	int sec;
}DTG_CTIME_CONF;


typedef struct
{
	char mode[10];
	char ouput[10];
}DTG_GPIO_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_SUMMARY_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_REG_CONF;

#define MAX_REPORT_CONT 20
#define MAX_REPORT1_LEN 53
#define MAX_REPORT2_LEN 51
#define MAX_REPORT_MSG_LEN 300

typedef struct
{
	int msg_count;
	int msg_len[MAX_REPORT_CONT];
	char report_msg[MAX_REPORT_CONT][MAX_REPORT_MSG_LEN];

	/*
	int report1_count;
	int report2_count;
	char report1_msg[MAX_REPORT_CONT][MAX_REPORT1_LEN];
	char report2_msg[MAX_REPORT_CONT][MAX_REPORT2_LEN];
	*/
}STATUS_MSG_INFO;

#define	UT_DEFAULT_MODE	0 // default mode
#define UT1_2_MODE		1 // UT1_2 mode
#define TEMP_UT3_MODE	2 // UT1_2 mode

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;

	//s32 ini_using;
	s32 serial_port_mode;
	int ps_mode_enable;
	int pw_status;

	DTG_NETWORK_CONF net_conf;
	DTG_REPORT_CONF report_conf;
	s32 cumulative_dist;
	DTG_GPIO_CONF gpio_conf;
	STATUS_MSG_INFO status_msg_conf;
	DTG_CTIME_CONF ctime_conf;
	DTG_REG_CONF reg_conf;
	DTG_SUMMARY_CONF summary_conf;
	s8 phone_num[15];
	s8 company_code[10];
	u8 action_working;
	
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;
}DTG_management;


void set_current_ctime();
s32* get_report_msg_count();
s8 (*get_report_msg())[MAX_REPORT_MSG_LEN];

void reset_report_msg_info();

s32 get_serial_port_mode();
s8* get_gpio_mode();
void set_gpio_mode(s8* str);
s8* get_gpio_output();
void set_gpio_output(s8* str);

s8* get_dev_phone_number();
void set_dev_phone_number(s8* num);
s8* get_server_ip_addr();
void set_server_ip_addr(s8* ip_addr);
s8* get_server_pwd();
void set_server_pwd(s8* pwd);
s32 get_normal_trans_period();
void set_normal_trans_period(s32 num);
s32 get_normal_create_period();
void set_normal_create_period(s32 num);
s32 get_psave_trans_period();
void set_psave_trans_period(s32 num);
s32 get_psave_create_period();
void set_psave_create_period(s32 num);
s32 get_cumulative_distance();
void set_cumulative_distance(s32 num);
u16 get_server_port();
void set_server_port(s32 port);
s32 get_interval();
void set_interval(s32 sec);
u8 is_working_action();
void set_working_action(u8 flag);

void set_signal();
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();

int init_configuration_data();

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

s8* get_summary_server_ip_addr();
void set_summary_server_ip_addr(s8* ip_addr);
u16 get_summary_server_port();
void set_summary_server_port(u16 port);

/* Package */
unsigned short get_package_type();
void set_package_type(unsigned short package_type);
unsigned char get_package_major_version();
unsigned char get_package_minor_version();
void set_package_version(unsigned char major, unsigned char minor);

#endif

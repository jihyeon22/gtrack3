<<<<<<< HEAD
#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#define DEFAULT_INTERVAL        3*60//3 min
#define RETRY_INTERVAL			10	//10 sec

#define SERVER_IP				"m2m.mdstec.com"
#define SERVER_PORT				30007

#define DTG_CONFIG_FILE_PATH			"/system/mds/system/bin/cip.ini"

typedef struct {
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

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

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	s32 interval_get_data_from_dtg; //default 3 min
	u8 action_working;
	char action_working_caller[128];
	char action_working_line;
	
	/* package feature */
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;

	/* server feature */
	DTG_NETWORK_CONF net_conf;
	DTG_SUMMARY_CONF summary_conf;
	DTG_REG_CONF reg_conf;

	/* debug & error feature */
	char *err_report_phonenum;
}DTG_management;

s8* get_dev_phone_number();
void set_dev_phone_number(s8* num);

/* Control Server */
s8* get_server_ip_addr();
void set_server_ip_addr(s8* ip_addr);
u16 get_server_port();
void set_server_port(u16 port);

/* Control */
s32 get_interval();
void set_interval(s32 sec);

/* Summary Server */
s8* get_summary_server_ip_addr();
void set_summary_server_ip_addr(s8* ip_addr);
u16 get_summary_server_port();
void set_summary_server_port(u16 port);

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

u8 is_working_action();
void set_working_action(u8 flag, char *func, int line);
char* working_action_caller();
int  working_action_caller_line();
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();
void set_signal();

int init_configuration_data();

int get_report_status_dtg_breakdown();
void set_report_status_dtg_breakdown(int onoff);

/* Package */
unsigned short get_package_type();
void set_package_type(unsigned short package_type);
unsigned char get_package_major_version();
unsigned char get_package_minor_version();
void set_package_version(unsigned char major, unsigned char minor);

/* Common: Error Reporting */
void set_err_report_phonenum(const char *phonenum);
char* get_err_report_phonenum();
#endif
=======
#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#define DEFAULT_INTERVAL        3*60//3 min
#define RETRY_INTERVAL			10	//10 sec

#define SERVER_IP				"m2m.mdstec.com"
#define SERVER_PORT				30007

#define DTG_CONFIG_FILE_PATH			"/system/mds/system/bin/cip.ini"

typedef struct {
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

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

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	s32 interval_get_data_from_dtg; //default 3 min
	u8 action_working;
	char action_working_caller[128];
	char action_working_line;
	
	/* package feature */
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;

	/* server feature */
	DTG_NETWORK_CONF net_conf;
	DTG_SUMMARY_CONF summary_conf;
	DTG_REG_CONF reg_conf;

	/* debug & error feature */
	char *err_report_phonenum;
}DTG_management;

s8* get_dev_phone_number();
void set_dev_phone_number(s8* num);

/* Control Server */
s8* get_server_ip_addr();
void set_server_ip_addr(s8* ip_addr);
u16 get_server_port();
void set_server_port(u16 port);

/* Control */
s32 get_interval();
void set_interval(s32 sec);

/* Summary Server */
s8* get_summary_server_ip_addr();
void set_summary_server_ip_addr(s8* ip_addr);
u16 get_summary_server_port();
void set_summary_server_port(u16 port);

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

u8 is_working_action();
void set_working_action(u8 flag, char *func, int line);
char* working_action_caller();
int  working_action_caller_line();
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();
void set_signal();

int init_configuration_data();

int get_report_status_dtg_breakdown();
void set_report_status_dtg_breakdown(int onoff);

/* Package */
unsigned short get_package_type();
void set_package_type(unsigned short package_type);
unsigned char get_package_major_version();
unsigned char get_package_minor_version();
void set_package_version(unsigned char major, unsigned char minor);

/* Common: Error Reporting */
void set_err_report_phonenum(const char *phonenum);
char* get_err_report_phonenum();
#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

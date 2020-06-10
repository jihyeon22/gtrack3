<<<<<<< HEAD
#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#pragma pack(push, 1)
typedef struct {
	char stx;				//fixed : '['
	char command[3];		//fixed : 'MPR'
	char unit_id[13];		//##01089619571" -> phone number
	char length[3];			//packet total length : '000' ~ '999'
	char data_count[2];		//fixed : '01'로 고정
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_HEAD;

typedef struct {
	char date[6];			//'yymmdd'
	char time[6];			//'hhmmss'
	char gps_status;		//fixed : 'A'
	char gps_latitude[9];	//bypass
	char gps_longitude[9];	//bypass
	char speed[3];			//'000' ~ '255'
	char direction[3];		//방위각
	char altitude[4];
	char accumu_dist[10];
	char acc_status;		//fixed : '1'
	char battery_status;	//fixed : '0'
	char area_no;			//fixed : '0'
	char event_code[2];
	char ext_inter_val[30];
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_DATA;

typedef struct {
	char check_sum;
	char ext;				//fixed : ']'
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_TAIL;

#pragma pack(pop)

#define DEFAULT_INTERVAL        3*60//3 min
#define RETRY_INTERVAL			10	//10 sec

#define SERVER_IP				"m2m.mdstec.com"
#define SERVER_PORT				30007

#define DTG_CONFIG_FILE_PATH			"/system/mds/system/bin/cloud.ini"

typedef struct {
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_REG_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_SUMMARY_CONF;

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	s32 interval_get_data_from_dtg; //default 3 min
	u8 action_working;
	
	/* package feature */
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;

	/* server feature */
	DTG_NETWORK_CONF net_conf;
	DTG_REG_CONF reg_conf;
	DTG_SUMMARY_CONF summary_conf;
	
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

u8 is_working_action();
void set_working_action(u8 flag);
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();
void set_signal();

int init_configuration_data();

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

void set_car_number(char *car_num);
char *get_car_number();

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

#pragma pack(push, 1)
typedef struct {
	char stx;				//fixed : '['
	char command[3];		//fixed : 'MPR'
	char unit_id[13];		//##01089619571" -> phone number
	char length[3];			//packet total length : '000' ~ '999'
	char data_count[2];		//fixed : '01'로 고정
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_HEAD;

typedef struct {
	char date[6];			//'yymmdd'
	char time[6];			//'hhmmss'
	char gps_status;		//fixed : 'A'
	char gps_latitude[9];	//bypass
	char gps_longitude[9];	//bypass
	char speed[3];			//'000' ~ '255'
	char direction[3];		//방위각
	char altitude[4];
	char accumu_dist[10];
	char acc_status;		//fixed : '1'
	char battery_status;	//fixed : '0'
	char area_no;			//fixed : '0'
	char event_code[2];
	char ext_inter_val[30];
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_DATA;

typedef struct {
	char check_sum;
	char ext;				//fixed : ']'
}__attribute__((packed))CLOUD_SOFT_SERVER_PACKET_TAIL;

#pragma pack(pop)

#define DEFAULT_INTERVAL        3*60//3 min
#define RETRY_INTERVAL			10	//10 sec

#define SERVER_IP				"m2m.mdstec.com"
#define SERVER_PORT				30007

#define DTG_CONFIG_FILE_PATH			"/system/mds/system/bin/cloud.ini"

typedef struct {
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_REG_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}DTG_SUMMARY_CONF;

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	s32 interval_get_data_from_dtg; //default 3 min
	u8 action_working;
	
	/* package feature */
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;

	/* server feature */
	DTG_NETWORK_CONF net_conf;
	DTG_REG_CONF reg_conf;
	DTG_SUMMARY_CONF summary_conf;
	
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

u8 is_working_action();
void set_working_action(u8 flag);
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();
void set_signal();

int init_configuration_data();

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

void set_car_number(char *car_num);
char *get_car_number();

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

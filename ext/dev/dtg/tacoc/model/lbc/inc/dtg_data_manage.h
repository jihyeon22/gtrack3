#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#pragma pack(push, 1)
struct tacom_lbc_hdr {
	char vehicle_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char registration_num[12];
	char business_license_num[10];
	char driver_code[18];
//	char modem_num[11];
}__attribute__((packed));
typedef struct tacom_lbc_hdr tacom_lbc_hdr_t;

struct tacom_lbc_data {
	char day_run_distance[4];
	char acumulative_run_distance[7];
	char date_time[14];
	char speed[3];
	char rpm[4];
	char bs;
	char gps_x[9];
	char gps_y[9];
	char azimuth[3];
	char accelation_x[6];
	char accelation_y[6];
	char status[2];
	char acumulative_oil_usage[7];
	char residual_oil[3];
	char temperature_A[4];
	char temperature_B[4];
}__attribute__((packed));
typedef struct tacom_lbc_data tacom_lbc_data_t;
#pragma pack(pop)

#define DEFAULT_INTERVAL        3*60//3 min
#define RETRY_INTERVAL			10	//10 sec

#define SERVER_IP				"m2m.mdstec.com"
#define SERVER_PORT				30007

#define DTG_CONFIG_FILE_PATH			"/system/mds/system/bin/lbc.ini"

typedef struct {
	char server_ip[20];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	char server_ip[20];
	int port;
}DTG_REG_CONF;

typedef struct
{
	char server_ip[20];
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

s8* get_reg_server_ip_addr();
void set_reg_server_ip_addr(s8* ip_addr);
u16 get_reg_server_port();
void set_reg_server_port(u16 port);

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

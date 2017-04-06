#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

/*
#if defined(BOARD_TL500S)
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/w200_etrs.ini"
#elif defined(BOARD_TL500K)
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/w200k_etrs.ini"
#else
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/etrs.ini"
#endif
*/
#define ENABLE_VOLTAGE_USED_SCINARIO

#if defined(SERVER_MODEL_ETRS)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/etrs.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/etrs.ini"
	#define SERVER_IP				"218.153.4.33"
	#define SERVER_PORT				8622
#elif defined(SERVER_MODEL_ETRS_TB)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/etrs_tb.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/etrs_tb.ini"
	#define SERVER_IP				"218.153.4.33"
	#define SERVER_PORT				7322
#else
	#error "ETRS SEVER MODEL NOT DEFINE ERROR"
#endif

#if defined(BOARD_TL500K)
	#if defined(SERVER_MODEL_ETRS)
		#define KT_FOTA_CONFIG_FILE		"/system/mds/system/bin/kt_fota.ini"
		#define KT_FOTA_DM_SVC_IP		"devicefota_dm.show.co.kr"
		#define KT_FOTA_DM_SVC_PORT		80
		#define KT_FOTA_QTY_SVC_IP		"devicefota_quality_automotive.show.co.kr"
		#define KT_FOTA_QTY_SVC_PORT	80
		#define ACC_QTY_REPORT			1800
		#define ACC_FOTA_REQ_REPORT		0
	#else
		#define KT_FOTA_CONFIG_FILE		"/system/mds/system/bin/kt_fota_iot.ini"
		#define KT_FOTA_DM_SVC_IP		"devicefota-tb-dm.show.co.kr"
		#define KT_FOTA_DM_SVC_PORT		80
		#define KT_FOTA_QTY_SVC_IP		"devicefota-tb-quality.show.co.kr"
		#define KT_FOTA_QTY_SVC_PORT	80
		#define ACC_QTY_REPORT			300
		#define ACC_FOTA_REQ_REPORT		0
	#endif
#endif

enum eTrace_vehicle_status
{
	eRESERVED_VALUE1	= 0x00,
	eKEY_ON				= 0x01,
	eKEY_OFF			= 0x02,
	eRESERVED_VALUE2	= 0x03,
	eDTG_NOT_WORKING	= 0x04,
	ePOWER_NOT_SUPPLY	= 0x05,
	ePOWER_SUPPLY		= 0x06,

	eVOLT				   = 0x10,
	eVOLT_KEY_ON		   = 0x11,
	eVOLT_KEY_OFF		   = 0x12,
	eVOLT_DTG_NOTWORKING   = 0x14,
	eVOLT_KEY_ON_NOT_POWER = 0x15,
	eVOLT_KEY_OFF_POWER    = 0x16,
	
};
typedef enum eTrace_vehicle_status eTrace_vehicle_status_t;

#pragma pack(push, 1)
//define packet structure
struct etrace_packet_date {
	unsigned short year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
}__attribute__((packed));
typedef struct etrace_packet_date etrace_packet_date_t;

struct etrace_packet_hdr {
	char prot_id;			//fixed 0x45('E')
	char data_id;
	char terminal_id[15];	//phone number, put date from left. remaining buffer make 0x20(space).
	etrace_packet_date_t date;
	unsigned short dtg_id;
	unsigned int data_size;
}__attribute__((packed));
typedef struct etrace_packet_hdr etrace_packet_hdr_t;


struct etrace_dtg_hdr {
	char model[20];
	char vin[25]; //Vehicle identification number
	unsigned char vechicle_type;
	char vrn[12]; //Vehicle registeration number
	char brn[10]; //bussiness registeration number
	char driver_code[18];
	unsigned short record_count;
}__attribute__((packed));
typedef struct etrace_dtg_hdr etrace_dtg_hdr_t;

struct etrace_dtg_body {
	unsigned short distance_a_day;
	unsigned int distance_all;
	unsigned char date[7];				//(year - 2000)/mon/day hour/min/sec/ 1/100, each bytes binary
	unsigned char speed;
	unsigned short rpm;
	unsigned char bs;			//ON : 1, OFF : 0
	int gps_x;					//* 1,000,0000 --> 127.23738393 = 
	int gps_y;					//* 1,000,0000
	unsigned short azimuth;
	short accelation_x;
	short accelation_y;
	unsigned char status_code;
	unsigned int rtUsedFuelAday; //while a day, amount of using fuel.
	unsigned int rtUsedFuelAll;  //total amount of using fuel.
	unsigned int rtRemainFuelAmount;
	short temp1;
	short temp2;
	short temp3;	
}__attribute__((packed));
typedef struct etrace_dtg_body etrace_dtg_body_t;


struct etrace_ternminal_info {
	char model[20];
	char vin[25];			//Vehicle identification number
	unsigned char vechicle_type;
	char vrn[12];			//Vehicle registeration number
	char brn[10];			//bussiness registeration number
	char driver_code[18];
	unsigned int distance_all;
	int gps_x;				//* 1,000,0000 --> 127.23738393 = 
	int gps_y;				//* 1,000,0000
	unsigned char vs;
	unsigned short report_period;
	char fw_ver[20];
}__attribute__((packed));
typedef struct etrace_ternminal_info etrace_ternminal_info_t;


////////////////////////////////////////
//packet reponse type define
////////////////////////////////////////
enum eTrace_packet_repsonse_code
{
	eSUCCESS			= 'Y',
	eUnKnown_DATA_ERR	= 'D',
	eNOT_SUBSCRIBE_ERR	= 'N',
	eDATA_INVALIE_ERR	= 'X',
	eSEVER_BUSY_ERR		= 'E'
};
typedef enum eTrace_packet_repsonse_code eTrace_packet_repsonse_code_t;

enum eTrace_packet_config_code
{
	eCF_DTG_ID			= 'D',
	eCF_SERVER_IP_PORT	= 'A',
	eCF_SERVER_DNS_PORT	= 'S',
	eCF_FOTA			= 'F',
	eCF_ACCUMULDIST		= 'L',
	eCF_VIN				= 'V',
	eCF_NONE			= 'N'
};
typedef enum eTrace_packet_config_code eTrace_packet_config_code_t;


struct etrace_config_dtg_id {
	unsigned int dtg_id;
}__attribute__((packed));
typedef struct etrace_config_dtg_id etrace_config_dtg_id_t;

struct etrace_config_dtg_ip{
	char ip[15];
	unsigned int port;
}__attribute__((packed));
typedef struct etrace_config_dtg_ip etrace_config_dtg_ip_t;

struct etrace_config_dtg_dns{
	char dns[30];
	unsigned int port;
}__attribute__((packed));
typedef struct etrace_config_dtg_dns etrace_config_dtg_dns_t;

struct etrace_config_dtg_acc_dist{ //acc_dist : accumulative distance
	unsigned int new_dist;
}__attribute__((packed));
typedef struct etrace_config_dtg_acc_dist etrace_config_dtg_acc_dist_t;

#pragma pack(pop)

typedef struct
{
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	int report_period;
	int vold_key_on_report_period;
	int vold_key_off_report_period;
}DTG_PERIOD_CONF;

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	DTG_NETWORK_CONF server_conf;
	DTG_PERIOD_CONF dtg_conf;
	u8 action_working;
}DTG_management;

u8 is_working_action();
void set_working_action(u8 flag);

void set_signal();
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();

int init_configuration_data();

void set_dtg_report_period(s32 num);
s32 get_dtg_report_period();

s8* get_server_ip_addr();
void set_server_ip_addr(s8* ip_addr);
u16 get_server_port();
void set_server_port(s32 port);


int get_volt_key_on_report_period();
void set_volt_key_on_report_period(int num);
int get_volt_key_off_report_period();
void set_volt_key_off_report_period(int num);
#endif

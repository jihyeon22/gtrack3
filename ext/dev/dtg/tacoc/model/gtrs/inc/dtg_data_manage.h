#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

////////////////////////////////////////////////////////////
//New Model Add Contents
////////////////////////////////////////////////////////////


#if defined(SERVER_MODEL_GTRS)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/gtrs.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/gtrs.ini"
	#define SERVER_IP					"112.169.88.14"
	#define SERVER_PORT					5200

	#define MDT_SERVER_IP				"112.169.88.14"
	#define MDT_SERVER_PORT				5100
#elif defined(SERVER_MODEL_GTRS_TB)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/gtrs_tb.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/gtrs_tb.ini"
	#define SERVER_IP					"112.169.88.14"
	#define SERVER_PORT					5200

	#define MDT_SERVER_IP				"112.169.88.14"
	#define MDT_SERVER_PORT				5100
#elif defined(SERVER_MODEL_PRINET)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/prinet.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/prinet.ini"
	#define SERVER_IP					"tacho.ezauto.co.kr"
	#define SERVER_PORT					28911
	#define MDT_SERVER_IP				"tacho.ezauto.co.kr"
	#define MDT_SERVER_PORT				28911


#elif defined(SERVER_MODEL_MORAM)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/moram.ini"
	#define DTG_CONFIG_FILE_PATH		"/factory/moram.ini"
	#define SERVER_IP					"112.169.88.14"
	#define SERVER_PORT					5200

	#define MDT_SERVER_IP				"112.169.88.14"
	#define MDT_SERVER_PORT				5100

#elif defined(SERVER_MODEL_NEOGNP)
	#define DTG_CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/neognp.ini"
	#define DTG_CONFIG_FILE_PATH		"/data/mds/data/neognp.ini"
	#define SERVER_IP					"210.116.106.71"
	#define SERVER_PORT					5000
#else
	#error "GTRS SEVER MODEL NOT DEFINE ERROR"
#endif

#if defined(BOARD_TL500K)
	#if defined(SERVER_MODEL_GTRS) || defined(SERVER_MODEL_PRINET) || defined(SERVER_MODEL_MORAM)
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


enum eGtrace_Event_Code
{
	eIP_SetUp_Response		= 1,
	ePeriod_Report			= 5,
	ePower_Event			= 6,
	eKeyOn_Event			= 26,
	eKeyOff_Event			= 27,
};
typedef enum eGtrace_Event_Code eGtrace_Event_Code_t;

enum eGtrace_Device_Type
{
	eNone_Device		= 0,
	eTOTAL_Device		= 1,
	eDTG_Device			= 2,
	eOBD_Device			= 3,
};
typedef enum eGtrace_Device_Type eGtrace_Device_Type_t;

enum eGtrace_Product_Type
{
	eChoyoung		= 1,
	eDongsun		= 2,
	eLoop			= 3, //Loop2
	eKDT			= 4,
	eSinhung		= 5,
	eUCAR			= 6,
	eIREAL			= 7,
	eINNOCAR		= 8,
	eCJ				= 9,
	eDAESIN			= 10,
	eJastec			= 50,
};
typedef enum eGtrace_Product_Type eGtrace_Product_Type_t;

#pragma pack(push, 1)
struct gtrace_packet_body {
	unsigned char prot_id; //0x11(fix)
	unsigned char msg_id;  //0x64(fix)
	char device_id[15]; //phone number
	unsigned char evt_code;
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char gps_status;
	int gps_y; //latitude(????),  36.xxx
	int gps_x; // longitude(??), 127.xxx
	unsigned char bearing;
	unsigned short speed;
	int acc_dist;
	short temp1;
	short temp2;
	short temp3;
	unsigned short report_period;
	unsigned char gpio;
	unsigned char power;
	short create_period;
	unsigned short crc16;
	unsigned char term_char;
}__attribute__((packed));
typedef struct gtrace_packet_body gtrace_packet_body_t;


struct gtrace_dtg_user_data_payload {
	unsigned long distance_a_day;
	unsigned long distance_all;
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char msec;
	unsigned char speed;
	unsigned short rpm;
	unsigned char bs;		//|bs|0|0|0|0|0|0|0|
	unsigned long gps_y; //latitude(????),  36.xxx
	unsigned long gps_x; // longitude(??), 127.xxx
	unsigned short azimuth;
	short accelation_x;
	short accelation_y;
	unsigned char status_code;
	unsigned int rtUsedFuelAday;
	unsigned int rtUsedFuelAll;
}__attribute__((packed));
typedef struct gtrace_dtg_user_data_payload gtrace_dtg_user_data_payload_t;

struct gtrace_dtg_user_data_summary {
	char dtg_model[20];
	char vin[17];
	unsigned char vechicle_type[2];
	char vrn[12];
	char brn[10];
	char driver_code[18];
}__attribute__((packed));
typedef struct gtrace_dtg_user_data_summary gtrace_dtg_user_data_summary_t;


struct gtrace_dtg_user_data_hdr {
	unsigned short length;
	unsigned int packet_id;
	char cmd_id[4];
	unsigned char dev_type;
	unsigned char product_type;
}__attribute__((packed));
typedef struct gtrace_dtg_user_data_hdr gtrace_dtg_user_data_hdr_t;

struct gtrace_dtg_server_response {
	unsigned long packet_id;
	unsigned char result;
}__attribute__((packed));
typedef struct gtrace_dtg_server_response gtrace_dtg_server_response_t;

#if defined(DEVICE_MODEL_INNOCAR)
	#define SET_FLAG_WEIGHT2			0x0001
	#define SET_FLAG_WEIGHT1			0x0002
	#define SET_FLAG_CUMUL_OIL_USAGE	0x0004
	#define SET_FLAG_OOD				0x0008
	#define SET_FLAG_BLN				0x0010 //business_license_num
	#define SET_FLAG_COMPANY_NAME		0x0020 //company name
	#define SET_FLAG_RPM_FACTOR			0x0040 //rpm-factor
	#define SET_FLAG_K_FACTOR			0x0080 //k-facotr
	#define SET_FLAG_VIN				0x0100 //vehicle_id_num
	#define SET_FLAG_DRIVER_CODE		0x0200 //driver_code
	#define SET_FLAG_DRIVER_NAME		0x0400 //driver_name
	#define SET_FLAG_VT					0x0800 //vehicle_type
	#define SET_FLAG_RN					0x1000 //registration_num
	#define SET_FLAG_RESERVED1			0x2000
	#define SET_FLAG_RESERVED2			0x4000
	#define SET_FLAG_RESERVED3			0x8000

	struct innocar_dtg_set_value {
		char registration_num[12];
		unsigned char vehicle_type;
		char driver_name[10];
		char driver_code[18];
		char vehicle_id_num[17];
		unsigned short k_factor;
		unsigned char rmp_factor;
		char commay_name[12];
		char business_license_num[10];
		unsigned long odo;
		unsigned long cumul_oil_usage;
		char weight1;
		char weight2;
		char reserved[4];
	}__attribute__((packed));
	typedef struct innocar_dtg_set_value innocar_dtg_set_value_t;
#endif

#pragma pack(pop)

typedef struct
{
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	int report_period;
}DTG_PERIOD_CONF;

typedef struct
{
	char server_ip[64];
	int port;
}MDT_NETWORK_CONF;

typedef struct
{
	int report_period;
	int create_period;
}MDT_PERIOD_CONF;

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;
	DTG_NETWORK_CONF server_conf;
	DTG_PERIOD_CONF dtg_conf;
	MDT_NETWORK_CONF mdt_svr_conf;
	MDT_PERIOD_CONF mdt_conf;
	u8 action_working;
#if defined(DEVICE_MODEL_INNOCAR)
	unsigned short K_Factor;
	char RPM_Factor;
	char weight1;
	char weight2;
	char dtg_ver[8];
#endif
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

s8* get_mdt_server_ip_addr();
void set_mdt_server_ip_addr(s8* ip_addr);
u16 get_mdt_server_port();
void set_mdt_server_port(s32 port);

void set_mdt_report_period(s32 num);
s32 get_mdt_report_period();

void set_mdt_create_period(s32 num);
s32 get_mdt_create_period();

unsigned short get_k_factor();
unsigned short get_rmp_factor();
unsigned short get_weight1();
unsigned short get_weight2();
char * get_dtg_version();
void set_factor_value(unsigned short k_facotr, char rmp_factor, char weight1, char weight2, char *dtg_ver);

#endif

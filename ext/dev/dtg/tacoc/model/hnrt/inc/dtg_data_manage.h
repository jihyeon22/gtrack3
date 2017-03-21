#ifndef __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__
#define __DTG_DATA_CONFIGURATION_AND_MANAGEMENT_HEADER__

#include "dtg_type.h"
#include <pthread.h>

#if defined(SERVER_MODEL_HNRT)
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/hnrt.ini"
#elif defined(SERVER_MODEL_HNRT_DEV)
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/hnrt_dev.ini"
#elif defined(SERVER_MODEL_HNRT_TB)
	#define DTG_CONFIG_FILE_PATH	"/system/mds/system/bin/hnrt_tb.ini"
#else
	#error "HNRT Server Model Not Define Error"
#endif


#define SERVER_IP				"219.254.35.130"
#define SERVER_PORT				3157

#define REG_SERVER_IP			"m2m.mdstec.com"
#define REG_SERVER_PORT			30007

#if defined(BOARD_TL500K)
	#if defined(SERVER_MODEL_HNRT)
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

#pragma pack(push, 1)
struct msg_hdr
{
	unsigned short prtc_id;
	unsigned char msg_id;
	unsigned char svc_id;
	unsigned char msg_len_mark[3];
}__attribute__((packed));
typedef struct msg_hdr msg_hdr_t; //MessageHeader

struct msg_terminal_info {
	char dtg_model[20];
	char modem_model[20];
	char dtg_serial[20];
	char modem_serial[20];
	char dtg_fw_ver[10];
	char modem_fw_ver[10];
	char mdn[11];
	char vrn[12];
}__attribute__((packed));
typedef struct msg_terminal_info msg_term_info_t;

struct term_packet
{
	msg_hdr_t header;
	msg_term_info_t body;
}__attribute__((packed));
typedef struct term_packet term_pck_t;

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
	//signed long gps_x;
	//signed long gps_y;
	signed long gps_y;
	signed long gps_x;
	//unsigned short azimuth;
	//unsigned char speed;
	unsigned char azimuth;
	unsigned short speed;
	unsigned long on_accumulative_distance;
	signed short temperature_a;
	signed short temperature_b;
	signed short temperature_c;
	unsigned short report_period;		//set on tacoc
	unsigned char gpio_input;
	unsigned char power_type;
	unsigned short create_period;		//set on tacoc
	unsigned short crc_val;				//set on tacoc
	char endflag;						//set on tacoc
}__attribute__((packed));
typedef struct msg_mdt msg_mdt_t;

struct mdt_packet
{
	msg_hdr_t header;
	msg_mdt_t body;
}__attribute__((packed));
typedef struct mdt_packet mdt_pck_t;

struct dtg_hdr {
	char dtg_model[20];
	char vin[17];
	unsigned char vechicle_type;
	char vrn[12];
	char brn[10];
	char driver_code[18];
}__attribute__((packed));
typedef struct dtg_hdr dtg_hdr_t;

struct msg_dtg
{
	unsigned long tid;
	unsigned char trip_num;
	unsigned char option_flag[3];
	dtg_hdr_t dtg_hdr;
	unsigned short dtg_num;
}__attribute__((packed));
typedef struct msg_dtg msg_dtg_t;

struct dtg_packet
{
	msg_hdr_t header;
	msg_dtg_t body;
}__attribute__((packed));
typedef struct dtg_packet dtg_pck_t;

struct dtg_data {
	unsigned long timestamp;
	unsigned char timestampmsec;
	unsigned long distance_a_day;
	unsigned long distance_all;
	unsigned long distance_trip;
	unsigned char speed;
	unsigned short rpm;
	unsigned char bs;		//|bs|0|0|0|0|0|0|0|
	unsigned long gps_x;
	unsigned long gps_y;
	unsigned short azimuth;
	short accelation_x;
	short accelation_y;
	unsigned char status_code;
#if defined(DEVICE_MODEL_LOOP2)
	unsigned int rtUsedFuelAday;
	unsigned int rtUsedFuelAll;
	unsigned int rtUsedFuelTrip;
#endif
}__attribute__((packed));
typedef struct dtg_data dtg_data_t;

struct resp_packet
{
	msg_hdr_t header;
	unsigned char result;
}__attribute__((packed));
typedef struct resp_packet resp_pck_t;

struct msg_vehiclestate
{
	char vrn[12];
	unsigned int tid;
	unsigned char opationflag[3];
	unsigned int timestamp;
	unsigned char timestapmMsec;
	unsigned short EventCode;
	//unsigned char PowerType;
	
	unsigned int gps_x;
	unsigned int gps_y;
	unsigned short azimuth;
	char speed;
	int distantall;

	unsigned short batteryVoltage;
	short temp1;
	short temp2;
}__attribute__((packed));
typedef struct msg_vehiclestate msg_vehiclestate_t;

struct vhcstatus_packet
{
	msg_hdr_t header;
	msg_vehiclestate_t body;
}__attribute__((packed));
typedef struct vhcstatus_packet vhc_pck_t;

#pragma pack(pop)

typedef enum OptionalAndFlags OptionalAndFlags_t;
enum OptionalAndFlags
{
	bitReserved_1			= 0x000001,
	bitReserved_2			= 0x000002,
	bitReserved_3			= 0x000004,
	bitReserved_4			= 0x000008,
	bitReserved_5			= 0x000010,
	bitReserved_6			= 0x000020,
	bitReserved_7			= 0x000040,
	bitReserved_8			= 0x000080,
	bitReserved_9			= 0x000100,
	bitReserved_10			= 0x000200,
	
	eOptionalVehicleInfoExt	= 0x000400,
	eOptionalLoadWeight		= 0x000800,
	eOptionalTemperature	= 0x001000,
	eOptionAirPressure		= 0x002000,
	eOptionWaterTemperature	= 0x004000,
	eOptionBatteryVoltage	= 0x008000,
	eOptionFuelTankVotage	= 0x010000,
	eOptionFuelTankLiter	= 0x020000,
	eOptionFuelTankPercent	= 0x040000,
	eOptionTranmissionLever	= 0x080000,
	eOptionGradient			= 0x100000,
	eOptionTransmission		= 0x200000,
	eOptionAccelator 		= 0x400000,
	eOptionUsedFuel 		= 0x800000,
	
};

typedef enum MessageID MessageID_t;
enum MessageID
{
	eMsgTerminalInfo       = 0x01,
	eMsgDTG                = 0x02,
	eMsgMDT                = 0x03,
	eMsgVechieState        = 0x04,
	eMsgFWUpgrade          = 0x05,
	eMsgFWUpgradeResponse  = 0x06,
	eMsgDTGFileInfo        = 0x07,
	eMsgDTGFile            = 0x08,
	eMsgResponse           = 0xff,
};

struct mdt_strt
{
	mdt_pck_t buf;
	int buf_size;
};
typedef struct mdt_strt mdt_strt_t;

#define	MDT_MAX 64

#define MSG_HDR			7
#define MSG_MDT			12
#define MSG_DTG_HDR		12
#define MSG_DTG_DATA	12

#define MAX_RECORD_NUM 255
#define MAX_RECORD_DATA_LEN MAX_RECORD_NUM*RECORD_DATA_LEN


typedef struct
{
	char server_ip[64];
	int port;
}DTG_NETWORK_CONF;

typedef struct
{
	int report_period;
	int create_period;
}DTG_PERIOD_CONF;

typedef struct
{
	int trip_num;
	int cumul_dist;
}MAINTAIN_CONF;

typedef struct
{
	char server_ip[64];
	int port;
	char service[128];
}DTG_REG_CONF;

#define MAX_REPORT_CONT 20
#define MAX_REPORT1_LEN 53
#define MAX_REPORT2_LEN 51
#define MAX_REPORT_MSG_LEN 300

typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t thread_cond;

	DTG_NETWORK_CONF server_conf;
	DTG_PERIOD_CONF dtg_conf;
	DTG_PERIOD_CONF mdt_conf;
	MAINTAIN_CONF maintain_info;
	DTG_REG_CONF reg_conf;

	u8 action_working;
	
	u16 package_type;
	u8 package_version_major;
	u8 package_version_minor;
}DTG_management;

u8 is_working_action();
void set_working_action(u8 flag);

void set_signal();
void lock_mutex();
void unlock_mutex();
void wait_for_singleobject();

int init_configuration_data();

void set_dtg_report_period(s32 num);
void set_dtg_create_period(s32 num);
void set_mdt_report_period(s32 num);
void set_mdt_create_period(s32 num);
s32 get_dtg_report_period();
s32 get_dtg_create_period();
s32 get_mdt_report_period();
s32 get_mdt_create_period();

void set_trip_number(s32 num);
void set_cumulative_dist(s32 num);
s32 get_trip_number();
s32 get_cumulative_dist();

s8* get_server_ip_addr();
void set_server_ip_addr(s8* ip_addr);
u16 get_server_port();
void set_server_port(s32 port);


/* Package */
unsigned short get_package_type();
void set_package_type(unsigned short package_type);
unsigned char get_package_major_version();
unsigned char get_package_minor_version();
void set_package_version(unsigned char major, unsigned char minor);

#endif

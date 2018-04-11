#ifndef __MODEL_PACKET_H__
#define __MODEL_PACKET_H__

#include <base/gpstool.h>
#include <board/rfidtool.h>

typedef struct locationData locationData_t;
struct locationData{
	gpsData_t gpsdata;
	int acc_status;
	unsigned int mileage_m;
	char event_code;
	unsigned char avg_speed;
};

#define MAX_BUFDATA 2048
typedef struct bufData bufData_t;
struct bufData
{
	char buf[MAX_BUFDATA];
	int used_size;
};

typedef struct clAdasData clAdasData_t;
struct clAdasData
{
	int event_code;
	int adas_speed;
	char adas_opt_str[6+1];
};


/*
   CL RESPONSE MESSAGE
*/
typedef struct {
	char stx;
	char command[3];
	char unit_id[13];
}__attribute__((packed))CL_COMM_HEAD;

typedef struct {
	char etx;
}__attribute__((packed))CL_COMM_TAIL;

typedef struct {
	char status;
	char server_ip[15];
	char server_port[5];  
}__attribute__((packed))CL_SERVER_BODY;

typedef struct {
	char status;
	char interval_time[5];
	char max_packet[2];  
}__attribute__((packed))CL_REPORT_BODY;

typedef struct {
	char zone_type;
	char lat[9];
	char lon[9];
	char distance[4];
	char zone_id[2];
}__attribute__((packed))CL_ZONE;

typedef struct {
	char status;
	CL_ZONE z;
}__attribute__((packed))CL_GIO_BODY;

typedef struct {
	char status;
	char stop_time[7];
}__attribute__((packed))CL_STOP_TIME_BODY;

typedef struct {
	char status;
	char m_kmh[11][4];
}__attribute__((packed))CL_MIR_BODY;

typedef struct {
	char sw_ver[8];
	char hw_ver[8];
	char server_ip[15];
	char server_port[5];
	char interval_time[5];
	char max_packet[2];
	char interval_dist[5];
	char speed[3];
	char acc_flag;
	char acc_time[5];
	char stop_time[7];
	char park_time[7];
	char out1;
	char out2;
	char out3;
	char batt_type;
	char batt_level[3];
	char door_mode;
	char door_active;
	char run_mode;
	char m_kmh[11][4];
	char zone_count[2];
	CL_ZONE zone_data[10];
}__attribute__((packed))CL_MST_BODY;

typedef struct {
	char stx;
	char error_code;
	char data[4096];
}__attribute__((packed))CL_RESP_PACKET;

/*
   CL REPORT MESSAGE
*/

typedef struct {
    CL_COMM_HEAD head;
	char length[3];
	char data_count[2];
}__attribute__((packed))CL_PACKET_HEAD;

typedef struct {
	unsigned char check_sum;
    CL_COMM_TAIL tail;  
}__attribute__((packed))CL_PACKET_TAIL;

typedef struct {
	char date[6];
	char time[6];  
	char gps_status;  
	char latitude[8];  
	char longitude[9];  
	char speed[3];  
  	char direction[3];  
	char avg_speed[3];  	// v06 : avg speed, v08 : motion sensor
  	char acc_status;  		// v06 : acc stat , v08 : door sensor
  	char accumul_dist[6];  	// v06 : accumul dist , v08 : thermal sensor
  	char event_code[2];      
}__attribute__((packed))CL_LOCATION_BODY;

typedef struct {
	char date[6];
	char time[6];  
	char gps_status;  
	char latitude[8];  
	char longitude[9];  
	char speed[3];  
  	char direction[3];  
	char adas_speed[3];  	// v06 : avg speed, v08 : motion sensor
  	char reserved_2;  		// v06 : acc stat , v08 : door sensor
  	char adas_opt[6];  	// v06 : accumul dist , v08 : thermal sensor
  	char adas_event_code[2];      
}__attribute__((packed))CL_ADAS_BODY;

typedef struct {
   CL_LOCATION_BODY data;
  	char boarding;  
  	char tag_id[32];      
}__attribute__((packed))CL_RFID_BODY;

typedef struct {
	CL_PACKET_HEAD head;
	CL_LOCATION_BODY body;
	CL_PACKET_TAIL tail;  
}__attribute__((packed))CL_LOCATION_PACKET;

typedef struct {
	CL_PACKET_HEAD head;
	CL_RFID_BODY body;
	CL_PACKET_TAIL tail;  
}__attribute__((packed))CL_RFID_PACKET;

typedef struct {
	CL_PACKET_HEAD head;
	CL_ADAS_BODY body;
	CL_PACKET_TAIL tail;  
}__attribute__((packed))CL_ADAS_PACKET;


#define CL_SUC_ERROR_CODE 0x30
#define CL_AUT_ERROR_CODE 0x31
#define CL_CHK_ERROR_CODE 0x32
#define CL_LEN_ERROR_CODE 0x33
#define CL_SET_ERROR_CODE 0x34

#define RFID_RF_ON_CMD 0x10
#define RFID_RF_OFF_CMD 0x11
#define RFID_FIND_CARD_CMD 0x4C
#define RFID_BUZZER_LED_CMD 0x13
#define RFID_LED_CMD 0xFF

#define RFID_OK_RESP 0x00
#define RFID_FIND_CARD_RESP 0x4D
#define RFID_NO_TAG_ERROR 0x02

#define CL_PKT_TYPE_NORMAL_REPORT	0
#define CL_PKT_TYPE_THERMAL_REPORT	1

#define CMD_MSI "MSI"
#define CMD_MIT "MIT"
#define CMD_CSS "CSS"
#define CMD_RST "RST"
#define CMD_CPO "CPO"
#define CMD_MPL "MPL"	// 온도센서 없는경우
#define CMD_MPT "MPT"	// 온도센서 있는경우
#define CMD_MPF "MPF"
#define CMD_GIO "GIO"
#define CMD_MGZ "MGZ"
#define CMD_MIR "MIR"
#define CMD_MST "MST"

#define CMD_MPA "MPA"

enum CL_EVENT_CODE
{
    CL_NONE_EVENT_CODE = 0,

    CL_MODEM_ON_EVENT_CODE = 1,
    CL_MODEM_OFF_EVENT_CODE = 2,

    CL_ACC_ON_EVENT_CODE = 3,
    CL_ACC_OFF_EVENT_CODE = 4,

    CL_CYCLE_EVENT_CODE = 5,
    CL_DISTANCE_EVENT_CODE = 6,

    CL_LIMITED_EVENT_CODE = 7,
    CL_INSTANCE_EVENT_CODE = 8,
    CL_EMERGENCY_EVENT_CODE = 9,

    CL_ZONE_IN_EVENT_CODE = 10,
    CL_ZONE_OUT_EVENT_CODE = 11,

    CL_STOP_WRN_EVENT_CODE = 12,

    CL_GPS_ON_EVENT_CODE = 13,
    CL_GPS_OFF_EVENT_CODE = 14,

    CL_PWR_ON_EVENT_CODE = 15,
    CL_PWR_OFF_EVENT_CODE = 16,

	CL_MDM_INTERNAL_BATT_LOW_EVENT_CODE = 17,
	CL_CAR_BATT_LOW_EVENT_CODE = 18,

	CL_INPUT_PORT_CHANGE_EVENT_CODE = 19,

	CL_DOOR_OPEN_EVENT_CODE = 20,
	CL_DOOR_CLOSE_EVENT_CODE = 21,

	CL_PARKING_EVENT_CODE = 22,
	CL_AREA_EVENT_CODE = 23,

	CL_NET_CONNECTION_RECOVERY_EVENT_CODE = 40,
	CL_BUFFER_OVERFLOW_EVENT_CODE = 40,

    CL_RFID_BOARDING_CODE = 50,
    CL_RFID_ALIGHTING_CODE = 51,

	CL_CAR_ACCIDENT_BTN_EVENT_CODE = 63, // 
	CL_CAR_BROKEN_BTN_EVENT_CODE = 64,

	// ADAS MGR
	CL_ADAS_FCW_EVENT_CODE = 81,		// 전방추돌경보
	CL_ADAS_UFCW_EVENT_CODE = 82,		// 서행충돌경보
	CL_ADAS_LDW_EVENT_CODE = 83,		// 차선이탈
	CL_ADAS_PCW_EVENT_CODE = 84,		// 보행자 충돌 경보
	CL_ADAS_SLI_EVENT_CODE = 85,		// 속도제한
	CL_ADAS_TSR_EVENT_CODE = 86,		// 교통표지판 인식
	CL_ADAS_HMW_EVENT_CODE = 87,		// 차간거리 경보
	CL_ADAS_RESERVED_EVENT_CODE = 88,	// RESERVED
	CL_ADAS_ERR_EVENT_CODE = 89,		// ERR

	CL_ADAS_PWR_ON = 90,				// ADAS단말기 전원ON
	CL_ADAS_PWR_OFF = 91,				// ADAS단말기 전원OFF
	
	CL_ADAS_FPW_HMW_EVENT_CODE = 92,	// 근접추돌 경보
	
};

#define RESULT_OK 0
#define RESULT_RETRY -1
#define RESULT_ERR -2
#define RESULT_NA 1

#define MAX_DATA_COUNT	10

int cl_resp_packet_check(char result);
int CL_event_code_send(int code);
int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc);
int make_report_packet(unsigned char **pbuf, unsigned short *packet_len, int pkt_type);
int make_mit_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int interval, unsigned int count);
int make_msi_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned char *ip, unsigned int port);
int make_css_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int stop_time);
int make_gio_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body);
int make_mgz_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body);
int make_rfid_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc, rfidData_t *param_rfid);
int make_mst_packet(unsigned char **pbuf, unsigned short *packet_len, CL_MST_BODY *body);
int make_raw_packet(unsigned char **pbuf, unsigned short *packet_len, bufData_t *pbuf_data);
int make_mir_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int *m_kmh);
int make_mpa_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata, clAdasData_t *param);

#endif

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

#define CMD_MSI "MSI"
#define CMD_MIT "MIT"
#define CMD_CSS "CSS"
#define CMD_RST "RST"
#define CMD_CPO "CPO"
#define CMD_MPL "MPL"	// ver 0.6 spec
#define CMD_MPT "MPT"	// ver 0.8 spec // 주기보고
#define CMD_MPF "MPF"
#define CMD_GIO "GIO"
#define CMD_MGZ "MGZ"
#define CMD_MIR "MIR"
#define CMD_MST "MST"

enum CL_EVENT_CODE
{
    CL_NONE_EVENT_CODE = 0,
    CL_MODEM_ON_EVENT_CODE,
    CL_MODEM_OFF_EVENT_CODE,
    CL_ACC_ON_EVENT_CODE,
    CL_ACC_OFF_EVENT_CODE,
    CL_CYCLE_EVENT_CODE = 5,
    CL_DISTANCE_EVENT_CODE,
    CL_LIMITED_EVENT_CODE,
    CL_INSTANCE_EVENT_CODE,
    CL_EMERGENCY_EVENT_CODE,

    CL_ZONE_IN_EVENT_CODE = 10,
    CL_ZONE_OUT_EVENT_CODE,
    CL_STOP_WRN_EVENT_CODE,
    CL_GPS_ON_EVENT_CODE,
    CL_GPS_OFF_EVENT_CODE,
    CL_PWR_ON_EVENT_CODE = 15,
    CL_PWR_OFF_EVENT_CODE,
    CL_MD_LBAT_EVENT_CODE,
    CL_CAR_LBAT_EVENT_CODE,
    CL_PORT_CH_EVENT_CODE,

    CL_CAR_OPEN_EVENT_CODE = 20,
    CL_CAR_CLOSE_EVENT_CODE,
    CL_PARKING_EVENT_CODE,
    CL_AREA_EVENT_CODE,

    CL_RFID_BOARDING_CODE = 50,
    CL_RFID_ALIGHTING_CODE,
};

#define RESULT_OK 0
#define RESULT_RETRY -1
#define RESULT_ERR -2
#define RESULT_NA 1

#define MAX_DATA_COUNT	10

int cl_resp_packet_check(char result);
int CL_event_code_send(int code);
int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc);
int make_report_packet(unsigned char **pbuf, unsigned short *packet_len);
int make_mit_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int interval, unsigned int count);
int make_msi_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned char *ip, unsigned int port);
int make_css_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int stop_time);
int make_gio_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body);
int make_mgz_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body);
int make_rfid_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc, rfidData_t *param_rfid);
int make_mst_packet(unsigned char **pbuf, unsigned short *packet_len, CL_MST_BODY *body);
int make_raw_packet(unsigned char **pbuf, unsigned short *packet_len, bufData_t *pbuf_data);
int make_mir_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int *m_kmh);

#endif

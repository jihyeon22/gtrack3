#ifndef __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_
#define __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_

#include <base/gpstool.h>
#include "kt_flood_unit.h"
#include "leakdata-list.h"


#define MAX_DEV_ID_LED			15
#define MDT800_PROTOCOL_ID		0x11
#define MDT800_MESSAGE_TYPE		0x64
#define MDT800_MESSAGE_TYPE2		0x81
#define MDT800_PACKET_END_FLAG	0x7E

#define LIMIT_TRANSFER_PACKET_COUNT		360 //60 min buffer, if 10 sec collection cycle

#define KT_THERMAL_VER_STRING		"271"

#define MAX_DEV_ID_LEN				5
#define FLOOD_PROTOCOL_ID			0x1A
#define FLOOD_PROTOCOL_ENC_TYPE		0x00
#define FLOOD_UNIQUE_TYPE			0x01
#define FLOOD_PACKET_START_FLAG		02
#define FLOOD_PACKET_END_FLAG		03
#define FLOOD_PACKET_WARRING_IDX	17
// #define MAX_DEVICE_ID				11
// #define INIT_DEVICE_ID				"00000000000"

typedef enum msg_ev_code msg_ev_code_t;
enum msg_ev_code{
	/* TCP/IP Response Event code about IP RE-SETUP command by SMS */
	eIP_SETUP_EVC                   = 1,	
	/* TCP/IP Response Event code about REPORT CYCLE TIME RE-SETUP by SMS */
	eREPORT_CYCLE_SETUP_EVT         = 2,	
	/* TCP/IP Response Event code about ODO(cumulative distance) RE-SETUP by SMS */
	eODO_SETUP_EVC                  = 3,	
	/* TCP/IP Response Event code about ODO(cumulative distance) RE-SETUP by SMS */
	eDEV_VER_INFO                  = 3,	
	/* TCP/IP Response Event code about requesting MDT STATUS by SMS */
	eMDT_STATUS_EVC                 = 4,	
	/* TCP/IP Response Event code about requesting MDT STATUS by SMS */
	eMDT_DEV_RESET                  = 4,	
	/* cycling report event code */
	eCYCLE_REPORT_EVC               = 5,	
	/* device power source changing event code */
	ePOWER_SOURCE_CHANGE_EVT        = 6,	
	/*NUM0 Button Pressed Event Code*/
	eBUTTON_NUM0_EVT                = 10,	
	/*NUM1 Button Pressed Event Code*/
	eBUTTON_NUM1_EVT                = 11,	
	eBUTTON_START_MILEAGE_EVT       = 11,	
	/*NUM2 Button Pressed Event Code*/
	eBUTTON_NUM2_EVT                = 12,	
	/*NUM3 Button Pressed Event Code*/
	eBUTTON_NUM3_EVT                = 13,	
	eBUTTON_END_MILEAGE_EVT         = 13,	
	/*NUM4 Button Pressed Event Code*/
	eBUTTON_NUM4_EVT                = 14,	
	/*NUM5 Button Pressed Event Code*/
	eBUTTON_NUM5_EVT                = 15,	
	/*GPIO NUM0 Event Code (button mode is exeption) */
	eGPIO_NUM0_EVT                  = 20,	
	/*GPIO NUM1 Event Code (button mode is exeption) */
	eGPIO_NUM1_EVT                  = 21,	
	/*GPIO NUM2 Event Code (button mode is exeption) */
	eGPIO_NUM2_EVT                  = 22,	
	/*GPIO NUM4 Event Code (button mode is exeption) */
	eGPIO_NUM3_EVT                  = 23,	
	/* Ignition ON Event Code (if gpio5 set up ignition sigal, this event occure */
	eIGN_ON_EVT                     = 26,	
	/* Ignition OFF Event Code (if gpio5 set up ignition sigal, this event occure */
	eIGN_OFF_EVT                    = 27,	
	/* TCP/IP Response Event code about GPIO mode re-setup by SMS */
	eGPIO_MODE_SETUP                = 30,	
	/* TCP/IP Response Event code about GPIO OUT re-setup by SMS */
	eGPIO_OUT_SETUP                 = 31,	
	/* TCP/IP Response Event code about REPORT CYCLE #2 TIME RE-SETUP by SMS */
	eREPORT_CYCLE_NUM2_SETUP_EVT    = 32,	
	/* TCP/IP Response Event code about Geo Fence RE-SETUP by SMS */
	eGEO_FENCE_SETUP                = 33,	
	/* GEO FENCE NUM #0 entry Event Code */
	eGEO_FENCE_NUM0_ENTRY_EVT       = 34,	
	/* GEO FENCE NUM #0 exit Event Code */
	eGEO_FENCE_NUM0_EXIT_EVT        = 35,	
	/* GEO FENCE NUM #1 entry Event Code */
	eGEO_FENCE_NUM1_ENTRY_EVT       = 36,	
	/* GEO FENCE NUM #1 exit Event Code */
	eGEO_FENCE_NUM1_EXIT_EVT        = 37,

	/*TCP/IP Request ODO initial value */
	eREQUEST_INIT_ODO               = 38,

	/*change flood  value */
	eUART_CHANGE_EVT                = 39,

#if defined(CORP_ABBR_NISSO)
	/* GEO FENCE NUM #2 entry Event Code */
	eGEO_FENCE_NUM2_ENTRY_EVT       = 40,
	/* GEO FENCE NUM #2 exit Event Code */
	eGEO_FENCE_NUM2_EXIT_EVT        = 41,
	/* GEO FENCE NUM #3 entry Event Code */
	eGEO_FENCE_NUM3_ENTRY_EVT       = 42,
	/* GEO FENCE NUM #3 exit Event Code */
	eGEO_FENCE_NUM3_EXIT_EVT        = 43
#endif
};

typedef enum gps_status gps_status_t;
enum gps_status{
	eSAT_GSP = 1, /* GPS satellites */
	eWCDMA_GSP = 2, /* WCDMA CELL */
	eNOSIGNAL_GPS = 3
};

typedef enum gps_dir gps_dir_t;
enum gps_dir{
	eNORTH_DIR,
	eNORTH_EAST_DIR,
	eEAST_DIR,
	eSOUTH_EAST_DIR,
	eSOUTH_DIR,
	eSOUTH_WEST_DIR,
	eWEST_DIR,
	eNORTH_WEST_DIR
};

typedef enum power_source power_source_t;
enum power_source{
	eEXT_POWER,
	eBAT_POWER,
};

#pragma pack(push, 1)

typedef struct {
	int latitude;
	int longitude;
}gps_pos_t;

typedef struct {
	unsigned short year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
}__attribute__((packed))packet_date_t;

typedef struct {
	unsigned char  msg_id;
	unsigned char  msg_type;
	unsigned char  dev_id[MAX_DEV_ID_LED];
	unsigned char  evcode;
	packet_date_t  date;
	unsigned char  gps_status;
	gps_pos_t      gps_pos;
	unsigned char  gps_dir;
	unsigned short speed;
	unsigned int   vehicle_odo;
	unsigned short report_cycle_time; /* unit : sec */
	unsigned char  gpio_status;
	unsigned char  dev_power;
	unsigned short create_cycle_time; /* unit : sec */
	unsigned char version[3];
	unsigned char record_leng;
	unsigned char record[100]; /*variable length*/
}__attribute__((packed))lotte_packet2_t;

typedef struct {	
	unsigned char XOR;
	unsigned char ETX;
}__attribute__((packed))flood_etx_t;

typedef struct {
	unsigned char year[2];
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
}__attribute__((packed))flood_date_t;

// typedef struct {
// 	unsigned char sensor1_num;
// 	unsigned char sensor1_state;
// 	unsigned char sensor2_num;
// 	unsigned char sensor2_state;
// 	unsigned char sensor3_num;
// 	unsigned char sensor3_state;
// 	unsigned char sensor4_num;
// 	unsigned char sensor4_state;
// 	unsigned char sensor5_num;
// 	unsigned char sensor5_state;
// 	unsigned char sensor6_num;
// 	unsigned char sensor6_state;
// }__attribute__((packed))flood_sensor_t;

typedef struct {
	unsigned char sensor_num;
	unsigned char sensor_state;
}__attribute__((packed))flood_sensor_t;

typedef struct {
	unsigned char	start_idx;
	unsigned char	msg_id;
	unsigned char	msg_enc_type;
	unsigned int	msg_len;
	unsigned char	unique_type;
	unsigned char	dev_id[MAX_DEV_ID_LEN];
	flood_date_t	date;
	unsigned char	sensor_date_len;
	flood_sensor_t	sensor_data[MAX_SENSOR_CNT];
	unsigned char	end_idx;
}__attribute__((packed))flood_packet_t;

typedef struct {
	unsigned char deviceid_len;
	unsigned char deviceid[MAX_DEVICE_ID];
}__attribute__((packed))warning_data_t;

typedef struct {
	unsigned char	msg_id;
	unsigned char	msg_enc_type;
	unsigned int	msg_len;
	unsigned short	report_interval;
	unsigned char	ip1;
	unsigned char	ip2;
	unsigned char	ip3;
	unsigned char	ip4;
	unsigned short	port;
	unsigned char	connected;
	unsigned char	warning_num;
}__attribute__((packed))resp_packet_t;

#pragma pack(pop)

int create_report2_data(int ev_code, lotte_packet2_t *packet, gpsData_t gpsdata, char *record, int rec_len);
void print_report2_data(lotte_packet2_t packet);
int create_report2_divert_buffer(unsigned char **buf, int num);
int is_available_report_divert_buffer(int cnt);

int create_flood_divert_buffer(unsigned char **buf, int num);
int create_flood_report_data(flood_packet_t *packet);
void print_flood_report_data(flood_packet_t packet);


#endif /* __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_ */


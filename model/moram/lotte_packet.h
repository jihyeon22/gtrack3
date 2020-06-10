#ifndef __LOTTE_VEHICLE_GEO_TRACK_PACKET_HEADER_
#define __LOTTE_VEHICLE_GEO_TRACK_PACKET_HEADER_

#include <base/gpstool.h>

#define MAX_DEV_ID_LED			15
#define LOTTE_PROTOCOL_ID		0x11
#define LOTTE_MESSAGE_TYPE		0x64
#define LOTTE_PACKET_END_FLAG	0x7E

#define LIMIT_TRANSFER_PACKET_COUNT		360 //60 min buffer, if 10 sec collection cycle

typedef enum msg_ev_code msg_ev_code_t;
enum msg_ev_code{
	/* TCP/IP Response Event code about IP RE-SETUP command by SMS */
	eIP_SETUP_EVC                   = 1,	
	/* TCP/IP Response Event code about REPORT CYCLE TIME RE-SETUP by SMS */
	eREPORT_CYCLE_SETUP_EVT         = 2,	
	/* TCP/IP Response Event code about ODO(cumulative distance) RE-SETUP by SMS */
	eODO_SETUP_EVC                  = 3,	
	/* TCP/IP Response Event code about requesting MDT STATUS by SMS */
	eMDT_STATUS_EVC                 = 4,	
	/* cycling report event code */
	eCYCLE_REPORT_EVC               = 5,	
	/* device power source changing event code */
	ePOWER_SOURCE_CHANGE_EVT        = 6,	
	/*NUM0 Button Pressed Event Code*/
	eBUTTON_NUM0_EVT                = 10,	
	/*NUM1 Button Pressed Event Code*/
	eBUTTON_NUM1_EVT                = 11,	
	/*NUM2 Button Pressed Event Code*/
	eBUTTON_NUM2_EVT                = 12,	
	/*NUM3 Button Pressed Event Code*/
	eBUTTON_NUM3_EVT                = 13,	
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

	eGPS_ANT_CONNECT			= 67,
	eGPS_ANT_DISCONNECT		= 68,

	eMAX_EVENT_CODE                 = 94
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
	//unsigned short gps_reliability;
	//unsigned short gps_sat_count;
	//unsigned short vehicle_volt;
	unsigned short temp1;
	unsigned short temp2;
	unsigned short temp3;
	unsigned short report_cycle_time; /* unit : sec */
	unsigned char  gpio_status;
	unsigned char  dev_power;
	unsigned short create_cycle_time; /* unit : sec */

}__attribute__((packed))lotte_packet_t;

#pragma pack(pop)

void create_report_data(int ev_code, lotte_packet_t *packet, gpsData_t gpsdata);
void print_report_data(lotte_packet_t packet);
int create_report_divert_buffer(unsigned char **buf, int size);
int is_available_report_divert_buffer(int cnt);
#endif /* __LOTTE_VEHICLE_GEO_TRACK_PACKET_HEADER_ */

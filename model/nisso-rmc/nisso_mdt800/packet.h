#ifndef __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_
#define __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_

#include <base/gpstool.h>

#define MAX_DEV_ID_LED			15
#define MDT800_PROTOCOL_ID		0x11
#define MDT800_MESSAGE_TYPE		0x64
#define MDT800_MESSAGE_TYPE2	0x81
#define MDT800_PACKET_END_FLAG	0x7E

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
//	eBUTTON_NUM4_EVT                = 14,	
	/*NUM5 Button Pressed Event Code*/
//	eBUTTON_NUM5_EVT                = 15,	
	/*GPIO NUM0 Event Code (button mode is exeption) */
//	eGPIO_NUM0_EVT                  = 20,	
	/*GPIO NUM1 Event Code (button mode is exeption) */
//	eGPIO_NUM1_EVT                  = 21,	
	/*GPIO NUM2 Event Code (button mode is exeption) */
//	eGPIO_NUM2_EVT                  = 22,	
	/*GPIO NUM4 Event Code (button mode is exeption) */
//	eGPIO_NUM3_EVT                  = 23,	
	/* Ignition ON Event Code (if gpio5 set up ignition sigal, this event occure */
	eIGN_ON_EVT                     = 26,	
	/* Ignition OFF Event Code (if gpio5 set up ignition sigal, this event occure */
	eIGN_OFF_EVT                    = 27,	
	/* Modem Reset Event Code */ // NISSO
	eMDM_DEV_RESET                  = 28,	
	/* TCP/IP Response Event code about GPIO mode re-setup by SMS */
	//eGPIO_MODE_SETUP                = 30,	
	/* TCP/IP Response Event code about GPIO OUT re-setup by SMS */
	eGPIO_OUT_SETUP                 = 31,	
	/* Spot Setting Response */
	eGEO_FENCE_SETUP                = 32,	// NISSO
	/* GEO FENCE NUM #0 entry Event Code */
	eGEO_FENCE_NUM0_ENTRY_EVT       = 33,	// NISSO
	/* GEO FENCE NUM #0 exit Event Code */
	eGEO_FENCE_NUM0_EXIT_EVT        = 34,	// NISSO
	/* GEO FENCE NUM #1 entry Event Code */
	eGEO_FENCE_NUM1_ENTRY_EVT       = 35,	// NISSO
	/* GEO FENCE NUM #1 exit Event Code */
	eGEO_FENCE_NUM1_EXIT_EVT        = 36,	// NISSO
	/* GEO FENCE NUM #2 entry Event Code */
	eGEO_FENCE_NUM2_ENTRY_EVT       = 37,	// NISSO
	/* GEO FENCE NUM #2 exit Event Code */
	eGEO_FENCE_NUM2_EXIT_EVT        = 38,	// NISSO
	/* GEO FENCE NUM #3 entry Event Code */
	eGEO_FENCE_NUM3_ENTRY_EVT       = 39,	// NISSO
	/* GEO FENCE NUM #3 exit Event Code */
	eGEO_FENCE_NUM3_EXIT_EVT        = 40,	// NISSO
	/* GEO FENCE NUM #4 entry Event Code */
	eGEO_FENCE_NUM4_ENTRY_EVT       = 41,	// NISSO
	/* GEO FENCE NUM #4 exit Event Code */
	eGEO_FENCE_NUM4_EXIT_EVT        = 42,	// NISSO

	/* Invoice receive Event Code */
	eINVOCE_RECV_EVT                = 43,
	/* SMS SEND Event Code */
	eSMS_TRANSFER_EVT               = 45,

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
	unsigned char  dev_power_level;
	unsigned short create_cycle_time; /* unit : sec */
	unsigned char version[3];
	unsigned char reseved;
}__attribute__((packed))nisso_packet2_t;

typedef struct {
	unsigned short time;
	gps_pos_t gps_pos;
}__attribute__((packed))etrace_packet_t;

#pragma pack(pop)

int create_report2_data(int ev_code, nisso_packet2_t *packet, gpsData_t gpsdata, char *record, int rec_len);
void print_report2_data(nisso_packet2_t packet);
int create_report2_divert_buffer(unsigned char **buf, int num);
int is_available_report_divert_buffer(int cnt);

char set_nisso_pkt__invoice_info(int invoice);
char get_nisso_pkt__invoice_info();

#define EXTERNAL_PWR_ON		1
#define EXTERNAL_PWR_OFF	0

char set_nisso_pkt__external_pwr(char pwr_stat);
char get_nisso_pkt__external_pwr();


#endif /* __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_ */

//3333030364676
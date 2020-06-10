#ifndef __MDT_DATA_DEFINE_HDR__
#define __MDT_DATA_DEFINE_HDR__

#define MAX_DEV_ID_LED			15
#define MDT800_PROTOCOL_ID		0x11
#define MDT800_MESSAGE_TYPE		0x64
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

	eKIT_DEFINE_1_EVT			= 90,
	eKIT_DEFINE_2_EVT			= 93,

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
	short temp1;
	short temp2;
	short temp3;
	unsigned short report_cycle_time; /* unit : sec */
	unsigned char  gpio_status;
	unsigned char  dev_power;
	unsigned short create_cycle_time; /* unit : sec */
	unsigned short crc16;

}__attribute__((packed))mdt800_packet_t;

#pragma pack(pop)

int get_mdt_count();
void save_mdt_data(mdt800_packet_t *data);
void clear_mdt_data(int clear_count);
int get_mdt_data(unsigned char *buf, int request_count);

int parse_mdt_msg(unsigned char *std_buff, unsigned char *dest, int ev);
int mdt_enc_msg(unsigned char *src, unsigned char *dest, int len);
#endif //__MDT_DATA_DEFINE_HDR__

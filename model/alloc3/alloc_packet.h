#ifndef __ALLOCATION_RFID_PACKET_HEADER__
#define __ALLOCATION_RFID_PACKET_HEADER__

#include <time.h>

#include <base/gpstool.h>

#define MAX_DEV_ID_LED				15

// allocation packet info..
#define PACKET_START_CHAR	'{'
#define PACKET_END_CHAR		'}'
#define PACKET_SPILT		','
#define SMS_END_CHAR		0x7E

#define MAX_RCV_PACKET_SIZE			512
#define PKT_FRAME_QUEUE_BUFF_SIZE	(MAX_RCV_PACKET_SIZE * 2)

typedef struct {
	unsigned char  packet_content[MAX_RCV_PACKET_SIZE];
	int size;
}__attribute__((packed))packet_frame_t;

typedef struct {
	unsigned char year[4];
	unsigned char mon[2];
	unsigned char day[2];
	unsigned char hour[2];
	unsigned char min[2];
	unsigned char sec[2];
}__attribute__((packed))packet_date_t;

// ===========================================================================
//  allocation protocol id 
// ===========================================================================
typedef enum alloc_protocol_id_code alloc_protocol_id_code_t;
enum alloc_protocol_id_code
{
	ePROTOCOL_ID_PERIOD_REPORT = 'M',
	ePROTOCOL_ID_TAG_REPORT	   = 'T',
	ePROTOCOL_ID_GET_PASSENGER = 'P',
	ePROTOCOL_ID_RESP_SMS	   = 'B',
	ePROTOCOL_ID_BUS_STOP_INFO = 'V', //jwrho
	ePROTOCOL_ID_RESP_DM 	   = 'D', //jwrho
};


// ============================================================================
//  Protocol info : ePROTOCOL_ID_PERIOD_REPORT
// ============================================================================
// temper sensor define
#define PKT_PRI_DEF_TEMP_SENSOR_OPEN	-3333
#define PKT_PRI_DEF_TEMP_SENSOR_SHORT	-4444
#define PKT_PRI_DEF_TEMP_SENSOR_NONE	-5555
#define PKT_PRI_DEF_TEMP_DEV_GET_RFID	888
#define PKT_PRI_DEF_TEMP_DEV_GET_STOP	777

// protocol info...
typedef enum packet_def_period packet_def_period_t;
enum packet_def_period{
	ePKT_PERI_IDX_PROTCOL_ID = 0,		// 1
	ePKT_PERI_IDX_MSG_TYPE,				// 1
	ePKT_PERI_IDX_DATA_CONTINUOUS,		// 1
	ePKT_PERI_IDX_DEV_ID,				// 15
	ePKT_PERI_IDX_EVT_CODE,				// 1
	ePKT_PERI_IDX_EVT_DATA,				// 3
	ePKT_PERI_IDX_DATE,					// 14
	ePKT_PERI_IDX_GPS_STAT,				// 1
	ePKT_PERI_IDX_GPS_LAT,				// 11
	ePKT_PERI_IDX_GPS_LOT,				// 11
	ePKT_PERI_IDX_GPS_DIR,				// 1
	ePKT_PERI_IDX_GPS_SPEED,			// 4
	ePKT_PERI_IDX_TOTAL_DIST,			// 6
	ePKT_PERI_IDX_TEMP1,				// 5
	ePKT_PERI_IDX_TEMP2,				// 5
	ePKT_PERI_IDX_TEMP3,				// 5
	ePKT_PERI_IDX_REPORT_CYCLE_SEC,		// 4
	ePKT_PERI_IDX_GPIO_STAT,			// 1
	ePKT_PERI_IDX_PWR_TYPE,				// 1
	ePKT_PERI_IDX_CREAT_CYCLE_SEC,		// 4
	ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	ePKT_PERI_IDX_STOP_ZONE_ID,			// 8
	ePKT_PERI_IDX_STOP_ZONE_IDX,		// 2
	ePKT_PERI_IDX_STOP_ZONE_STAT,		// 1
	ePKT_PERI_IDX_MAX_PREIOD_PKT,
};

// 	msg type..
typedef enum alloc_msg_type alloc_msg_type_t;
enum alloc_msg_type
{
	eMSG_TYPE_PREIOD_REPORT = '6',
	eMSG_TYPE_RESP_SMS = 'S',
};

// 	msg event code
typedef enum alloc_evt_code alloc_evt_code_t;
enum alloc_evt_code
{
	eEVT_CODE_PERIOD_REPORT 	= '0',
	eEVT_CODE_START_DRIVE 		= '1',
	eEVT_CODE_END_DRIVE 		= '2',
	eEVT_CODE_PASSENGER_FULL 	= '3',
	eEVT_CODE_EMERGENCY_BTN		= '4',
};

#define SUB_EVT_CODE_GEOFENCE_NONE	0
#define SUB_EVT_CODE_GEOFENCE_IN	1
#define SUB_EVT_CODE_GEOFENCE_OUT	2

// 	gps stat
typedef enum gps_status gps_status_t;
enum gps_status{
	eSAT_GSP = '1', /* GPS satellites */
	eWCDMA_GSP = '2', /* WCDMA CELL */
	eNOSIGNAL_GPS = '0'
};

// gps rssi
typedef enum gps_rssi gps_rssi_t;
enum gps_rssi{
	eGPS_NOSIGNAL = 0,
	eGPS_NOFIX,
	eGPS_FIX
};

// 	gps dir..
typedef enum gps_dir gps_dir_t;
enum gps_dir{
	eNORTH_DIR = '0',
	eNORTH_EAST_DIR = '1',
	eEAST_DIR = '2',
	eSOUTH_EAST_DIR = '3',
	eSOUTH_DIR = '4',
	eSOUTH_WEST_DIR = '5',
	eWEST_DIR = '6',
	eNORTH_WEST_DIR = '7'
};

// 	pwr src
typedef enum power_source power_source_t;
enum power_source{
	eEXT_POWER = '0',
	eBAT_POWER = '1',
};

// ============================================================================
//  Protocol info : binary
// ============================================================================
typedef enum packet_def_get_passenger packet_def_get_passenger_t;
enum packet_def_get_passenger{
	ePKT_PESG_IDX_PROTCOL_ID = 0,	// 1
	ePKT_PESG_IDX_DEV_ID,			// 15
	ePKT_PESG_IDX_VERSION_INFO,		// 8
	ePKT_PESG_IDX_MAX_PASSENGER_INFO_PKT,
};

typedef enum rcv_bin_pktid rcv_bin_pktid_t;
enum rcv_bin_pktid{
	eRCV_PKT_ID_PASSENGER_INFO = '1', 
	eRCV_PKT_ID_SEND_PERIOD = '2',
	eRCV_PKT_ID_STOP_INFO = '3',
	eRCV_PKT_ID_FTP_INFO = '4',
	eRCV_PKT_ID_FIRMWARE_UPDATE = '5',
	//eNOSIGNAL_GPS = 3
};

#define VERSION_APP_HARDCODING "00.87"

#pragma pack(push, 1)

typedef struct {
	unsigned char latitude[11]; 
	unsigned char longitude[11]; // 11byte??
}gps_pos_t;

typedef struct {
	int pkt_idx;
	int field_size;
	char* pkt_descript;
}__attribute__((packed))pkt_define_t;





#pragma pack(pop)

int _alloc_get_packet_frame(const char* buff, int buff_len, packet_frame_t* result);
int mkpkt_report_data(	unsigned char ** buff, char ev_code, char* ev_data, char *phonenum, 
						int temp1, int temp2, int temp3, 
						gpsData_t gpsdata, 
						char* zone_id, int zone_idx, int zone_state, 
						int total_dist_cm);
int mkpkt_tag_data(	unsigned char ** buff, char *phonenum,
						int count, char* zone_id, char* date,
						int num_rfid, char* list_rfid);
int mkpkt_get_passenger(unsigned char ** buff, char* phonenum, char* version_info);
int mkpkt_sms_resp_tcp_data(unsigned char ** buff, char *phonenum);
int mkpkt_bus_stop_info_tcp_data(unsigned char ** buff, char *phonenum); //jwrho
int mkpkt_sms_resp_sms_data(unsigned char ** buff);
int mkpkt_sms_resp_dm_data(unsigned char ** buff, int setup);
void pkt_send_period_report(gpsData_t gpsdata);
void pkt_send_tagging(void);
void pkt_send_btn_passenger_full(void);
void pkt_send_start_drive(void);
void pkt_send_end_drive(void);
void pkt_send_geofence_in(gpsData_t gpsdata, int fence_idx);
void pkt_send_geofence_out(gpsData_t gpsdata, int fence_idx);
void pkt_send_get_rfid(void);
void pkt_send_btn_emergency();

#endif /* __MDT800_VEHICLE_GEO_TRACK_PACKET_HEADER_ */

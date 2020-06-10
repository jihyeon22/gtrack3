<<<<<<< HEAD
#ifndef __OPENSNS_SERVER_PACKET_DEFINE_HEADER__
#define __OPENSNS_SERVER_PACKET_DEFINE_HEADER__


#define MAX_GPS_TRACK_COUNT		500
#define RESOURCE_COUNT			4
#define MAX_RESOURCE_DATA_LEN	15


typedef enum Business_CODE Business_CODE_t;
enum Business_CODE
{
	eBC_LBS       = 0x01,
	eBC_ASP       = 0x02,
};

typedef enum Service_CODE Service_CODE_t;
enum Service_CODE
{
	eSC_WASTE				= 0x01,
	eSC_TAXICALL			= 0x02,
	eSC_LUGGAGE_TRACKING	= 0x03,
	eSC_DTG_DATA			= 0x04,
	eSC_NETWORK_ONS			= 0x05,
	eSC_SKNETWORKS_TCMS		= 0x06
	
};

typedef enum Packet_Encrypt Packet_Encrypt_t;
enum Packet_Encrypt
{
	ePE_NOT_APPLY		= 0x00,
	ePE_ARIA_128BIT		= 0x01,
	ePE_ARIA_192BIT		= 0x02,
	ePE_ARIA_256BIT		= 0x03,
	
};

typedef enum Packet_Compression Packet_Compression_t;
enum Packet_Compression
{
	ePC_NO_ZIP		= 0x00,
	ePC_ZIP			= 0x01,
};

typedef enum Response_Type Response_Type_t;
enum Response_Type
{
	eRT_REQUEST_NO_RESPONSE			= 0x00,
	eRT_REQUEST_RESPOSE				= 0x01,
	eRT_REQUEST_ECHO_MODE_RESPONSE	= 0x02

};

typedef enum Device_state Device_state_t;
enum Device_state
{
	eDS_KeyOn			= 0x00,
	eDS_Key_On_Running	= 0x01,
	eDS_KeyOff			= 0x02,
	eDS_Key_Off_Running	= 0x03

};

typedef enum DTG_acc_status DTG_acc_status_t;
enum DTG_acc_status
{
	eDTG_Key_Invalid     = 0xFF,
	eDTG_KeyOn			 = 0x00,
	eDTG_Key_On_Running	 = 0x40, 
	eDTG_KeyOff			 = 0x80,
	eDTG_Key_Off_Running = 0xC0,

};

typedef enum ALERT_CODE ALERT_CODE_t;
enum ALERT_CODE
{
	eAC_LUGGAGE_INFO_CHANGE					= 1,
	eAC_DRIVER_VEHICLE_SETUP_INFO_CHANGE	= 2,
	eAC_REQUEST_DATA = 3, /* DTG/TAXI */
	eAC_REQUEST_DEVICE_STATUS	= 4,
	eAC_REQUEST_UPGRADE_CHECK = 5,
	eAC_REQUEST_DEVICE_LOG = 6,
	eAC_NOTI_NORMAL_MSG = 7,
	eAC_NOTI_LOST_DEVICE = 8,
	eAC_SERVER_INFO_CHANGE = 9,
	eAC_REQUEST_CONFIGURED_VALUE = 10,
	eAC_REQUEST_CONFIGURATION_VALUE_CHANGE = 11

};

typedef enum RESOURCE_ITEM RESOURCE_ITEM_t;
enum RESOURCE_ITEM
{
	eRI_ACCIDENT					= 1,
	eRI_VEHICLE_BATTERY				= 2,
	eRI_TEMPERATURE					= 3,
	eRI_AIR_PRESSUER				= 4,
	eRI_TANK_CAPACITY				= 5,
	eRI_WEIGHT						= 6,
	eRI_ODO							= 7,
	eRI_SPEED						= 8, /* ascii : 0~255 */
	eRI_RPM							= 9,   /* ascii : 0000~9999 */
	eRI_BRAKE						= 10,
	eRI_E_RPM						= 100,
	eRI_E_SPEED						= 101,
	eRI_FUEL_INJECTION				= 102, //연료 분사량
	eRI_E_BRAKE						= 103,
	eRI_INSTANCE_FUEL_EFFICIENCY	= 104, //연비(순간)
	eRI_TRIP_FUEL_EFFICIENCY		= 105, //연비(Trip)
	eRI_DAY_FUEL_EFFICIENCY			= 106, //연비(Day)
	eRI_AVG_FUEL_EFFICIENCY			= 107, //연비(평균)

	eRI_INSTANCE_OIL_USAGE			= 108, //연료사용량(순간)
	eRI_TRIP_OIL_USAGE				= 109, //연료사용량(Trip)
	eRI_DAY_OIL_USAGE				= 110, //연료사용량(Day)
	eRI_ACCUMULATE_OIL_USAGE		= 111, //연료사용량(누적)
	
	eRI_INSTANCE_DRIVING_MILEAGE	= 112, //주행거리(순간)
	eRI_TRIP_DRIVING_MILEAGE		= 113,//주행거리(Trip)
	eRI_DAY_DRIVING_MILEAGE			= 114,//주행거리(Day)
	eRI_ACCUMULATE_DRIVING_MILEAGE	= 115,//주행거리(누적)

	eRI_ACCELERATOR_PEDAL			= 116 //가속 페달

};

#pragma pack(push, 1)

struct gps_track_info_pack
{
	unsigned int gps_start_time; //utc
	long start_x;
	long start_y;
	unsigned char collection_interval;
	unsigned short data_cnt;
}__attribute__((packed));
typedef struct gps_track_info_pack gps_track_info_pack_t;

struct gps_track_data
{
	char dy; //delta y;
	char dx; //delta x
}__attribute__((packed));
typedef struct gps_track_data gps_track_data_t;

struct message_define
{
	unsigned int msg_len;
	unsigned char business_code;
	unsigned char service_code;
	unsigned int dst_ip;
	unsigned short dst_port;
	unsigned char packet_encrypt;
	unsigned char packet_compression;
}__attribute__((packed));
typedef struct message_define message_define_t;

struct message_header
{
	unsigned int msg_body_len;	/*msg header + msg_body*/
	unsigned int msg_date; //utc
	unsigned char msg_flow; /*request : 0x01, reponse : 0x02*/
	char terminal_ID[15];
	unsigned char terminal_type; //12 fixed
	unsigned char operation_code;
	unsigned char operation_flag;
	char work_id[16]; //vehicle register number;
	unsigned char response_type;
	unsigned char device_state;
	unsigned int driver_id;
	unsigned char protocol_ver;
	unsigned short return_code;
	unsigned int terminal_ip;
	unsigned short port;
	unsigned char gpsinfo_flag;
}__attribute__((packed));
typedef struct message_header message_header_t;

struct message_env_pack
{
	unsigned char dev_ctrl;
	unsigned char state_sned_ctrl;
	unsigned short state_send_cycle;
	unsigned char data_send_ctrl;
	unsigned short data_send_cycle;
	unsigned char gps_ctrl;
	unsigned char gps_interval;
}__attribute__((packed));
typedef struct message_env_pack message_env_pack_t;


#if defined(DEVICE_MODEL_INNOCAR)
	#define FRAMEWORK_SIZE_44	//use dtg record pack2
#elif defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	#define FRAMEWORK_SIZE_44	//use dtg record pack2
#else
	#error "No Define DTG record pack"
#endif
//#define FRAMEWORK_SIZE_44 //use dtg record pack2
//if FRAMEWORK_SIZE_44 and FRAMEWORK_SIZE_48 don't define, use dtg record pack1

struct dtg_header
{
	char vehicle_model[20];
	char vehicle_id_num[17];
	unsigned char vehicle_type;
	char registration_num[12];
	char business_license_num[10];
	unsigned char driver_idx;
	char driver_code[7];

#if defined(FRAMEWORK_SIZE_44)
	char reserved[20]; 
#else
	char reserved[28]; //all of cases exception FRAMEWORK_SIZE_44
#endif

}__attribute__((packed));
typedef struct dtg_header dtg_header_t;

//#define DTG_RECORD_BIG_ENDIAN

struct dtg_record_pack
{
	unsigned int cumul_run_dist;
	unsigned int date_time; //UTC
	int gps_x;
	int gps_y;
	short accelation_x;
	short accelation_y;
	unsigned short day_run_dist;
	short azimuth;
	unsigned short rpm;
	unsigned char speed;
	unsigned char msec;
	unsigned char brake;
	unsigned char gps_status; //0 : valid, 11 : Invalid
	unsigned char dummy1;
	unsigned char dummy2;

#if defined(FRAMEWORK_SIZE_44) || defined(FRAMEWORK_SIZE_48) //pack2
	unsigned int day_oil_usage;
	unsigned int cumulative_oil_usage; //unit : ml
	short temperature_A; //Unit : 0.1, Invalid : 0xA000, example> 22.3 -> 223
	short temperature_B; //Unit : 0.1, Invalid : 0xA000
#endif

#if defined(FRAMEWORK_SIZE_48)
	unsigned short residual_oil; //unit 0.1 litter, example> 22.3 -> 223
	unsigned short dummy3;
#endif

}__attribute__((packed));
typedef struct dtg_record_pack dtg_record_pack_t;

#pragma pack(pop)

#endif //__OPENSNS_SERVER_PACKET_DEFINE_HEADER__
=======
#ifndef __OPENSNS_SERVER_PACKET_DEFINE_HEADER__
#define __OPENSNS_SERVER_PACKET_DEFINE_HEADER__


#define MAX_GPS_TRACK_COUNT		500
#define RESOURCE_COUNT			4
#define MAX_RESOURCE_DATA_LEN	15


typedef enum Business_CODE Business_CODE_t;
enum Business_CODE
{
	eBC_LBS       = 0x01,
	eBC_ASP       = 0x02,
};

typedef enum Service_CODE Service_CODE_t;
enum Service_CODE
{
	eSC_WASTE				= 0x01,
	eSC_TAXICALL			= 0x02,
	eSC_LUGGAGE_TRACKING	= 0x03,
	eSC_DTG_DATA			= 0x04,
	eSC_NETWORK_ONS			= 0x05,
	eSC_SKNETWORKS_TCMS		= 0x06
	
};

typedef enum Packet_Encrypt Packet_Encrypt_t;
enum Packet_Encrypt
{
	ePE_NOT_APPLY		= 0x00,
	ePE_ARIA_128BIT		= 0x01,
	ePE_ARIA_192BIT		= 0x02,
	ePE_ARIA_256BIT		= 0x03,
	
};

typedef enum Packet_Compression Packet_Compression_t;
enum Packet_Compression
{
	ePC_NO_ZIP		= 0x00,
	ePC_ZIP			= 0x01,
};

typedef enum Response_Type Response_Type_t;
enum Response_Type
{
	eRT_REQUEST_NO_RESPONSE			= 0x00,
	eRT_REQUEST_RESPOSE				= 0x01,
	eRT_REQUEST_ECHO_MODE_RESPONSE	= 0x02

};

typedef enum Device_state Device_state_t;
enum Device_state
{
	eDS_KeyOn			= 0x00,
	eDS_Key_On_Running	= 0x01,
	eDS_KeyOff			= 0x02,
	eDS_Key_Off_Running	= 0x03

};

typedef enum DTG_acc_status DTG_acc_status_t;
enum DTG_acc_status
{
	eDTG_Key_Invalid     = 0xFF,
	eDTG_KeyOn			 = 0x00,
	eDTG_Key_On_Running	 = 0x40, 
	eDTG_KeyOff			 = 0x80,
	eDTG_Key_Off_Running = 0xC0,

};

typedef enum ALERT_CODE ALERT_CODE_t;
enum ALERT_CODE
{
	eAC_LUGGAGE_INFO_CHANGE					= 1,
	eAC_DRIVER_VEHICLE_SETUP_INFO_CHANGE	= 2,
	eAC_REQUEST_DATA = 3, /* DTG/TAXI */
	eAC_REQUEST_DEVICE_STATUS	= 4,
	eAC_REQUEST_UPGRADE_CHECK = 5,
	eAC_REQUEST_DEVICE_LOG = 6,
	eAC_NOTI_NORMAL_MSG = 7,
	eAC_NOTI_LOST_DEVICE = 8,
	eAC_SERVER_INFO_CHANGE = 9,
	eAC_REQUEST_CONFIGURED_VALUE = 10,
	eAC_REQUEST_CONFIGURATION_VALUE_CHANGE = 11

};

typedef enum RESOURCE_ITEM RESOURCE_ITEM_t;
enum RESOURCE_ITEM
{
	eRI_ACCIDENT					= 1,
	eRI_VEHICLE_BATTERY				= 2,
	eRI_TEMPERATURE					= 3,
	eRI_AIR_PRESSUER				= 4,
	eRI_TANK_CAPACITY				= 5,
	eRI_WEIGHT						= 6,
	eRI_ODO							= 7,
	eRI_SPEED						= 8, /* ascii : 0~255 */
	eRI_RPM							= 9,   /* ascii : 0000~9999 */
	eRI_BRAKE						= 10,
	eRI_E_RPM						= 100,
	eRI_E_SPEED						= 101,
	eRI_FUEL_INJECTION				= 102, //연료 분사량
	eRI_E_BRAKE						= 103,
	eRI_INSTANCE_FUEL_EFFICIENCY	= 104, //연비(순간)
	eRI_TRIP_FUEL_EFFICIENCY		= 105, //연비(Trip)
	eRI_DAY_FUEL_EFFICIENCY			= 106, //연비(Day)
	eRI_AVG_FUEL_EFFICIENCY			= 107, //연비(평균)

	eRI_INSTANCE_OIL_USAGE			= 108, //연료사용량(순간)
	eRI_TRIP_OIL_USAGE				= 109, //연료사용량(Trip)
	eRI_DAY_OIL_USAGE				= 110, //연료사용량(Day)
	eRI_ACCUMULATE_OIL_USAGE		= 111, //연료사용량(누적)
	
	eRI_INSTANCE_DRIVING_MILEAGE	= 112, //주행거리(순간)
	eRI_TRIP_DRIVING_MILEAGE		= 113,//주행거리(Trip)
	eRI_DAY_DRIVING_MILEAGE			= 114,//주행거리(Day)
	eRI_ACCUMULATE_DRIVING_MILEAGE	= 115,//주행거리(누적)

	eRI_ACCELERATOR_PEDAL			= 116 //가속 페달

};

#pragma pack(push, 1)

struct gps_track_info_pack
{
	unsigned int gps_start_time; //utc
	long start_x;
	long start_y;
	unsigned char collection_interval;
	unsigned short data_cnt;
}__attribute__((packed));
typedef struct gps_track_info_pack gps_track_info_pack_t;

struct gps_track_data
{
	char dy; //delta y;
	char dx; //delta x
}__attribute__((packed));
typedef struct gps_track_data gps_track_data_t;

struct message_define
{
	unsigned int msg_len;
	unsigned char business_code;
	unsigned char service_code;
	unsigned int dst_ip;
	unsigned short dst_port;
	unsigned char packet_encrypt;
	unsigned char packet_compression;
}__attribute__((packed));
typedef struct message_define message_define_t;

struct message_header
{
	unsigned int msg_body_len;	/*msg header + msg_body*/
	unsigned int msg_date; //utc
	unsigned char msg_flow; /*request : 0x01, reponse : 0x02*/
	char terminal_ID[15];
	unsigned char terminal_type; //12 fixed
	unsigned char operation_code;
	unsigned char operation_flag;
	char work_id[16]; //vehicle register number;
	unsigned char response_type;
	unsigned char device_state;
	unsigned int driver_id;
	unsigned char protocol_ver;
	unsigned short return_code;
	unsigned int terminal_ip;
	unsigned short port;
	unsigned char gpsinfo_flag;
}__attribute__((packed));
typedef struct message_header message_header_t;

struct message_env_pack
{
	unsigned char dev_ctrl;
	unsigned char state_sned_ctrl;
	unsigned short state_send_cycle;
	unsigned char data_send_ctrl;
	unsigned short data_send_cycle;
	unsigned char gps_ctrl;
	unsigned char gps_interval;
}__attribute__((packed));
typedef struct message_env_pack message_env_pack_t;


#if defined(DEVICE_MODEL_INNOCAR)
	#define FRAMEWORK_SIZE_44	//use dtg record pack2
#elif defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	#define FRAMEWORK_SIZE_44	//use dtg record pack2
#else
	#error "No Define DTG record pack"
#endif
//#define FRAMEWORK_SIZE_44 //use dtg record pack2
//if FRAMEWORK_SIZE_44 and FRAMEWORK_SIZE_48 don't define, use dtg record pack1

struct dtg_header
{
	char vehicle_model[20];
	char vehicle_id_num[17];
	unsigned char vehicle_type;
	char registration_num[12];
	char business_license_num[10];
	unsigned char driver_idx;
	char driver_code[7];

#if defined(FRAMEWORK_SIZE_44)
	char reserved[20]; 
#else
	char reserved[28]; //all of cases exception FRAMEWORK_SIZE_44
#endif

}__attribute__((packed));
typedef struct dtg_header dtg_header_t;

//#define DTG_RECORD_BIG_ENDIAN

struct dtg_record_pack
{
	unsigned int cumul_run_dist;
	unsigned int date_time; //UTC
	int gps_x;
	int gps_y;
	short accelation_x;
	short accelation_y;
	unsigned short day_run_dist;
	short azimuth;
	unsigned short rpm;
	unsigned char speed;
	unsigned char msec;
	unsigned char brake;
	unsigned char gps_status; //0 : valid, 11 : Invalid
	unsigned char dummy1;
	unsigned char dummy2;

#if defined(FRAMEWORK_SIZE_44) || defined(FRAMEWORK_SIZE_48) //pack2
	unsigned int day_oil_usage;
	unsigned int cumulative_oil_usage; //unit : ml
	short temperature_A; //Unit : 0.1, Invalid : 0xA000, example> 22.3 -> 223
	short temperature_B; //Unit : 0.1, Invalid : 0xA000
#endif

#if defined(FRAMEWORK_SIZE_48)
	unsigned short residual_oil; //unit 0.1 litter, example> 22.3 -> 223
	unsigned short dummy3;
#endif

}__attribute__((packed));
typedef struct dtg_record_pack dtg_record_pack_t;

#pragma pack(pop)

#endif //__OPENSNS_SERVER_PACKET_DEFINE_HEADER__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

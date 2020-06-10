<<<<<<< HEAD
#ifndef __LILA_PKT_H__
#define __LILA_PKT_H__


//typedef enum {
//}e_PKT_AAA;

#define LILA_PKT_PREFIX     0xcfcc
#define LILA_PKT_SUFFIX     0xd0cf

#define LILA_PKT_ID__DEVICE_ID      0x03

#define LILA_PKT_ID__INFO               "INFO" // Device  -> Server
#define LILA_PKT_ID__DATA_SEND_CONFIRM  "CSTD" // "DTSC" // Server  -> Device
#define LILA_PKT_ID__DATA_SEND_SYNC     "DTSS" // "DTSS" // Server <-> Device

#define LILA_PKT_RET__FAIL      -1
#define LILA_PKT_RET__SUCCESS   1

typedef struct {
    char reserved_1[4];             // Reserved 	Binary 	4
    char reserved_2[7];             // Reserved 	Binary 	7
    char driver_name[10];           //운전자 이름 	ASCII 	10
    char driver_code[18];           // 운전자 코드 	ASCII 	18
    char car_regi_no[12];           // 자동차 등록번호 	Binary 	12
    char car_code[17];              // 차대번호 	ASCII 	17
    char company_name[12];          // 회사 이름 	ASCII 	12
    char company_regi_no_1[10];     // 사업자등록번호 	ASCII 	10
    char company_regi_no_2[10];     // 형식 승인 번호 	ASCII 	10
    char serial_no[14];             // 일련번호 	ASCII 	14
    char model_no[20];              // 모델번호 	ASCII 	20
    unsigned short k_factor;        // K-Factor 	decimal 	2
    unsigned short rpm_factor;      // RPM-Factor 	decimal 	2
    char reserved_3;                // Reserved 	Binary 	1
    char firmware_ver[6];           // 펌웨어버전 	Binary 	6
    char reserved_4[4];             // Reserved 	Binary 	4
    char reserved_5[38];            // Reserved 	Binary 	38
    char reserved_6[1];             // Reserved 	Binary 	1
    char reserved_7[4];             // Reserved 	Binary 	4
    char reserved_8[4];             // Reserved 	Binary 	4
    char reserved_9[4];             // Reserved 	Binary 	4
    char reserved_10[56];           // Reserved 	Binary 	56
}__attribute__((packed))LILA_PKT_DATA__DTG_INFO_T; // 256 byte

typedef struct {
    unsigned int  utc_time;                 // Time 	4	u32 
    unsigned char speed;                    // Speed 	1	u8 
    unsigned char speed_float;              // Speed Float 	1	u8 
    unsigned short rpm;                     // RPM 	2	u16 
    unsigned short car_signal;              // Signal 	2	u16
    unsigned short car_status;              // Status 	2	u16
    unsigned int gps_lat;                   // Latitude 	4	u32 
    unsigned int gps_lon;                   // Longitude 	4	u32 
    unsigned short gps_azimuth;             // Azimuth 	2	u16 
    unsigned char gps_status;               // GPS Status 	1	char
    unsigned char gps_speed;                // GPS Speed 	1	u8 
    short acceleration_x;                   // 가속도 Vx 	2	s16 
    short acceleration_y;                   // 가속도 Vy 	2	s16 
    unsigned char trip_cnt;                 // Trip count 	1	u8 
    unsigned char driver_no;                // Driver No 	1	u8 
    unsigned char modem_rssi;               // RSSI 	1	u8
    unsigned char reserved_1;               // reserved 	1	u8
    float  cur_distance;             // 주행거리 	4	float 
    float  day_distance;             // 일주행거리 	4	float 
    float  total_distance;           // 총주행거리 	4	float 
    float  cur_fuel_consumption;     // 연료소모량 	4	float 
    float  day_fuel_consumption;     // 일연료소모량 	4	float 
    float  total_fuel_consumption;   // 총연료소모량 	4	float 
    unsigned short batt_volt;               // Battery volt 	2	u16
    unsigned char  adas_aebs;               // AEBS	1	u8
    unsigned char  adas_ldw;                // LDW	1	u8
    unsigned short temp_1;                  // Temp1	2	s16
    unsigned short temp_2;                  // Temp2	2	s16
}__attribute__((packed))LILA_PKT_DATA__DATA_T; // Total 	64 bytes


typedef struct {
    unsigned short pkt_prefix;      // SPK	2	Binary 
    unsigned short dev_id;          // Device ID	2	u16
    unsigned int   phone_no;        // Identity No	4	u32 
    char           pkt_command[4];  // Command	4	ASCII 
    unsigned short idx;             // Index	2	u16
    unsigned short data_cnt;        // Count	2	u16
    unsigned int   data_len;        // Length	4	u32 
}__attribute__((packed))LILA_PKT_FRAME__HEADER_T; // 256 byte

typedef struct {
    unsigned short crc;          // CRC	2	Binary 
    unsigned short pkt_suffix;      // EPK	2	Binary 
}__attribute__((packed))LILA_PKT_FRAME__TAIL_T; // 256 byte

typedef struct {
    LILA_PKT_FRAME__HEADER_T header;          // CRC	2	Binary 
    unsigned char              data;
    LILA_PKT_FRAME__TAIL_T tail;      // EPK	2	Binary 
}__attribute__((packed))LILA_PKT_RESP__DTST_T; // 256 byte


typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{
	ePKT_TRANSFER_ID__DTG_INFO,
    ePKT_TRANSFER_ID__DTG_DATA,
};

//jwrho persistant data path modify++
//#define MILEAGE_FILE	"/data/mds/data/mdt800_bic_mileage.dat"
//#define MILEAGE_FILE	CONCAT_STR(USER_DATA_DIR, "/mdt800_bic_mileage.dat")
//jwrho persistant data path modify--

// ---------------------------------------------------
int lila_dtg__send_dtg_header();
int lila_dtg__send_dtg_data();

int lila_dtg__convert_dtg_data_to_pkt();
// ---------------------------------------------------
int lila_dtg__make_dtg_header(unsigned char **pbuf, unsigned short *packet_len);
int lila_dtg__make_dtg_data_dummy(unsigned char **pbuf, unsigned short *packet_len);

int lila_dtg__parse_dtg_header(LILA_PKT_RESP__DTST_T* resp);

int lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len);

#endif // __LILA_PKT_H__
=======
#ifndef __LILA_PKT_H__
#define __LILA_PKT_H__


//typedef enum {
//}e_PKT_AAA;

#define LILA_PKT_PREFIX     0xcfcc
#define LILA_PKT_SUFFIX     0xd0cf

#define LILA_PKT_ID__DEVICE_ID      0x03

#define LILA_PKT_ID__INFO               "INFO" // Device  -> Server
#define LILA_PKT_ID__DATA_SEND_CONFIRM  "CSTD" // "DTSC" // Server  -> Device
#define LILA_PKT_ID__DATA_SEND_SYNC     "DTSS" // "DTSS" // Server <-> Device

#define LILA_PKT_RET__FAIL      -1
#define LILA_PKT_RET__SUCCESS   1

typedef struct {
    char reserved_1[4];             // Reserved 	Binary 	4
    char reserved_2[7];             // Reserved 	Binary 	7
    char driver_name[10];           //운전자 이름 	ASCII 	10
    char driver_code[18];           // 운전자 코드 	ASCII 	18
    char car_regi_no[12];           // 자동차 등록번호 	Binary 	12
    char car_code[17];              // 차대번호 	ASCII 	17
    char company_name[12];          // 회사 이름 	ASCII 	12
    char company_regi_no_1[10];     // 사업자등록번호 	ASCII 	10
    char company_regi_no_2[10];     // 형식 승인 번호 	ASCII 	10
    char serial_no[14];             // 일련번호 	ASCII 	14
    char model_no[20];              // 모델번호 	ASCII 	20
    unsigned short k_factor;        // K-Factor 	decimal 	2
    unsigned short rpm_factor;      // RPM-Factor 	decimal 	2
    char reserved_3;                // Reserved 	Binary 	1
    char firmware_ver[6];           // 펌웨어버전 	Binary 	6
    char reserved_4[4];             // Reserved 	Binary 	4
    char reserved_5[38];            // Reserved 	Binary 	38
    char reserved_6[1];             // Reserved 	Binary 	1
    char reserved_7[4];             // Reserved 	Binary 	4
    char reserved_8[4];             // Reserved 	Binary 	4
    char reserved_9[4];             // Reserved 	Binary 	4
    char reserved_10[56];           // Reserved 	Binary 	56
}__attribute__((packed))LILA_PKT_DATA__DTG_INFO_T; // 256 byte

typedef struct {
    unsigned int  utc_time;                 // Time 	4	u32 
    unsigned char speed;                    // Speed 	1	u8 
    unsigned char speed_float;              // Speed Float 	1	u8 
    unsigned short rpm;                     // RPM 	2	u16 
    unsigned short car_signal;              // Signal 	2	u16
    unsigned short car_status;              // Status 	2	u16
    unsigned int gps_lat;                   // Latitude 	4	u32 
    unsigned int gps_lon;                   // Longitude 	4	u32 
    unsigned short gps_azimuth;             // Azimuth 	2	u16 
    unsigned char gps_status;               // GPS Status 	1	char
    unsigned char gps_speed;                // GPS Speed 	1	u8 
    short acceleration_x;                   // 가속도 Vx 	2	s16 
    short acceleration_y;                   // 가속도 Vy 	2	s16 
    unsigned char trip_cnt;                 // Trip count 	1	u8 
    unsigned char driver_no;                // Driver No 	1	u8 
    unsigned char modem_rssi;               // RSSI 	1	u8
    unsigned char reserved_1;               // reserved 	1	u8
    float  cur_distance;             // 주행거리 	4	float 
    float  day_distance;             // 일주행거리 	4	float 
    float  total_distance;           // 총주행거리 	4	float 
    float  cur_fuel_consumption;     // 연료소모량 	4	float 
    float  day_fuel_consumption;     // 일연료소모량 	4	float 
    float  total_fuel_consumption;   // 총연료소모량 	4	float 
    unsigned short batt_volt;               // Battery volt 	2	u16
    unsigned char  adas_aebs;               // AEBS	1	u8
    unsigned char  adas_ldw;                // LDW	1	u8
    unsigned short temp_1;                  // Temp1	2	s16
    unsigned short temp_2;                  // Temp2	2	s16
}__attribute__((packed))LILA_PKT_DATA__DATA_T; // Total 	64 bytes


typedef struct {
    unsigned short pkt_prefix;      // SPK	2	Binary 
    unsigned short dev_id;          // Device ID	2	u16
    unsigned int   phone_no;        // Identity No	4	u32 
    char           pkt_command[4];  // Command	4	ASCII 
    unsigned short idx;             // Index	2	u16
    unsigned short data_cnt;        // Count	2	u16
    unsigned int   data_len;        // Length	4	u32 
}__attribute__((packed))LILA_PKT_FRAME__HEADER_T; // 256 byte

typedef struct {
    unsigned short crc;          // CRC	2	Binary 
    unsigned short pkt_suffix;      // EPK	2	Binary 
}__attribute__((packed))LILA_PKT_FRAME__TAIL_T; // 256 byte

typedef struct {
    LILA_PKT_FRAME__HEADER_T header;          // CRC	2	Binary 
    unsigned char              data;
    LILA_PKT_FRAME__TAIL_T tail;      // EPK	2	Binary 
}__attribute__((packed))LILA_PKT_RESP__DTST_T; // 256 byte


typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{
	ePKT_TRANSFER_ID__DTG_INFO,
    ePKT_TRANSFER_ID__DTG_DATA,
};

//jwrho persistant data path modify++
//#define MILEAGE_FILE	"/data/mds/data/mdt800_bic_mileage.dat"
//#define MILEAGE_FILE	CONCAT_STR(USER_DATA_DIR, "/mdt800_bic_mileage.dat")
//jwrho persistant data path modify--

// ---------------------------------------------------
int lila_dtg__send_dtg_header();
int lila_dtg__send_dtg_data();

int lila_dtg__convert_dtg_data_to_pkt();
// ---------------------------------------------------
int lila_dtg__make_dtg_header(unsigned char **pbuf, unsigned short *packet_len);
int lila_dtg__make_dtg_data_dummy(unsigned char **pbuf, unsigned short *packet_len);

int lila_dtg__parse_dtg_header(LILA_PKT_RESP__DTST_T* resp);

int lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len);

#endif // __LILA_PKT_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

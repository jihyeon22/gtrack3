#ifndef PACKET_H
#define PACKET_H

#include <base/gpstool.h>

// return value...
typedef enum
{
	PACKET_RET_FAIL = -1,
	PACKET_RET_SUCCESS = 0,
	PACKET_RET_DONE = PACKET_RET_SUCCESS,
	PACKET_RET_MDT_PAKCET_FRAME_FULL,
	PACKET_RET_MDT_PAKCET_DATA_RANGE,
	PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE,
}PACKET_RET;


typedef enum
{
	PACKET_POWER_STATUS_OFF = 0,
	PACKET_POWER_STATUS_ON,
}PACKET_POWER_STATUS;


// ++++++++++++++++++++++++++++++++++++
// packet ID define
// ++++++++++++++++++++++++++++++++++++
typedef enum 
{
	STD_DTG_INFO_SEND = 0x01,
	STD_DTG_DATA_SEND = 0x03,
	KEY_OFF_DATA_SEND = 0x04,
	OBD_DATA_SEND = 0x06,
	MDT_DATA_SEND = 0x07,
	SERVER_RESP_RCV = 0x09,
}TRIPHOS_PROTOCOL_ID;






#define PACKET_DEFINE_TEL_NO_LEN			11
#define PACKET_DEFINE_TEMPER_SENSOR_NO		99
#define PACKET_DEFINE_MILEAGE_NO			0
// ------------------------------------------
//  COMMON_DEFINE
// ------------------------------------------
typedef struct {
	unsigned char time_YY;
	unsigned char time_MM;
	unsigned char time_DD;
	unsigned char time_hour;
	unsigned char time_min;
	unsigned char time_sec;
	unsigned char time_mil_sec;
}__attribute__((packed))TRIPHOS_COMMON_PACKET_TIME;

typedef struct {
	unsigned char   gps_speed;		// 000 ~ 255
	unsigned int    gps_longitude;	// maximum 9 digit : 127.123456 * 1000000 -> 127123456
	unsigned int    gps_latitude;	// maximum 9 digit : 37.527823 * 1000000 -> 37527823
	unsigned short  gps_azimuth;	// 0 ~ 360
}__attribute__((packed))TRIPHOS_COMMON_PACKET_GPS_MDT;



// ------------------------------------------
//   STD_DTG_INFO_SEND
// ------------------------------------------
typedef struct {
    unsigned char 	protocol_id;
    unsigned char 	tel_no[PACKET_DEFINE_TEL_NO_LEN];
	unsigned char 	dtg_model_no[20];		// 오른쪽 정렬, 빈칸은 '#' 으로 채운다.
	unsigned char	car_identity_no[17];	// 영문(대문자) 아라비아숫자 전부 표기
	
	// 11 : 시내버스 , 12 : 농어촌버스 , 13 : 마을버스 , 14 : 시외버스 , 15 : 고속버스 , 16 : 전세버스 , 17 : 특수여객자동차
	// 21 : 일반택시 , 22 : 개인택시
	// 31 : 일반화물자동차, 32 : 개별화물자동차
	// 41 : 비사업용자동차
	unsigned char	car_kind[2];			
			
	// 자동차 등록번호가 "서울 22 가 1234" 일경우..  만약 빈칸일경우 '#' 으로 모두 채운다.
	unsigned char	car_registration_no_1[2];	// "서" : 한글일경우 두자리차지
	unsigned char	car_registration_no_2[2];	// "울"
	unsigned char	car_registration_no_3[2];	// "22"
	unsigned char	car_registration_no_4[2];	// "가"
	unsigned char	car_registration_no_5[4];	// "1234"
	
	unsigned char	business_code[10]; 	// 사업자번호
	unsigned char	driver_code[18];	// 운전자 자격증번호, 빈칸은 '#'으로 채움, 중간 '-' 는 생략한다.
	
	unsigned char	dtg_firmware_ver[10];	// 빈칸은 '#' 으로 채움
}__attribute__((packed))TRIPHOS_DATA_PACKET__STD_DTG_INFO_SEND;


// ------------------------------------------
//   STD_DTG_DATA_SEND
// ------------------------------------------


typedef struct {
    unsigned char 	protocol_id;
    unsigned char 	tel_no[PACKET_DEFINE_TEL_NO_LEN];
	unsigned short 	send_interval_min;
	unsigned short 	body_cnt;
	TRIPHOS_COMMON_PACKET_TIME keyon_time;
}__attribute__((packed))TRIPHOS_HEAD_PACKET__STD_DTG_DATA_SEND;


typedef struct {
	unsigned short  dtg_mileage_day;	// 00 to 24 hour mileage
	unsigned int    dtg_mileage_total;	// car total 
	TRIPHOS_COMMON_PACKET_TIME cur_time;
	unsigned char   dtg_speed;			// 000 ~ 255
	unsigned short  dtg_rpm;			// 0000 ~ 9999
	unsigned char   dtg_break_signal;	// 0 (OFF) / 1 (ON)
	unsigned int    gps_longitude;		// maximum 9 digit : 127.123456 * 1000000 -> 127123456
	unsigned int    gps_latitude;		// maximum 9 digit : 37.527823 * 1000000 -> 37527823
	unsigned short  gps_azimuth;		// 0 ~ 360
	unsigned short	dtg_accel_sensor_x;		// 123m/sec^2 -> 12.3 * 10 // -100.0 ~ +100.0
	unsigned short	dtg_accel_sensor_y;		// 123m/sec^2 -> 12.3 * 10 // -100.0 ~ +100.0
	
	// DTG 상태를 올려준다. 1 BYTE
	// 00 : 운행기록장치 정상
	// 11 : gps 수신기이상 , 12 : 속도센서이상 , 13 : rpm 센서이상 , 14 : 브레이크센서 이상
	// 21 : 센서입력부장치이상 , 22 : 센서출력부장치이상 
	// 31 : 데이터출력부장치이상 , 32 : 통신장치이상
	// 41 : 운행거리산정이상 
	// 99 : 전원공급이상
	unsigned char	dtg_status;
	
	unsigned int	dtg_fuel_consumption_day; 		// 123.456 l *1000 = 123456 // 00 to 24 hour fuel consumption // 000.000 ~ 999.999
	unsigned int 	dtg_fuel_consumption_total;		// 123.456 l *1000 = 123456 // car total fuel consumption // 000000.000 ~ 999999.999
	unsigned short	dtg_fuel_remain;				// 0.0 % * 100 = 0	// 0 ~ 100 %
	
	unsigned short  temperature_A;		// xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off
	unsigned short  temperature_B;  	// -xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off
	
	unsigned short  mainpwr_volt; 		// xx.0 * 10 = xx0  // 0 ~ 99.9 
	
	unsigned short	dtg_kilo_per_liter;	// 10.0 km/L * 10 = 100	// 0 ~ 99.9 // 0 : nothing
	
	// 업무 코드
	// 0 : 업무보고기능 사용않함
	// 1 : 시작 , 2 : 종료 , 3 : 상차 , 4 : 하차, 5 : 출발(운행) , 6 : 도착 , 7 : 대기
	// 8 : 휴식 , 9 : 휴무 , 10 : 비상 , 11 : 정비 , 12 : 대차
	unsigned char	work_code;
}__attribute__((packed))TRIPHOS_DATA_PACKET__STD_DTG_DATA_SEND;


// ------------------------------------------
// OBD_DATA_SEND
// ------------------------------------------

typedef struct {
    unsigned char 	protocol_id;
    unsigned char 	tel_no[PACKET_DEFINE_TEL_NO_LEN];
	unsigned short 	send_interval_min;
	unsigned short 	body_cnt;
	TRIPHOS_COMMON_PACKET_TIME keyon_time;
}__attribute__((packed))TRIPHOS_HEAD_PACKET__OBD_DATA_SEND;


typedef struct {
	unsigned short  obd_mileage_day;	// 00 to 24 hour mileage
	unsigned int    obd_mileage_total;	// car total 
	TRIPHOS_COMMON_PACKET_TIME cur_time;
	unsigned char   obd_speed;			// 000 ~ 255
	unsigned short  obd_rpm;			// 0000 ~ 9999
	
	unsigned int    gps_longitude;		// maximum 9 digit : 127.123456 * 1000000 -> 127123456
	unsigned int    gps_latitude;		// maximum 9 digit : 37.527823 * 1000000 -> 37527823
	unsigned short  gps_azimuth;		// 0 ~ 360
	
	unsigned int	obd_fuel_consumption_day; 		// 123.456 l *1000 = 123456 // 00 to 24 hour fuel consumption // 000.000 ~ 999.999
	unsigned int 	obd_fuel_consumption_total;		// 123.456 l *1000 = 123456 // car total fuel consumption // 000000.000 ~ 999999.999
	
	unsigned short  temperature_A;		// xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off
	unsigned short  temperature_B;  	// -xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off
	
	unsigned short  mainpwr_volt; 		// xx.0 * 10 = xx0  // 0 ~ 99.9 
	
	unsigned short	obd_kilo_per_liter;	// 10.0 km/L * 10 = 100	// 0 ~ 99.9 // 0 : nothing
	
}__attribute__((packed))TRIPHOS_DATA_PACKET__OBD_DATA_SEND;


// ------------------------------------------
// MDT_DATA_SEND
// ------------------------------------------

typedef struct {
    unsigned char 	protocol_id;
    unsigned char 	tel_no[PACKET_DEFINE_TEL_NO_LEN];
	unsigned short 	send_interval_min;
	unsigned short 	body_cnt;
	TRIPHOS_COMMON_PACKET_TIME keyon_time;
}__attribute__((packed))TRIPHOS_HEAD_PACKET__MDT_KEYON_DATA;

typedef struct {
	unsigned short  mileage_day;	// 00 to 24 hour mileage
	unsigned int    mileage_total;	// car total 
	TRIPHOS_COMMON_PACKET_TIME 		cur_time;
	TRIPHOS_COMMON_PACKET_GPS_MDT	gps_data;
	unsigned short  temperature_A;	// xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off
	unsigned short  temperature_B;  // -xx.0 * 10 = xx0  // 99.0 : have no sensor  // 98.0 : turn off 
	unsigned short  mainpwr_volt; // xx.0 * 10 = xx0  // 0 ~ 99.9 

}__attribute__((packed))TRIPHOS_DATA_PACKET__MDT_KEYON_DATA;

// --------------------------------------------
//  KEY_OFF_DATA_SEND
// --------------------------------------------

typedef struct {
    unsigned char 	protocol_id;
    unsigned char 	tel_no[PACKET_DEFINE_TEL_NO_LEN];
	TRIPHOS_COMMON_PACKET_TIME cur_time;
	unsigned int    gps_longitude;  // maximum 9 digit : 127.123456 * 1000000 -> 127123456 
	unsigned int    gps_latitude;	// maximum 9 digit : 37.527823 * 1000000 -> 37527823
	TRIPHOS_COMMON_PACKET_TIME keyoff_time;
	// voltage add : (20141024) Protocol ver. 0.5
	unsigned short  mainpwr_volt; // xx.0 * 10 = xx0  // 0 ~ 99.9 
}__attribute__((packed))TRIPHOS_DATA_PACKET__MDT_KEYOFF_DATA;

// ------------------------------------------
//   SERVER_RESP_RCV
// ------------------------------------------

typedef struct {
	unsigned char protocol_id;
	unsigned char response;
}__attribute__((packed))TRIPHOS_RESP_PACKET__SERVER_RESP_RCV;

#define MDT_SERVER_STAT__SUCCESS			0
#define MDT_SERVER_STAT__ERR_PROTOCOL		1
#define MDT_SERVER_STAT__ERR_NO_REGI		2
#define MDT_SERVER_STAT__ERR_SERVER_CHECK	3
#define MDT_SERVER_STAT__ERR_SERVER_OVER	4

// -----------------------------------------
//  MDT packet Frame
// -----------------------------------------
#define MDT_DATA_PACKET__MAX_FRAME					600

#define MDT_DATA_PACKET__MAX_KEYON_DATA_PACKET		(60*4) // spare +5
#define MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET 	1

#define MDT_DATA_PACKET__STATUS_CLEAR				0x00
#define MDT_DATA_PACKET__STATUS_USED				0x00
#define MDT_DATA_PACKET__STATUS_PREPARE				0x02
#define MDT_DATA_PACKET__STATUS_PREPARE_COMPLETE	0x04
#define MDT_DATA_PACKET__STATUS_SEND_COMPLETE		0x08

typedef struct {
	TRIPHOS_HEAD_PACKET__MDT_KEYON_DATA header;
	TRIPHOS_DATA_PACKET__MDT_KEYON_DATA body[MDT_DATA_PACKET__MAX_KEYON_DATA_PACKET];
	unsigned int status;
	unsigned int body_cnt;
}__attribute__((packed))TRIPHOS_PACKET_FRAME__MDT_KEYON;


typedef struct {
	TRIPHOS_DATA_PACKET__MDT_KEYOFF_DATA body[MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET];
	unsigned int status;
	unsigned int body_cnt;	
	
}__attribute__((packed))TRIPHOS_PACKET_FRAME__MDT_KEYOFF;


typedef struct {
	unsigned int front;
	unsigned int rear;
	unsigned int total_use_cnt;
}__MANAGE_MDT_PACKET_FRAME;

#define MDT_PACKET_KEYON	0
#define MDT_PACKET_KEYOFF	1
#define MDT_PACKET_MAX		2


typedef struct PKT_CONTEXT {
    TRIPHOS_COMMON_PACKET_TIME keyon_time;
    TRIPHOS_COMMON_PACKET_TIME keyoff_time;
	char device_phone_num[PACKET_DEFINE_TEL_NO_LEN];
}PKT_CONTEXT_T;

int make_packet_keyon_data_done();
int make_packet_keyoff_data_done();
int send_keyon_data(const int pkt_count);
int send_keyoff_data(const int pkt_count);
int make_packet_keyon_data_insert(gpsData_t* pgpsdata);
int make_packet_keyoff_data_insert(gpsData_t* pgpsdata);
int get_packet_keyon_first(unsigned short *size, unsigned char **pbuf);
int get_packet_keyoff_first(unsigned short *size, unsigned char **pbuf);
int set_pkt_ctx_deivce_phone_num(char* phone_num, int size);
int set_pkt_ctx_keyon_time(int year, int month, int day, int hour, int min, int sec);
int check_packet_keyon_data_count();

void model_mds_make_poweron_packet(gpsData_t gpsdata);
void model_mds_send_poweron_packet();
void model_mds_make_poweroff_packet(gpsData_t gpsdata);
void model_mds_send_poweroff_packet();

void model_mds_check_server();

#endif /* PACKET_H */

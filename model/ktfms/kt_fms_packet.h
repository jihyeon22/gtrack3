#ifndef __KT_FMS_PACKET_H__
#define __KT_FMS_PACKET_H__

#include <base/gpstool.h>
#include <seco_obd.h>

#include "kt_fota_ver.h"

#include <board/board_system.h>
// -------------------------------
// default value
// -------------------------------
// CONCAT_STR(USER_DATA_DIR, "/mileage.dat")
#define SERVER_POLICY_SAVE_PATH		CONCAT_STR(USER_DATA_DIR, "/fms_policy.dat" )
#define SERVER_POLICY_SAVE_PATH2	CONCAT_STR(USER_DATA_DIR, "/fms_policy.dat.bak" )

#define CAR_DAILY_INFO				CONCAT_STR(USER_DATA_DIR, "/car_d_info.dat" )
#define CAR_DAILY_INFO2				CONCAT_STR(USER_DATA_DIR, "/car_d_info.dat.bak" )

#define CAR_TRIP_SEQ_PATH			CONCAT_STR(USER_DATA_DIR, "/car_trip_seq.dat" )
#define CAR_TRIP_SEQ_PATH2			CONCAT_STR(USER_DATA_DIR, "/car_trip_seq.dat.bak" )

#define CAR_OBD_INFO_PATH			CONCAT_STR(USER_DATA_DIR, "/car_obd_info.dat" )
#define CAR_OBD_INFO_PATH2			CONCAT_STR(USER_DATA_DIR, "/car_obd_info.dat.bak" )

#define KSMC_KEY_LAST_STAT_PATH		CONCAT_STR(USER_DATA_DIR, "/ksmc_last_key_stat.dat" )

#define DEFAULT_FMS_SDR_INIT	"A000|A001|A002|A020"
#define DEFAULT_FMS_SDR_ALIVE	"A000|A001|A002"
#define DEFAULT_FMS_SDR_RUN 	"A000|A001|A002|A020"

#define DEFAULT_FMS_MAX_CLI_CMD_CNT		5

// default server policy :: by kt fms spec
#define DEFAULT_FMS_INIT_COLLECT_INTERVAL		1
#define DEFAULT_FMS_INIT_SEND_INTERVAL			60


#define DEFAULT_FMS_SEND_INTERVAL			180		
#define DEFAULT_FMS_COLLECT_INTERVAL		1
#define DEFAULT_FMS_SEND_FAIL_RETRY_CNT		3		
#define DEFAULT_FMS_SEND_RETRY_DELAY_SEC	(3*60*10) 
#define DEFAILT_FMS_SEND_FAIL_REMOVE_CNT	2		
#define DEFAILT_FMS_SEND_FAIL_STOP_CNT		3

#define DEFAILT_FMS_SEND_FAIL_NORMAL_SLEEP_SEC	60
//#define DEFAILT_FMS_SEND_FAIL_NORMAL_SLEEP_SEC	30 // Test 시에는 30초로 변경

#define DEFAULT_FMS_SERVER_ADDR			"tfms.biz.olleh.com"
#define DEFAULT_FMS_SERVER_PORT			80
#define DEFAULT_FMS_SUB_SERVER_PATH		"TFM_CT/MDSMDSMDS/vid-rp"
#define DEFAULT_FMS_SUB_SERVER_PATH2	"TFM_CT/MDSMDSMDS/vid-rp"
#define DEFAULT_FMS_CAR_VIN				"MDSTEST9999999999"
#define DEFAULT_FMS_CAR_VIN2			"MDSTEST8888888888"
#define DEFAULT_FMS_CAR_VIN_NON_OBD		"MDSTEST7777777777"
#define DEFAULT_FMS_CAR_NUM				"MDSTEST99"
#define DEFAULT_FMS_CAR_NUM2			"MDSTEST88"
#define DEFAULT_FMS_CAR_NUM_NON_OBD		"MDSTEST77"
#define DEFAULT_FMS_DRIVER_ID			"9876543210"
#define DEFAULT_FMS_BUSINESS_NUM		"0123456789"

#define DEFAULT_FMS_DEV_MODEL			KT_FOTA_MODEL

#define SERVER_SEND_DEFAULT_INTERVAL	10

// ---------------------------------------
// return value
// ---------------------------------------
#define KT_FMS_API_RET_SUCCESS	0
#define KT_FMS_API_RET_FAIL		-1

// ----------------------------------
// variable type
// ----------------------------------
typedef enum {
	e_id_date = 0, 		// A000
	e_gps_x, 			// A001
	e_gps_y,			// A002
	e_mileage_day,		// A003
	e_mileage_total,	// A004
	e_speed,			// A005
	e_rpm,				// A006
	e_break_signal,		// A007
	e_gps_azimuth,		// A008
	e_acceleration_x,	// A009
	e_acceleration_y,	// A010
	e_fuel_consumption_day,		// A011
	e_fuel_consumption_total,	// A012
	e_fuel_efficiency,	// A013
	e_engine_oil_temp,	// A014
	e_fuel_injection,	// A015
	e_acceleration_pedal,	// A016
	e_gear_auto,	// A017
	e_gear_level,			// A018
	e_coolant_temp,	// A019
	e_key_stat,		// A020
	e_batt_volt,	// A021
	e_intake_temp,	// A022
	e_outtake_temp,	// A023
	e_maf_delta,	// A024 // mass_air_flow
	e_maf_total,	// A025 // mass_air_flow
	e_map,			// A026 // Manifold Absolute Pressure
	e_amp,			// A027 // 대기압
	e_cold_storage_temp,	// A028 // 내장온도
	e_forzen_storage_temp,	// A029 // 냉동온도
	e_remain_fuel,	// A030 // 연료잔량
	e_dct,			// A031 // DCT
	e_rssi,			// A032 // RSSI
	e_dev_stat,		// A033 // 장애모니터링
	e_ksmc_mode,    // A054 // 한국SMC공압 추가 // 1 : 업무용  // 0 : 비업무용
	e_END_OF_FACTOR_IDX,
	e_MAX_FACTOR_ID
} e_FACTOR_ID;


typedef enum {
	KT_FMS_SERVER_RET__SUCCESS = 200, 
	KT_FMS_SERVER_RET__NOT_REGI_DEV	= 403,
	KT_FMS_SERVER_RET__INVAILD_DATA_1 = 400,
	KT_FMS_SERVER_RET__NEED_MORE_DATA = 480,
	KT_FMS_SERVER_RET__INVAILD_DATA_2 = 481,
	KT_FMS_SERVER_RET__INVAILD_DATA_3 = 482,
	KT_FMS_SERVER_RET__SERVER_ERR_1	 = 500,
	KT_FMS_SERVER_RET__SERVER_ERR_2	 = 503,
	KT_FMS_SERVER_RET__SERVER_UNKOWN = 888,	
	KT_FMS_SERVER_RET__SERVER_NO_RESP = 999
} e_FMS_SERVER_RET;


typedef enum {
	KT_FMS_SEND_POLICY__NONE = 0 , 
	KT_FMS_SEND_POLICY__INIT_EVENT = 1 ,
	KT_FMS_SEND_POLICY__PWR_ON_EVENT = 2 ,
	KT_FMS_SEND_POLICY__PWR_OFF_EVENT = 3 ,
	//KT_FMS_SEND_POLICY__KEEP_ALIVE = 4 ,	// not support
	KT_FMS_SEND_POLICY__RUNNING  = 5,
	KT_FMS_SEND_POLICY__SERVER_FAIL_STOP = 6
} e_FMS_SEND_POLICY;

typedef enum {
	e_FMS_SVR_POLICY_STAT_NONE = 0,
	e_FMS_SVR_POLICY_STAT_SUCCESS,
	e_FMS_SVR_POLICY_STAT_RUNNING,
	e_FMS_SVR_POLICY_STAT_COMPLETE,
} e_FMS_SVR_STAT;


typedef enum {
	e_not_recv = 0,
	e_recv_setting_val,
} e_SERVER_PRAM_STAT;

typedef enum {
	e_CLI_CMD_NONE = 0,
	e_CLI_CMD_CMD_RECV,
	e_CLI_CMD_RUN_SUCCESS,
} e_CLI_CMD_STAT;


typedef enum {
	e_CLI_RET_NONE = 0,
	e_CLI_RET_FAIL = 'F',
	e_CLI_RET_RUNNING = 'P',
	e_CLI_RET_SUCCESS = 'S',
} e_CLI_RET_VAL;


typedef enum {
	e_HW_ERR_CODE_OBD_CONN_DISCONN = 00,
	e_HW_ERR_CODE_OBD_CONN_DATA_ERR = 11,
	e_HW_ERR_CODE_DATA_DISCON = 31,
	e_HW_ERR_CODE_GPS_ANT_DISCONN = 41,
	e_HW_ERR_CODE_GPS_INVAILD = 51,
} e_HW_ERR_CODE_VAL;

typedef struct factor_define
{
	int e_factor_idx;
	int e_factor_size;
	char* factor_name;
}factor_define_t;

typedef struct fms_car_info
{
	unsigned char user_business_no[13];	// 4. 사업자번호 : 13
	unsigned char car_vin[17]; 			// 5. 차대번호 : 17
	unsigned char car_no[12];	// 6. 차량번호 : 20
	unsigned char driver_id[20];		// 7. 운전자 아이디 : 20
}fms_car_info_t;

typedef struct cli_cmd_mgr
{
	e_CLI_CMD_STAT cmd_stat;
	char cmd_result;
	int cli_id; 		// 각 명령어는 id 가 있다.
	char cli_cmd[6];	// x.x.x 최대 5개 문자열
	char cli_arg[64];	// 가변형이다. 하지만 64를 넘어가지 않을것으로 보인다.
}cli_cmd_mgr_t;

// 일일 운행정보는 다음과 같이 계산한다.
// obd 에서는 총 운행거리, 연료소모량이 나오므로..
//  => 총 운행거리 - 일일운행거리 offset = 일일운행거리
//  => 총 연료소모량 - 일일소모량 offset = 일일연료소모량
typedef struct daliy_car_info
{
	int yyyymmdd;	// 날짜 바뀔때, 최초 저장
	long long trip;		// 일일 운행거리 측정을 위한 offset
	int fuel; 		// 일일 연료소모량 측정을 위한 offset
}daliy_car_info_t;

typedef struct server_fail_policy
{
	// 재전송횟수
	//  - 전송실패시 "pkt_send_fail_retry_cnt" 회 만큼 반복한다.
	//  - "m" 회만큼 반복해서 연속 실패 하면, 재전송지연시간 만큼 1회 쉬어야 .. 해야 폐기 기준횟수 1회다.
	int pkt_send_fail_retry_cnt;
	
	// 폐기 기준횟수
	//  - 재전송 횟수만큼 "pkt_send_fail_remove_cnt" 회 반복후에도 실패시에는 해당 패킷은 폐기한다.
	int pkt_send_fail_remove_cnt; 
	
	// 재전송지연시간
	//  - 재전송횟수(pkt_send_fail_retry_cnt) 만큼 시도시에도 실패하면 "pkt_send_retry_delay_sec" 만큼 쉬어야한다.
	int pkt_send_retry_delay_sec; 
	
	// 전송중단 기준 횟수
	//  - 재전송 횟수(pkt_send_retry_delay_sec) 만큼 시도후, 재전송시연시간(_pkt_retry_delay_sec) 만큼 쉬면 1회 실패다.
	//  - 총 1회실패 x _fail_send_stop_cnt 만큼 했는데도 전송성공 못하면, 전송을 아예 멈춘다.
	//  - 최대 전송 시도 횟수
	int pkt_send_fail_stop_cnt;
}server_fail_policy_t;


typedef struct server_policy
{
	int policy_stat;
	
	int policy_num;
	
	server_fail_policy_t runtime_fail_policy;

	
	int pkt_send_interval_sec;
	int pkt_collect_interval_sec;
	
	// sdr factor 1 개당 5byte
	//  ==> 1024 / 5 = 205 개의 sdr factor list 저장가능
	//  ==> 현재 스팩은 총 48개의 sdr factor 
	//  이곳에 저장되어있는 sdr factor 는 running 상태의 factor 이다.
	int sdr_factor[e_MAX_FACTOR_ID];
}server_policy_t;

typedef struct last_dev_stat
{
	int svr_ret_code;
	int obd_stat;
	int obd_key;
	int obd_rpm;
	int obd_speed;
	long long last_trip;
	int last_set_gender_spec_ret;
	int last_set_gender_err_code;
}last_dev_stat_t;
extern last_dev_stat_t g_last_dev_stat;

// ------------------------------------------------
// function
// ------------------------------------------------
int init_server_policy();

// sdr factor util
int convert_factor_id_to_str(char* buff, const int sdr_factor[e_MAX_FACTOR_ID] );
int convert_factor_str_to_id(const char* buff, int (*sdr_factor)[e_MAX_FACTOR_ID] );

// pkt make util
int make_sdr_body(char* input_buff, gpsData_t* p_gpsdata, obdData_t* p_obddata, const int sdr_factor[e_MAX_FACTOR_ID]);
int make_sdr_body_null(char* input_buff, gpsData_t* p_gpsdata, obdData_t* p_obddata, const int sdr_factor[e_MAX_FACTOR_ID]);
int make_sdr_header(char* input_buff, int body_size, const int sdr_factor[e_MAX_FACTOR_ID], int policy_num, e_FMS_SEND_POLICY send_policy);

// server return proc
int set_server_stat_from_return_str(const char* ret_buff);

// server send interval api
int get_server_send_interval();
int set_server_send_interval(int sec);
int set_server_send_interval_default();

// server change function
int chk_change_server();
int set_change_server(int flag);

// init and reboot
int init_server_routine();
int init_server_and_poweroff();
int init_server_and_poweroff2();

// set to obd trip 
int proc_set_trip();
int req_trip_setting(int req_trip);
int get_req_trip_setting_result();

// server policy
int init_fms_server_policy();
int clr_fms_server_policy();
int get_server_policy(server_policy_t* p_srv_policy);
int set_server_policy(server_policy_t* p_srv_policy);

int get_runtime_server_policy(server_policy_t* policy);
int set_runtime_server_policy();

// send policy
e_FMS_SEND_POLICY get_send_policy();
e_FMS_SEND_POLICY set_send_policy(e_FMS_SEND_POLICY cur_policy);

int get_cur_sdr_factor(int (*sdr_factor)[e_MAX_FACTOR_ID]);
//void set_cur_sdr_factor_id(const int sdr_factor[e_MAX_FACTOR_ID]);
//void set_cur_sdr_factor_str(const char* buff);

// trip seq ..
int init_trip_seq();
int get_trip_seq(char* buff);
void set_trip_seq(int year, int mon, int day, int hour, int min, int sec);
int clear_trip_seq();

// obd info
int load_obd_info();

// daliy trip info
int init_car_daily_info();

int clr_daily_info();
long long get_daliy_trip(long long cur_trip);
int get_daliy_fuel(int cur_fuel);
int save_car_daliy_info(int yymmdd, long long trip, int fuel);
int get_cur_daily_date_num();

// obd info..
int save_obd_info(odbGender_t* p_gender_spec);

// get car info
int get_fms_car_info(fms_car_info_t* car_info);

// cli cmd proc 
int run_cli_cmd();
int get_cli_cmd_stat(char* buff);
int get_cli_cmd_reset_stat();

// error code..
int set_hw_err_code(e_HW_ERR_CODE_VAL code, int flag);
int init_hw_err_code();
int get_hw_err_code(char* buff);

int set_last_obd_stat(int stat);
int get_last_obd_stat();

// ksmc model
#define SMC_MODE__COMPANY	1
#define SMC_MODE__PRIVATE	0
#define SMC_MODE__DEFAULT	SMC_MODE__PRIVATE


int init_ksmc_mode();
int set_ksmc_mode(int mode);
int get_ksmc_mode();


#endif





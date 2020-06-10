#ifndef __SECO_OBD_H__
#define __SECO_OBD_H__

#define MAX_OBD_UART_INIT_TRY_CNT	3

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_INVALID_COND	-999


#define OBD_ERROR_CODE__DEFAULT_ERR				0
#define OBD_ERROR_CODE__PACKET_TYPE_ERR			1
#define OBD_ERROR_CODE__UNKOWN_CMD				2
#define OBD_ERROR_CODE__PKT_CHECKSUM_ERR		3
#define OBD_ERROR_CODE__UNSUPPORT_FUNC			4
#define OBD_ERROR_CODE__NOT_INIT_VECHICLE		100
#define OBD_ERROR_CODE__NOT_INIT_OBD			101
#define OBD_ERROR_CODE__DETECT_RESTART			102
#define OBD_ERROR_CODE__FIRM_UP_FAIL_CAR_ON		200

#define OBD_ERROR_CODE__NOT_TRANS_DATA_ADDR		300
#define OBD_ERROR_CODE__NOT_TRANS_DATA			400
#define OBD_ERROR_CODE__FLASH_ERASE				500
#define OBD_ERROR_CODE__SET_GENDER_SPEC			600
#define OBD_ERROR_CODE__GET_GENDER_SPEC			700
#define OBD_ERROR_CODE__GET_TIME				800
#define OBD_ERROR_CODE__GET_DTC_CODE			900

#define OBD_ERROR_CODE__SUCCESS					-99
#define OBD_ERROR_CODE__UNKOWN_CODE				-1
#define OBD_ERROR_CODE__NOT_VAILD_CMD_RET		-2
#define OBD_ERROR_CODE__UART_READ_TIMEOUT		-3
#define OBD_ERROR_CODE__NO_DATA_RET				-4

#define MAX_RET_BUFF_SIZE 1024



typedef struct {
	int obd_read_stat;
	int obd_daily_info_init;
	long long car_mileage_total;	// (8) 지속적인 증가 8 byte => shift 3
	unsigned int car_speed;					// (4) x.xxx => shift 3
	unsigned int car_rpm;					// (2) x => shift 0
	unsigned int car_break_signal;			// (1) x => shift 0 / On=1, Off=0
	unsigned int car_fuel_consumption_total;	// (4) x.xxx => shift 3 
	unsigned int car_fuel_efficiency;		// (2) x.x => shift 1
	unsigned int car_engine_oil_temp;		// (3) x.x => shift 1 
	unsigned int car_fuel_injection;			// (4) x.xxx => shift 3
	unsigned int car_accel_pedal;			// (1) x => shift 0 / ~~%
	unsigned char car_gear_auto;	// x => shfit 0 / R,P,N,D
	unsigned int car_gear_level;				// x => shfit 0 / 0 ~ 10 // 기어단수
	unsigned int car_coolant_temp;	// (3) x.x => shfit 1
	unsigned int car_key_stat;		// (1) x => shfit 0 /  On=1, Off=0
	unsigned int car_batt_volt;		// (2) x.x => shift 1
	unsigned int car_intake_temp;	// (3) x.x => shfit 1 
	unsigned int car_outtake_temp;	// (3) x.x => shfit 1 
	unsigned int car_maf_delta;		// (2) x => shfit 0;
	unsigned int car_maf_total;
	unsigned int car_map;			
	unsigned int car_amp;			// x.x => shfit 1
	unsigned int car_remain_fuel_percent;	// (1) x => shfit 0;
	unsigned int car_engine_torque;	// x.x => shfit 1
	unsigned int car_air_gauge_volt;	// x.x => shfit 1
	unsigned int car_fuel_gauge_volt;	// x.x => shfit 1
	unsigned char car_dct[60];
}obdData_t;

typedef struct {
	unsigned char gender_sn[8+1];	// (8)
	unsigned int gender_sw;		// (2)
	
	unsigned char gender_car_num[12+1]; // 차량번호 => size 12 완성형 : 생략
	unsigned char gender_car_vin[17+1]; // 차대번호 => size 17 ascii : 생략
	// 차종 => size 8 완성형 : 생략
	// 유종 => size 1 hex : 생략
	// 년식 => size 2 hex : 생략
	// 배기량 => size 2 hex : 생략
	// 엔진기통수 => size 1 hex : 생략
	// BT MAC size => 12 ascii : 생략
	long long gender_total_trip; // 누적거리 => size 8 hex : 생략
	long long gender_total_fuel;// 누적연료소모량 => size 8 hex : 생략
	unsigned int gender_is_detach; //탈착여부 => size 1 hex : 생략
	unsigned int gender_is_detach_cond;// 탈착 cond => size 1 hex : 생략
	// 연도 => size 2 hex: 생략
	// 월 => size 1 hex: 생략
	// 일 => size 1 hex: 생략
	// 요일 => size 1 hex: 생략
	// 오전/오후 => size 1 hex: 생략
	// 시간 => size 1 hex : 생략
	// 분 => size 1 hex : 생략
	// 초 => size 1 hex : 생략
}odbGender_t;

typedef struct {
	unsigned char gender_sn[8+1];
	
	long long first_read_trip;
	long long setting_target_trip;
	long long trip_offset;
	
	long long first_read_fuel;
	long long setting_target_fuel;
	long long fuel_offset;
}trip_fuel_calc_t;

typedef struct {
	unsigned char car_dct[60];	// (60)
}odbDcdcode_t;

int req_obd_data(obdData_t* p_obdData);

int get_obd_gender_spec(odbGender_t* p_obdGender);
int set_obd_gender_spec(odbGender_t* p_obdGender);

// set gender spec
int set_seco_obd_car_info(char* car_num, char* car_vin);
int set_seco_obd_total_trip(long long trip);
int set_seco_obd_total_fuel(long long fuel);
int set_seco_obd_total_trip_fuel(long long trip, long long fuel);
int set_seco_obd_car_vin(char* car_vin);
int set_seco_obd_car_num(char* car_num);

int get_obd_dct_code(odbDcdcode_t* p_odbDcdcode);

int req_firmware_update_data(); 	// not support
int transfer_not_recv_data_addr(); 	// not support
int transfer_not_recv_data();		// not support
int req_flash_erase();				// not support
int set_obd_time();					// not support
int get_obd_dct_code(odbDcdcode_t* p_odbDcdcode);

int req_obd_ext_pwr_line_off();

// ---------------------------------------
int req_obd_data_fake(obdData_t* p_obdData);

int get_obd_gender_spec_fake(odbGender_t* p_obdGender);
int set_obd_gender_spec_fake(odbGender_t* p_obdGender);

// set gender spec
int set_seco_obd_car_info_fake(char* car_num, char* car_vin);
int set_seco_obd_total_trip_fake(long long trip);
int set_seco_obd_total_fuel_fake(long long fuel);
int set_seco_obd_total_trip_fuel_fake(long long trip, long long fuel);
int set_seco_obd_car_vin_fake(char* car_vin);
int set_seco_obd_car_num_fake(char* car_num);

int get_obd_dct_code_fake(odbDcdcode_t* p_odbDcdcode);

int req_firmware_update_data_fake(); 	// not support
int transfer_not_recv_data_addr_fake(); 	// not support
int transfer_not_recv_data_fake();		// not support
int req_flash_erase_fake();				// not support
int set_obd_time_fake();					// not support
int get_obd_dct_code_fake(odbDcdcode_t* p_odbDcdcode);

int req_obd_ext_pwr_line_off_fake();



void dbg_print_obd_data(obdData_t* p_obdGender);
void dbg_print_gender_spec(odbGender_t* p_obdGender);

#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <board/board_system.h>
#include "logd/logd_rpc.h"

#include "seco_obd.h"
#include "seco_obd_util.h"

#define DEBUG_AS_CMD

#ifdef DEBUG_AS_CMD
#include "kt_fms_packet.h"
#endif

#define CHK_BATT_INTERVAL_SEC 20
#define LOG_TARGET eSVC_MODEL
int get_obd_gender_spec_hex(char buff[92]);
// *********************************************************
// 1. 데이터 요청
// *********************************************************
int req_obd_data_fake(obdData_t* p_obdData)
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	static int chk_batt_interval = 0;
	static int last_batt_level = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	unsigned char* tmp_buff = ret_buff + 0;
	unsigned int hex_read_size = 0;
	
	gpsData_t gpsdata = {0,};
	gps_get_curr_data(&gpsdata);
	
	// -----------------------------------------
	// 운행거리 => size 8;
	// -----------------------------------------
	{
		// TODO: 누적거리 단위확인
		
		p_obdData->car_mileage_total = mileage_get_m();
		LOGD(LOG_TARGET, " >> fake obd : mileage_total is [%lld]\r\n",p_obdData->car_mileage_total);

	}
	
	// -------------------------------------------
	// speed  => size 4
	// -------------------------------------------
	{
		p_obdData->car_speed = gpsdata.speed;
	}
	
	// -------------------------------------------
	// 누적 연료소모량 => size 4
	// -------------------------------------------
	{
		p_obdData->car_fuel_consumption_total = 0;
	}
	
	// ------------------------------------------
	// 트립연비 => size 2
	// ------------------------------------------
	{
		p_obdData->car_fuel_efficiency =  0xffffffff;
	}
	
	// ------------------------------------------
	// 순간연료분사량 => size 4
	// ------------------------------------------
	{
		p_obdData->car_fuel_injection =  0xffffffff;
	}
	
	// ------------------------------------
	// rpm => size 2
	// ------------------------------------
	{
		p_obdData->car_rpm =  0x0;
	}
	
	// ------------------------------------
	// 브레이크 신호 => size  1
	// ------------------------------------
	{
		p_obdData->car_break_signal =  0xffffffff;
	}
	
	// ------------------------------------
	// key 신호 => size 1
	// ------------------------------------
	{
		if ( power_get_ignition_status() == POWER_IGNITION_ON )
			p_obdData->car_key_stat = 1;
		else
			p_obdData->car_key_stat = 0;
	}
	
	// ---------------------------------------
	// 변속레버 => size 1 // R,P,N,D
	// ---------------------------------------
	{
		p_obdData->car_gear_auto = 0xffffffff;
	}
	
	// ----------------------------------------
	// 변속단 => size 1 // 0~10
	// ----------------------------------------
	{
		p_obdData->car_gear_level = 0xffffffff;
	}
	
	// --------------------------------------------------
	// 가속페달 => size 1 / ~~%
	// --------------------------------------------------
	{
		p_obdData->car_accel_pedal = 0xffffffff;
	}
	
	// -------------------------------
	// 엔진오일온도 => size 3
	// -------------------------------
	{
		p_obdData->car_engine_oil_temp = 0xffffffff;
	}
	
	// -------------------------------
	// 흡기온도 => size 3	
	// -------------------------------
	{
		p_obdData->car_intake_temp = 0xffffffff;
	}
	
	// -------------------------------
	// 외기온도 => size 3
	// -------------------------------
	{
		p_obdData->car_outtake_temp = 0xffffffff;
	}
	
	// -------------------------------
	// 냉각수온도 => size 3
	// -------------------------------
	{
		p_obdData->car_coolant_temp = 0xffffffff;
	}
	
	// -------------------------------
	// maf : 2
	// -------------------------------
	{
		p_obdData->car_maf_delta = 0xffffffff;
	}
	// ------------------------------
	// amp : 2
	// ------------------------------
	{
		p_obdData->car_amp = 0xffffffff;
	}
	
	// ------------------------------
	// 엔진토크 => 4
	// ------------------------------
	{
		p_obdData->car_engine_torque = 0xffffffff;
	}
	
	// -----------------------------------------
	// 배터리전압 : 2
	// -----------------------------------------
	{
		// TODO: 단위 확인할것 / 
		// TODO: 너무 자주 확인하지 않게 할것
		if ( ( chk_batt_interval++ % CHK_BATT_INTERVAL_SEC ) == 0 )
		{
			p_obdData->car_batt_volt = (battery_get_battlevel_car()/100);
			last_batt_level = p_obdData->car_batt_volt;
		}
		else
			p_obdData->car_batt_volt = last_batt_level;
	}
	
	// ----------------------------------------------
	// 에어게이지전압 : 2
	// ----------------------------------------------
	{
		p_obdData->car_air_gauge_volt = 0xffffffff;
	}
	
	// ----------------------------------------------
	// 연료게이지전압 : 2
	// ----------------------------------------------
	{
		p_obdData->car_fuel_gauge_volt = 0xffffffff;
	}
	
	// ----------------------------------------------
	// 연료잔량 :1
	// ----------------------------------------------
	{
		p_obdData->car_remain_fuel_percent = 0xffffffff;
	}
	
	
	//printf("%s() : end \r\n",__func__);
	p_obdData->obd_read_stat = OBD_RET_SUCCESS;
	
	return OBD_RET_SUCCESS;
}

// 2.1 Firmware Update Sequence Request
int req_firmware_update_seq_fake()
{	
	return OBD_RET_FAIL;
}

// 2.2 Firmware Update Data
int req_firmware_update_data_fake()
{	
	return OBD_RET_FAIL;
}


// 3. 미전송 Data Address 전송
int transfer_not_recv_data_addr_fake()
{
	return OBD_RET_FAIL;
}

//4. 미전송 Data 전송
int transfer_not_recv_data_fake()
{		
	return OBD_RET_FAIL;
}

// 5. Flash Erase

int req_flash_erase_fake()
{
	return OBD_RET_FAIL;	
}

// 7. Get Gender SPEC.
int get_obd_gender_spec_fake(odbGender_t* p_obdGender)
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	unsigned char* tmp_buff = ret_buff + 0;
	unsigned int hex_read_size = 0;
	
	printf("%s() : start ++ \r\n",__func__);
		
	// ---------------------------------------------
	// 젠더시리얼 => size 8; // ascii
	// ---------------------------------------------
	{
		memset(p_obdGender->gender_sn, 0x00, 8);
		strncpy(p_obdGender->gender_sn, "00000000", 8);
	}
	
	// ---------------------------------------------
	// 젠더 sw ver => size 2  // hex
	// ---------------------------------------------
	{
		p_obdGender->gender_sw = 123456;
	}
	
	// ---------------------------------------------
	// 차량번호 => size 12; // 완성형 :: 생략
	// ---------------------------------------------
	{
		char tmp_car_no[32] = {0,};
		memset(p_obdGender->gender_car_num, 0x00, 12);

		if ( get_car_info_car_no(tmp_car_no) == 0 )
			strncpy(p_obdGender->gender_car_num, tmp_car_no, strlen(tmp_car_no));
		else
			strncpy(p_obdGender->gender_car_num, DEFAULT_FMS_CAR_NUM_NON_OBD, strlen(DEFAULT_FMS_CAR_NUM_NON_OBD));
		//printf("car_num is [%s]\r\n",car_num);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 차대번호 => size 17; // ascii :: 생략
	// ---------------------------------------------
	{
		char tmp_car_vin[32] = {0,};
		memset(p_obdGender->gender_car_vin, 0x00, 17);

		if ( get_car_info_car_vin(tmp_car_vin) == 0 )
			strncpy(p_obdGender->gender_car_vin, tmp_car_vin, strlen(tmp_car_vin));
		else
			strncpy(p_obdGender->gender_car_vin, DEFAULT_FMS_CAR_VIN_NON_OBD, strlen(DEFAULT_FMS_CAR_VIN_NON_OBD));
	}
	
	// ---------------------------------------------
	// 차종 => size 8; // 완성형 : 생략
	// ---------------------------------------------
	{
		//unsigned char car_name[8] = {0,};

	}
	
	// ---------------------------------------------
	// 유종 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int fuel_type = 0;
	}
	
	// ---------------------------------------------
	// 년식 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_year = 0;
	}
	
	// ---------------------------------------------
	// 배기량 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_cc = 0;
	}
	
	// ---------------------------------------------
	// 엔진기통수 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_engine_type = 0;
	}
	
	// ---------------------------------------------
	// BT MAC => size 12; // ascii : 생략
	// ---------------------------------------------
	{
		//unsigned char bt_mac [12] = {0,};
	}
	
	// ---------------------------------------------
	// 누적거리 => size 8; // hex : 생략
	// ---------------------------------------------
	{
		// 누적거리 단위확인 : meter 단위
		p_obdGender->gender_total_trip = mileage_get_m();
	}
	
	// ---------------------------------------------
	// 누적연료소모량 => size 8; // hex : 생략
	// ---------------------------------------------
	{
		p_obdGender->gender_total_fuel = 0;
	}
	
	// ---------------------------------------------
	// 탈부착여부 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		p_obdGender->gender_is_detach = 0;
	}
	
	// ---------------------------------------------
	// 탈착 cond => size 1; // hex : 생략
	// ---------------------------------------------
	{
		p_obdGender->gender_is_detach_cond = 0;
	}
	
	// ---------------------------------------------
	// 연도 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_year = 0;
	}
	
	// ---------------------------------------------
	// 월 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_month = 0;
	}	
	
	// ---------------------------------------------
	// 일 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_day = 0;
	}	
	
	// ---------------------------------------------
	// 요일 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_day_of_week = 0;
	}	
	
	// ---------------------------------------------
	// 오전오후 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_am_pm = 0;
	}	
	
	// ---------------------------------------------
	// 시간  => size 1;  // hex  : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_hour = 0;
	}	
	
	// ---------------------------------------------
	// 분 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_min = 0;
	}	
	
	// ---------------------------------------------
	// 초 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_sec = 0;
	}	

	printf("%s() : end \r\n",__func__);
	
	return OBD_RET_SUCCESS;
}




int set_obd_gender_spec_fake(odbGender_t* p_obdGender)
{
	return OBD_RET_SUCCESS;
}


int get_obd_gender_spec_hex_fake(char buff[92])
{
	// TODO: remove this api
	return OBD_RET_SUCCESS;
}

int set_seco_obd_total_trip_fuel_fake(long long trip, long long fuel)
{
	return OBD_RET_SUCCESS;
}


int set_seco_obd_total_trip_fake(long long trip)
{
	mileage_set_m(trip);
	mileage_write();
	return OBD_RET_SUCCESS;
}

int set_seco_obd_car_vin_fake(char* car_vin)
{
	return OBD_RET_SUCCESS;
}

int set_seco_obd_car_num_fake(char* car_num)
{
	return OBD_RET_SUCCESS;
}

int set_seco_obd_total_fuel_fake(long long fuel)
{
	return OBD_RET_SUCCESS;
}


int set_seco_obd_car_info_fake(char* car_num, char* car_vin)
{
	return OBD_RET_SUCCESS;
}


// 8. Set Time.
int set_obd_time_fake()
{
	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
}


// 9. Get DTC(Diagnostic Trouble Code) code
int get_obd_dct_code_fake(odbDcdcode_t* p_odbDcdcode)
{	
	sprintf(p_odbDcdcode->car_dct, "%.60s","");
	return OBD_RET_SUCCESS;
}

// 10. Power Off Request
int req_obd_ext_pwr_line_off_fake()
{	
	return OBD_RET_SUCCESS;
}

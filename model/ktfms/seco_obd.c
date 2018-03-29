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


#include "seco_obd.h"
#include "seco_obd_util.h"

#define DEBUG_AS_CMD

#ifdef DEBUG_AS_CMD
#include "kt_fms_packet.h"
#endif

int get_obd_gender_spec_hex(char buff[92]);
// *********************************************************
// 1. 데이터 요청
// *********************************************************
int req_obd_data(obdData_t* p_obdData)
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	unsigned char* tmp_buff = ret_buff + 0;
	unsigned int hex_read_size = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return req_obd_data_fake(p_obdData);
	//printf("%s() : start ++ \r\n",__func__);
	
	// obd uart 가 닫혀있으면, 다시 연다.
	// 여러번 열기를 시도하며, 열기 fail 일경우 fail 로 리턴
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	if (is_sec_obd_init() == OBD_RET_FAIL)
	{
		p_obdData->obd_read_stat = OBD_RET_FAIL;
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	// 정상적으로 uart 가 열린지 확인했다면, uart 에서 데이터를 읽는다.
	read_cnt = seco_obd_write_cmd_resp("<AC", NULL, 0, ret_buff, &error_code);

	if (read_cnt <= 0)
	{
		//printf("cmd is error code is [%d]\r\n", error_code);
		// 만약에 uart 에서 응답이 없을경우 uart 를 닫는다.
		// 다음에 시도시 다시 자동으로 열것이니..
		if ( error_code == OBD_ERROR_CODE__UART_READ_TIMEOUT )
		{
			printf("obd uart write fail.. close uartch\r\n");
			seco_obd_deinit();
			p_obdData->obd_read_stat = OBD_RET_FAIL;
			return OBD_CMD_RET_TIMEOUT;
		}
		
		p_obdData->obd_read_stat = OBD_RET_FAIL;
		return OBD_RET_FAIL;
	}
	
	// debug code..
	/*
	printf("%s() return ----------------\r\n", __func__);
	for(i = 0 ; i < read_cnt ; i++ )
	{	
		if ((i > 0) && ((i%10)==0))
			printf("\r\n");
		printf("[0x%02x]",ret_buff[i]);
	}
	printf("\r\n-----------------------------\r\n");
	*/
	
	// 길이 확인
	if (read_cnt != 64 )
	{
		printf("%s() - %d: error return size is wrong!\r\n",__func__, __LINE__);
		p_obdData->obd_read_stat = OBD_RET_FAIL;
		return OBD_RET_FAIL;
	}
	
	// -----------------------------------------
	// 운행거리 => size 8;
	// -----------------------------------------
	{
		long long mileage_total = 0;
		hex_read_size = 8;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			mileage_total += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		// printf("mileage_total is [%lld]\r\n",mileage_total);
		p_obdData->car_mileage_total = mileage_total;
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------------------
	// speed  => size 4
	// -------------------------------------------
	{
		unsigned int speed = 0;
		hex_read_size = 4;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			speed += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		// printf("speed is [%d]\r\n",speed);
		p_obdData->car_speed = speed;
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------------------
	// 누적 연료소모량 => size 4
	// -------------------------------------------
	{
		unsigned int fuel_consumption_total = 0;
		hex_read_size = 4;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_consumption_total += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_fuel_consumption_total = fuel_consumption_total;
		
		printf("fuel_consumption_total is [%d]\r\n",fuel_consumption_total);
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------------------
	// 트립연비 => size 2
	// ------------------------------------------
	{
		unsigned int fuel_efficiency = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_efficiency += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_fuel_efficiency = fuel_efficiency;
		//printf("fuel_efficiency is [%d]\r\n",fuel_efficiency);
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------------------
	// 순간연료분사량 => size 4
	// ------------------------------------------
	{
		unsigned int fuel_injection = 0;
		hex_read_size = 4;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_injection+= (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_fuel_injection = fuel_injection;
		//printf("fuel_injection is [%d]\r\n",fuel_injection);
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------------
	// rpm => size 2
	// ------------------------------------
	{
		unsigned int rpm = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			rpm += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_rpm = rpm;
		// printf("rpm is [%d]\r\n",rpm);
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------------
	// 브레이크 신호 => size  1
	// ------------------------------------
	{
		unsigned int break_signal = 0;
		hex_read_size = 1;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			break_signal += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_break_signal = break_signal;
		//printf("break_signal is [%d]\r\n",break_signal);
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------------
	// key 신호 => size 1
	// ------------------------------------
	{
		unsigned int key_signal = 0;
		hex_read_size = 1;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			key_signal += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		p_obdData->car_key_stat = key_signal;
		// printf("key_signal is [%d]\r\n",key_signal);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------
	// 변속레버 => size 1 // R,P,N,D
	// ---------------------------------------
	{
		char gear_auto = 0;
		hex_read_size = 1;
		
		gear_auto = tmp_buff[i];
		p_obdData->car_gear_auto = gear_auto;
		//printf("gear_auto is [%c]\r\n",gear_auto);
		
		tmp_buff += hex_read_size;
	}
	
	// ----------------------------------------
	// 변속단 => size 1 // 0~10
	// ----------------------------------------
	{
		unsigned int gear_level = 0;
		hex_read_size = 1;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			gear_level += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		//printf("gear_level is [%d]\r\n",gear_level);
		
		p_obdData->car_gear_level = gear_level;
		
		tmp_buff += hex_read_size;
	}
	
	// --------------------------------------------------
	// 가속페달 => size 1 / ~~%
	// --------------------------------------------------
	{
		unsigned int accel = 0;
		hex_read_size = 1;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			accel += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		//printf("accel is [%d]\r\n",accel);
		p_obdData->car_accel_pedal = accel;
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------
	// 엔진오일온도 => size 3
	// -------------------------------
	{
		unsigned int engine_oil_temp = 0;
		hex_read_size = 3;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			engine_oil_temp += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		p_obdData->car_engine_oil_temp = engine_oil_temp;
		
		//printf("engine_oil_temp is [%d]\r\n",engine_oil_temp);
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------
	// 흡기온도 => size 3	
	// -------------------------------
	{
		unsigned int intake_temp = 0;
		hex_read_size = 3;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			intake_temp += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_intake_temp = intake_temp;
		//printf("intake_temp is [%d]\r\n",intake_temp);
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------
	// 외기온도 => size 3
	// -------------------------------
	{
		unsigned int outtake_temp = 0;
		hex_read_size = 3;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			outtake_temp += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_outtake_temp = outtake_temp;
		//printf("outtake_temp is [%d]\r\n",outtake_temp);
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------
	// 냉각수온도 => size 3
	// -------------------------------
	{
		unsigned int coolant_temp = 0;
		hex_read_size = 3;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			coolant_temp += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_coolant_temp = coolant_temp;
		//printf("coolant_temp is [%d]\r\n",coolant_temp);
		
		tmp_buff += hex_read_size;
	}
	
	// -------------------------------
	// maf : 2
	// -------------------------------
	{
		unsigned int maf_delta = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			maf_delta += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		p_obdData->car_maf_delta = maf_delta;
		//printf("map is [%d]\r\n",map);
		
		tmp_buff += hex_read_size;
	}
	// ------------------------------
	// amp : 2
	// ------------------------------
	{
		unsigned int amp = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			amp += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		//printf("amp is [%d]\r\n",amp);
		p_obdData->car_amp = amp;
		
		tmp_buff += hex_read_size;
	}
	
	// ------------------------------
	// 엔진토크 => 4
	// ------------------------------
	{
		unsigned int engine_toque = 0;
		hex_read_size = 4;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			engine_toque += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		p_obdData->car_engine_torque = engine_toque;
		//printf("engine_tok is [%d]\r\n",engine_tok);
		
		tmp_buff += hex_read_size;
	}
	
	// -----------------------------------------
	// 배터리전압 : 2
	// -----------------------------------------
	{
		unsigned int batt_volt = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			batt_volt += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		//printf("batt_volt is [%d]\r\n",batt_volt);
		p_obdData->car_batt_volt = batt_volt;
		
		tmp_buff += hex_read_size;
	}
	
	// ----------------------------------------------
	// 에어게이지전압 : 2
	// ----------------------------------------------
	{
		unsigned int air_gauge_volt = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			air_gauge_volt += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		p_obdData->car_air_gauge_volt = air_gauge_volt;
		//printf("air_gage_volt is [%d]\r\n",air_gage_volt);
		
		tmp_buff += hex_read_size;
	}
	
	// ----------------------------------------------
	// 연료게이지전압 : 2
	// ----------------------------------------------
	{
		unsigned int fuel_gauge_volt = 0;
		hex_read_size = 2;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_gauge_volt += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		//printf("fuel_gage_volt is [%d]\r\n",fuel_gage_volt);
		p_obdData->car_fuel_gauge_volt = fuel_gauge_volt;
		
		tmp_buff += hex_read_size;
	}
	
	// ----------------------------------------------
	// 연료잔량 :1
	// ----------------------------------------------
	{
		unsigned int fuel_percent = 0;
		hex_read_size = 1;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_percent += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		//printf("fuel_percent is [%d]\r\n",fuel_percent);
		
		p_obdData->car_remain_fuel_percent = fuel_percent;
		
		tmp_buff += hex_read_size;
	}
	
	
	//printf("%s() : end \r\n",__func__);
	p_obdData->obd_read_stat = OBD_RET_SUCCESS;
	
	return OBD_RET_SUCCESS;
}

// 2.1 Firmware Update Sequence Request
int req_firmware_update_seq()
{	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return req_firmware_update_seq_fake();

	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
	
}

// 2.2 Firmware Update Data
int req_firmware_update_data()
{	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return req_firmware_update_data_fake();

	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
	
}

// 3. 미전송 Data Address 전송
int transfer_not_recv_data_addr()
{	
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int i = 0;
	int error_code = 0;
	
	int read_cnt = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return transfer_not_recv_data_addr_fake();

	printf("%s() : start ++ \r\n",__func__);
	
	read_cnt = seco_obd_write_cmd_resp("<AA", NULL, 0, ret_buff, &error_code);

	if (read_cnt > 0)
		printf("cmd is success :: [%d]\r\n", read_cnt);
	else
		printf("cmd is error code is [%d]\r\n", error_code);
	
	
	// debug code..
	printf("%s() return ----------------\r\n", __func__);
	
	for(i = 0 ; i < read_cnt ; i++ )
	{	
		if ((i > 0) && ((i%10)==0))
			printf("\r\n");
		printf("[0x%02x]",ret_buff[i]);
	}
	printf("\r\n-----------------------------\r\n");
	
	printf("%s() : end \r\n",__func__);
	
	return OBD_RET_FAIL;
	
}

//4. 미전송 Data 전송
int transfer_not_recv_data()
{	
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int i = 0;
	int error_code = 0;
	int read_cnt = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return transfer_not_recv_data_fake();

	printf("%s() : start ++ \r\n",__func__);
	
	read_cnt = seco_obd_write_cmd_resp("<AL", NULL, 0, ret_buff, &error_code);

	if (read_cnt > 0)
		printf("cmd is success :: [%d]\r\n", read_cnt);
	else
		printf("cmd is error code is [%d]\r\n", error_code);
	
	
	// debug code..
	printf("%s() return ----------------\r\n", __func__);
	
	for(i = 0 ; i < read_cnt ; i++ )
	{	
		if ((i > 0) && ((i%10)==0))
			printf("\r\n");
		printf("[0x%02x]",ret_buff[i]);
	}
	printf("\r\n-----------------------------\r\n");
	
	printf("%s() : end \r\n",__func__);
	
	return OBD_RET_FAIL;
}

// 5. Flash Erase

int req_flash_erase()
{
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return req_flash_erase_fake();

	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
}

// 6. Set Gender SPEC.
/*
int set_gender_spec()
{
	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
}
*/

// 7. Get Gender SPEC.
int get_obd_gender_spec(odbGender_t* p_obdGender)
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	unsigned char* tmp_buff = ret_buff + 0;
	unsigned int hex_read_size = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return get_obd_gender_spec_fake(p_obdGender);

	printf("%s() : start ++ \r\n",__func__);
	
	// obd uart 가 닫혀있으면, 다시 연다.
	// 여러번 열기를 시도하며, 열기 fail 일경우 fail 로 리턴
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	if (is_sec_obd_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	// 정상적으로 uart 가 열린지 확인했다면, uart 에서 데이터를 읽는다.
	read_cnt = seco_obd_write_cmd_resp("<AG", NULL, 0, ret_buff, &error_code);

	if (read_cnt <= 0)
	{
		printf("cmd is error code is [%d]\r\n", error_code);
		// 만약에 uart 에서 응답이 없을경우 uart 를 닫는다.
		// 다음에 시도시 다시 자동으로 열것이니..
		if ( error_code == OBD_ERROR_CODE__UART_READ_TIMEOUT )
		{
			printf("obd uart write fail.. close uartch\r\n");
			seco_obd_deinit();
			return OBD_CMD_RET_TIMEOUT;
		}
		
		return OBD_RET_FAIL;
	}
	
	// debug code..
	/*
	printf("%s() return ----------------\r\n", __func__);
	for(i = 0 ; i < read_cnt ; i++ )
	{	
		if ((i > 0) && ((i%10)==0))
			printf("\r\n");
		printf("[0x%02x]",ret_buff[i]);
	}
	printf("\r\n-----------------------------\r\n");
	*/
	
	// 길이 확인
	if (read_cnt != 92 )
	{
		printf("%s() - %d: error return size is wrong!\r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
	}
		
	// ---------------------------------------------
	// 젠더시리얼 => size 8; // ascii
	// ---------------------------------------------
	{
		hex_read_size = 8;
		memset(p_obdGender->gender_sn, 0x00, 8);
		
		strncpy(p_obdGender->gender_sn, tmp_buff, hex_read_size);
			
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 젠더 sw ver => size 2  // hex
	// ---------------------------------------------
	{
		hex_read_size = 2;
		p_obdGender->gender_sw = 0;
		
		for (i = 0 ; i < hex_read_size ; i++)
			p_obdGender->gender_sw += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 차량번호 => size 12; // 완성형 :: 생략
	// ---------------------------------------------
	{
		hex_read_size = 12;
		memset(p_obdGender->gender_car_num, 0x00, 12);
		
		strncpy(p_obdGender->gender_car_num, tmp_buff, hex_read_size);
		//printf("car_num is [%s]\r\n",car_num);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 차대번호 => size 17; // ascii :: 생략
	// ---------------------------------------------
	{
		hex_read_size = 17;
		memset(p_obdGender->gender_car_vin, 0x00, 17);
		
		strncpy(p_obdGender->gender_car_vin, tmp_buff, hex_read_size);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 차종 => size 8; // 완성형 : 생략
	// ---------------------------------------------
	{
		//unsigned char car_name[8] = {0,};
		hex_read_size = 8;
		
		//strncpy(car_name, tmp_buff, hex_read_size);
		//printf("car_name is [%s]\r\n",car_name);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 유종 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int fuel_type = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	fuel_type += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("fuel_type is [%d]\r\n",fuel_type);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 년식 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_year = 0;
		hex_read_size = 2;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	car_year += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("car_year is [%d]\r\n",car_year);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 배기량 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_cc = 0;
		hex_read_size = 2;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	car_cc += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("car_cc is [%d]\r\n",car_cc);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 엔진기통수 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int car_engine_type = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	car_engine_type += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("car_engine_type is [%d]\r\n",car_engine_type);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// BT MAC => size 12; // ascii : 생략
	// ---------------------------------------------
	{
		//unsigned char bt_mac [12] = {0,};
		hex_read_size = 12;
		
		//strncpy(bt_mac, tmp_buff, hex_read_size);
		//printf("bt_mac is [%s]\r\n",bt_mac);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 누적거리 => size 8; // hex : 생략
	// ---------------------------------------------
	{
		hex_read_size = 8;
		p_obdGender->gender_total_trip = 0;
		
		for (i = 0 ; i < hex_read_size ; i++)
			p_obdGender->gender_total_trip += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 누적연료소모량 => size 8; // hex : 생략
	// ---------------------------------------------
	{
		hex_read_size = 8;
		p_obdGender->gender_total_fuel = 0;
		
		for (i = 0 ; i < hex_read_size ; i++)
			p_obdGender->gender_total_fuel += (tmp_buff[i] << (8*(hex_read_size-i-1)));
	
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 탈부착여부 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		hex_read_size = 1;
		p_obdGender->gender_is_detach = 0;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			p_obdGender->gender_is_detach += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		}
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 탈착 cond => size 1; // hex : 생략
	// ---------------------------------------------
	{
		hex_read_size = 1;
		p_obdGender->gender_is_detach_cond = 0;
		
		for (i = 0 ; i < hex_read_size ; i++)
			p_obdGender->gender_is_detach_cond += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 연도 => size 2; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_year = 0;
		hex_read_size = 2;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_year += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_year is [%d]\r\n",obd_year);
		
		tmp_buff += hex_read_size;
	}
	
	// ---------------------------------------------
	// 월 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_month = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_month += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_month is [%d]\r\n",obd_month);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 일 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_day = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_day += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_day is [%d]\r\n",obd_day);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 요일 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_day_of_week = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_day_of_week += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_day_of_week is [%d]\r\n",obd_day_of_week);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 오전오후 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_am_pm = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_am_pm += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_am_pm is [%d]\r\n",obd_am_pm);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 시간  => size 1;  // hex  : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_hour = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_hour += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_hour is [%d]\r\n",obd_hour);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 분 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_min = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_min += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_min is [%d]\r\n",obd_min);
		
		tmp_buff += hex_read_size;
	}	
	
	// ---------------------------------------------
	// 초 => size 1; // hex : 생략
	// ---------------------------------------------
	{
		//unsigned int obd_sec = 0;
		hex_read_size = 1;
		
		//for (i = 0 ; i < hex_read_size ; i++)
		//{
		//	obd_sec += (tmp_buff[i] << (8*(hex_read_size-i-1)));
		//}
		//printf("obd_sec is [%d]\r\n",obd_sec);
		
		tmp_buff += hex_read_size;
	}	

	printf("%s() : end \r\n",__func__);
	
	return OBD_RET_SUCCESS;
}




int set_obd_gender_spec(odbGender_t* p_obdGender)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	if ( p_obdGender == NULL )
		return OBD_RET_FAIL;
	
	// 기존데이터를 갖고온다.
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12 
	// -------------------------
	clear_size = 12;
	
	{
		unsigned char tmp_car_num[17] = {0,};
	
		memcpy(&tmp_car_num, p_obdGender->gender_car_num, strlen(p_obdGender->gender_car_num));
		//printf("car num -2 is [%d]\r\n",tmp_car_num);
		for ( i = 0 ; i < clear_size ; i++)
		{
			hex_ret_buff[i+idx] = tmp_car_num[i];
		}
		
	}
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17 
	// -------------------------
	clear_size = 17;
	
	{
		unsigned char tmp_car_vin[17] = {0,};
		
		memcpy(&tmp_car_vin, p_obdGender->gender_car_vin, strlen(p_obdGender->gender_car_vin));
		
		for ( i = 0 ; i < clear_size ; i++)
			hex_ret_buff[i+idx] = tmp_car_vin[i];
	}
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&p_obdGender->gender_total_trip, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&p_obdGender->gender_total_fuel, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1 => 모두 초기화
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}


int get_obd_gender_spec_hex(char buff[92])
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	// non obd model..
	if ( get_use_obd_device() == 0 )
		return get_obd_gender_spec_hex_fake(NULL);

//	unsigned char* p_ret_buff = ret_buff + 0;
//	unsigned char* p_target_buff = buff + 0;
//	unsigned int hex_read_size = 0;
	
//	int trip_idx = 0;
//	int fuel_idx = 0;
	
	printf("%s() : start ++ \r\n",__func__);
	
	// obd uart 가 닫혀있으면, 다시 연다.
	// 여러번 열기를 시도하며, 열기 fail 일경우 fail 로 리턴
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	if (is_sec_obd_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	// 정상적으로 uart 가 열린지 확인했다면, uart 에서 데이터를 읽는다.
	read_cnt = seco_obd_write_cmd_resp("<AG", NULL, 0, ret_buff, &error_code);

	if (read_cnt <= 0)
	{
		printf("cmd is error code is [%d]\r\n", error_code);
		// 만약에 uart 에서 응답이 없을경우 uart 를 닫는다.
		// 다음에 시도시 다시 자동으로 열것이니..
		if ( error_code == OBD_ERROR_CODE__UART_READ_TIMEOUT )
		{
			printf("obd uart write fail.. close uartch\r\n");
			seco_obd_deinit();
			return OBD_CMD_RET_TIMEOUT;
		}
		
		return OBD_RET_FAIL;
	}
	
	// 길이 확인
	if (read_cnt != 92 )
	{
		printf("%s() - %d: error return size is wrong!\r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
	}
	
	memcpy(buff, ret_buff, 92);
	printf("%s() : success! \r\n",__func__);
	
	return OBD_RET_SUCCESS;
}

int set_seco_obd_total_trip_fuel(long long trip, long long fuel)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_total_trip_fuel_fake(trip,fuel);

	printf("[%lld] / [%lld] \r\n", trip, fuel);

	//long long trip = set_trip * 1000;
	//long long fuel = set_fuel;
	
	if ( trip < 0 )
		return OBD_RET_FAIL;

	if ( fuel < 0 )
		return OBD_RET_FAIL;
	
	// 기존데이터를 갖고온다.
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
#ifdef DEBUG_AS_CMD
		g_last_dev_stat.last_set_gender_spec_ret = 1;
#endif
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12 => 기존데이터 유지
	// -------------------------
	clear_size = 12;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17 => 기존데이터 유지
	// -------------------------
	clear_size = 17;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1 => 모두 초기화
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
#ifdef DEBUG_AS_CMD
		g_last_dev_stat.last_set_gender_spec_ret = 2;
#endif
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
#ifdef DEBUG_AS_CMD
		g_last_dev_stat.last_set_gender_spec_ret = 3;
#endif
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}


int set_seco_obd_total_trip(long long trip)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;

	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	long long fuel_consumption_total = 0;
	
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_total_trip_fake(0);

	if ( trip < 0 )
		return OBD_RET_FAIL;

	// 기존데이터를 갖고온다.
	
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12 ==> 기존유지
	// -------------------------
	clear_size = 12;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17 ==> 기존유지
	// -------------------------
	clear_size = 17;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;

	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8 ==> 기존유지
	// -------------------------
	clear_size = 8;
	
	// 누적연료소모량은 읽은 그대로 쓰면 안된다 (/ 1000) 한다음에 써야한다.
	// 데이터를 숫자형으로 변경한다. 변경후에는 /1000 한다.
	{	
		int hex_read_size = 8;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_consumption_total += (hex_ret_buff[i+idx] << (8*(hex_read_size-i-1)));
		}
		fuel_consumption_total = fuel_consumption_total / 1000;
	}
		
	
	// 숫자형 fuel  데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel_consumption_total, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}






int set_seco_obd_car_vin(char* car_vin)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	long long fuel_consumption_total = 0;
	
	//printf("[%lld] / [%lld] \r\n", trip, fuel);
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_car_vin_fake(NULL);

	//long long trip = set_trip * 1000;
	//long long fuel = set_fuel;
	
	if ( car_vin == NULL )
		return OBD_RET_FAIL;

	if ( strlen(car_vin) >= 18)
		return OBD_RET_FAIL;
	// 기존데이터를 갖고온다.
	
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12
	// -------------------------
	clear_size = 12;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17
	// -------------------------
	clear_size = 17;
	
	{
		unsigned char tmp_car_vin[17] = {0,};
		
		memcpy(&tmp_car_vin, car_vin, strlen(car_vin));
		
		for ( i = 0 ; i < clear_size ; i++)
			hex_ret_buff[i+idx] = tmp_car_vin[i];
	}
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8 ==> 기존유지
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	//memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8 ==> 기존유지
	// -------------------------
	clear_size = 8;
	
	// 누적연료소모량은 읽은 그대로 쓰면 안된다 / 1000 한다음에 써야한다./
	// 데이터를 int 형으로 변경한다. 변경후에는 /1000 한다.
	{	
		int hex_read_size = 8;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_consumption_total += (hex_ret_buff[i+idx] << (8*(hex_read_size-i-1)));
		}
		fuel_consumption_total = fuel_consumption_total / 1000;
	}
		
	
	// int 형 fuel  데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel_consumption_total, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1 ==> 모두초기화
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}


int set_seco_obd_car_num(char* car_num)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	long long fuel_consumption_total = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_car_num_fake(NULL);
	//printf("[%lld] / [%lld] \r\n", trip, fuel);

	//long long trip = set_trip * 1000;
	//long long fuel = set_fuel;
	
	//if ( trip < 0 )
	//	return OBD_RET_FAIL;

	// 기존데이터를 갖고온다.
	
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12
	// -------------------------
	clear_size = 12;
	//printf("car num -1 is [%d]\r\n",car_num);
	{
		unsigned char tmp_car_num[17] = {0,};
	
		memcpy(&tmp_car_num, car_num, strlen(car_num));
		//printf("car num -2 is [%d]\r\n",tmp_car_num);
		for ( i = 0 ; i < clear_size ; i++)
		{
			hex_ret_buff[i+idx] = tmp_car_num[i];
		}
		
	}
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17
	// -------------------------
	clear_size = 17;
	
	/*
	{
		unsigned char tmp_car_vin[17] = {0,};
	
		memcpy(&tmp_car_vin, car_vin, strlen(car_vin));
		
		for ( i = 0 ; i < clear_size ; i++)
		{
			hex_ret_buff[i+idx] = tmp_car_vin[i];
		}
		
	}
	*/
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	//memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8
	// -------------------------
	clear_size = 8;
	
	// 누적연료소모량은 읽은 그대로 쓰면 안된다 / 1000 한다음에 써야한다./
	// 데이터를 int 형으로 변경한다. 변경후에는 /1000 한다.
	{	
		int hex_read_size = 8;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_consumption_total += (hex_ret_buff[i+idx] << (8*(hex_read_size-i-1)));
		}
		fuel_consumption_total = fuel_consumption_total / 1000;
	}
		
	
	// int 형 fuel  데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel_consumption_total, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}

int set_seco_obd_total_fuel(long long fuel)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_total_fuel_fake(0);

	if ( fuel < 0 )
		return OBD_RET_FAIL;
	
	// 기존데이터를 갖고온다.
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12 => 기존데이터 유지
	// -------------------------
	clear_size = 12;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17 => 기존데이터 유지
	// -------------------------
	clear_size = 17;
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8
	// -------------------------
	clear_size = 8;
	
	/*
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	*/
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1 => 모두 초기화
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}


int set_seco_obd_car_info(char* car_num, char* car_vin)
{
	unsigned char hex_ret_buff[92] = {0,00};
	
	unsigned char tmp_buff[8] = {0,};
	unsigned char ret_buff[128] ={0,};
	
	int i = 0;
	int idx = 0;
	int clear_size = 0;
	int read_cnt = 0;
	
	int error_code = 0;
	
	int ret = OBD_RET_FAIL;
	
	long long fuel_consumption_total = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return set_seco_obd_car_info_fake(NULL,NULL);

	//printf("[%lld] / [%lld] \r\n", trip, fuel);

	//long long trip = set_trip * 1000;
	//long long fuel = set_fuel;
	
	if ( car_vin == NULL )
		return OBD_RET_FAIL;

	if ( strlen(car_vin) >= 18)
		return OBD_RET_FAIL;
	// 기존데이터를 갖고온다.
	
	if ( get_obd_gender_spec_hex(hex_ret_buff) != OBD_RET_SUCCESS )
	{
		printf("%s() get gender spec hex fail...\r\n",__func__);
		return OBD_RET_FAIL;
	}
	
	// 각종 field 를 clear 한다.
	// -------------------------
	// 1. 젠더 시리얼 : 8 
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 2. 젠더 swver : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 3. 차량번호 : 12
	// -------------------------
	clear_size = 12;
	
		//printf("car num -1 is [%d]\r\n",car_num);
	{
		unsigned char tmp_car_num[17] = {0,};
	
		memcpy(&tmp_car_num, car_num, strlen(car_num));
		//printf("car num -2 is [%d]\r\n",tmp_car_num);
		for ( i = 0 ; i < clear_size ; i++)
		{
			hex_ret_buff[i+idx] = tmp_car_num[i];
		}
		
	}
	
	idx += clear_size;
	// -------------------------
	// 4. 차대번호 : 17
	// -------------------------
	clear_size = 17;
	
	{
		unsigned char tmp_car_vin[17] = {0,};
		
		memcpy(&tmp_car_vin, car_vin, strlen(car_vin));
		
		for ( i = 0 ; i < clear_size ; i++)
			hex_ret_buff[i+idx] = tmp_car_vin[i];
	}
	
	idx += clear_size;
	// -------------------------
	// 5. 차종 : 8
	// -------------------------
	clear_size = 8;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 6. 유종 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	// -------------------------
	// 7. 년식 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 8. 배기량 : 2
	// -------------------------
	clear_size = 2;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	// -------------------------
	// 9. 기통수 : 1
	// -------------------------
	clear_size = 1;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 10. BT MAC : 12
	// -------------------------
	clear_size = 12;
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// -------------------------
	// 11. 누적주행거리 : 8 ==> 기존유지
	// -------------------------
	clear_size = 8;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	//memcpy(&tmp_buff, (void*)&trip, sizeof(tmp_buff) );
	
	//for ( i = 0 ; i < clear_size ; i++)
	//	hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 12. 누적연료소모량 : 8 ==> 기존유지
	// -------------------------
	clear_size = 8;
	
	// 누적연료소모량은 읽은 그대로 쓰면 안된다 / 1000 한다음에 써야한다./
	// 데이터를 int 형으로 변경한다. 변경후에는 /1000 한다.
	{	
		int hex_read_size = 8;
		
		for (i = 0 ; i < hex_read_size ; i++)
		{
			fuel_consumption_total += (hex_ret_buff[i+idx] << (8*(hex_read_size-i-1)));
		}
		fuel_consumption_total = fuel_consumption_total / 1000;
	}
		
	
	// int 형 fuel  데이터를 char 8 byte 로 변환한다.
	memcpy(&tmp_buff, (void*)&fuel_consumption_total, sizeof(tmp_buff) );
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = tmp_buff[8-i-1];
	
	idx += clear_size;
	
	// -------------------------
	// 13. 탈착유무 ~ 20 시간 : 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1 ==> 모두초기화
	// -------------------------
	clear_size = 1 + 1 + 2 + 1 + 1 + 1  + 1  + 1;
	
	// 입력받은 trip 데이터를 char 8 byte 로 변환한다.
	
	for ( i = 0 ; i < clear_size ; i++)
		hex_ret_buff[i+idx] = 0x00;
	
	idx += clear_size;
	
	
	// trip 조작한데이터를 다시 쓴다.
	read_cnt = seco_obd_write_cmd_resp("<AS", hex_ret_buff, 92, ret_buff, &error_code);
	
	printf(" error_code = [%d], read_cnt = [%d]\r\n", error_code, read_cnt);
	
	if (( read_cnt == 1 ) && (ret_buff[0] = 0x01))
	{
		printf("%s() set success \r\n", __func__);
		ret = OBD_RET_SUCCESS;
	}
	else
	{
		printf("%s() set fail \r\n", __func__);
		ret = OBD_RET_FAIL;
	}
	
	return ret;
}




// 8. Set Time.
int set_obd_time()
{
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return set_obd_time_fake();

	printf("%s() : start ++ \r\n",__func__);	

	printf(" not implement yet \r\n");
	printf("%s() : end \r\n",__func__);
	return OBD_RET_FAIL;
}

// 9. Get DTC(Diagnostic Trouble Code) code
int get_obd_dct_code(odbDcdcode_t* p_odbDcdcode)
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	// non obd model..
	if ( get_use_obd_device() == 0 )
		return get_obd_dct_code_fake(p_odbDcdcode);

	//unsigned char* tmp_buff = ret_buff + 0;
	//unsigned int hex_read_size = 0;
	
	printf("%s() : start ++ \r\n",__func__);
	
	// obd uart 가 닫혀있으면, 다시 연다.
	// 여러번 열기를 시도하며, 열기 fail 일경우 fail 로 리턴
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	if (is_sec_obd_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	// 정상적으로 uart 가 열린지 확인했다면, uart 에서 데이터를 읽는다.
	read_cnt = seco_obd_write_cmd_resp("<AD", NULL, 0, ret_buff, &error_code);

	if (read_cnt <= 0)
	{
		printf("cmd is error code is [%d]\r\n", error_code);
		// 만약에 uart 에서 응답이 없을경우 uart 를 닫는다.
		// 다음에 시도시 다시 자동으로 열것이니..
		if ( error_code == OBD_ERROR_CODE__UART_READ_TIMEOUT )
		{
			printf("obd uart write fail.. close uartch\r\n");
			seco_obd_deinit();
			return OBD_CMD_RET_TIMEOUT;
		}
		
		return OBD_RET_FAIL;
	}
	
	sprintf(p_odbDcdcode->car_dct, "%.60s",ret_buff);
	
	return OBD_RET_SUCCESS;
}

// 10. Power Off Request
int req_obd_ext_pwr_line_off()
{
	unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int i = 0;
	
	int error_code = 0;
	int read_cnt = 0;

	//unsigned char* tmp_buff = ret_buff + 0;
	//unsigned int hex_read_size = 0;
	
	// non obd model..
	if ( get_use_obd_device() == 0 )
		return req_obd_ext_pwr_line_off_fake();

	printf("%s() : start ++ \r\n",__func__);
	
	// obd uart 가 닫혀있으면, 다시 연다.
	// 여러번 열기를 시도하며, 열기 fail 일경우 fail 로 리턴
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	if (is_sec_obd_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	// 정상적으로 uart 가 열린지 확인했다면, uart 에서 데이터를 읽는다.
	read_cnt = seco_obd_write_cmd_resp("<AP", NULL, 0, ret_buff, &error_code);

	if (read_cnt <= 0)
	{
		printf("cmd is error code is [%d]\r\n", error_code);
		// 만약에 uart 에서 응답이 없을경우 uart 를 닫는다.
		// 다음에 시도시 다시 자동으로 열것이니..
		if ( error_code == OBD_ERROR_CODE__UART_READ_TIMEOUT )
		{
			printf("obd uart write fail.. close uartch\r\n");
			seco_obd_deinit();
			return OBD_CMD_RET_TIMEOUT;
		}
		
		return OBD_RET_FAIL;
	}
	
	// debug code..
	/*
	printf("%s() return ----------------\r\n", __func__);
	
	for(i = 0 ; i < read_cnt ; i++ )
	{	
		if ((i > 0) && ((i%10)==0))
			printf("\r\n");
		printf("[0x%02x]",ret_buff[i]);
	}
	printf("\r\n-----------------------------\r\n");
	*/
	printf("%s() : end \r\n",__func__);
	
	return OBD_RET_SUCCESS;
}


void dbg_print_obd_data(obdData_t* p_obdData)
{
	printf("------ obd data start --------------\r\n");
	printf(" - car_mileage_total [%lld]\r\n", p_obdData->car_mileage_total);
	printf(" - car_speed [%d]\r\n", p_obdData->car_speed);
	printf(" - car_rpm [%d]\r\n", p_obdData->car_rpm);
	printf(" - car_break_signal [%d]\r\n", p_obdData->car_break_signal);
	printf(" - car_fuel_consumption_total [%d]\r\n", p_obdData->car_fuel_consumption_total);
	printf(" - car_fuel_efficiency [%d]\r\n", p_obdData->car_fuel_efficiency);
	printf(" - car_engine_oil_temp [%d]\r\n", p_obdData->car_engine_oil_temp);
	printf(" - car_fuel_injection [%d]\r\n", p_obdData->car_fuel_injection);
	printf(" - car_accel_pedal [%d]\r\n", p_obdData->car_accel_pedal);
	printf(" - car_gear_auto [%x]\r\n", p_obdData->car_gear_auto);
	printf(" - car_gear_level [%d]\r\n", p_obdData->car_gear_level);
	printf(" - car_coolant_temp [%d]\r\n", p_obdData->car_coolant_temp);
	printf(" - car_key_stat [%d]\r\n", p_obdData->car_key_stat);
	printf(" - car_batt_volt [%d]\r\n", p_obdData->car_batt_volt);
	printf(" - car_intake_temp [%d]\r\n", p_obdData->car_intake_temp);
	printf(" - car_outtake_temp [%d]\r\n", p_obdData->car_outtake_temp);
	printf(" - car_maf_delta [%d]\r\n", p_obdData->car_maf_delta);
	printf(" - car_maf_total [%d]\r\n", p_obdData->car_maf_total);
	printf(" - car_map [%d]\r\n", p_obdData->car_map);
	printf(" - car_amp [%d]\r\n", p_obdData->car_amp);
	printf(" - car_remain_fuel_percent [%d]\r\n", p_obdData->car_remain_fuel_percent);
	printf(" - car_engine_torque [%x]\r\n", p_obdData->car_engine_torque);
	printf(" - car_air_gauge_volt [%d]\r\n", p_obdData->car_air_gauge_volt);
	printf(" - car_fuel_gauge_volt [%d]\r\n", p_obdData->car_fuel_gauge_volt);
	printf(" - car_dct [%s]\r\n", p_obdData->car_dct);
	printf("------ obd data end --------------\r\n");
}

void dbg_print_gender_spec(odbGender_t* p_obdGender)
{
	printf("------ obd gender spec start --------------\r\n");
	printf(" - gender_sn [%s]\r\n", p_obdGender->gender_sn);
	printf(" - gender_sw [%d]\r\n", p_obdGender->gender_sw);
	printf(" - gender_car_num [%s]\r\n", p_obdGender->gender_car_num);
	printf(" - gender_car_vin [%s]\r\n", p_obdGender->gender_car_vin);
	
	printf(" - gender_total_trip [%lld]\r\n", p_obdGender->gender_total_trip);
	printf(" - gender_total_fuel [%lld]\r\n", p_obdGender->gender_total_fuel);
	
	printf(" - gender_is_detach [%d]\r\n", p_obdGender->gender_is_detach);
	printf(" - gender_is_detach_cond [%d]\r\n", p_obdGender->gender_is_detach_cond);
	printf("------ obd gender spec end --------------\r\n");
}


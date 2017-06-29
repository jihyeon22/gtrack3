#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/storage.h>

#include <base/dmmgr.h>

#include "util/nettool.h"
#include "logd/logd_rpc.h"
#include "config.h"


#include "kt_fms_packet.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

// -----------------------------------------------
// settings..
// -----------------------------------------------
#define ITOA_TMP_BUFF_SIZE	32

// ------------------------------------------------
// global variable
// ------------------------------------------------
#define TRIP_SEQ_SIZE	12

static char g_trip_seq[TRIP_SEQ_SIZE] = {0,};

static int g_server_send_interval_sec = 0;
static int g_server_stat = KT_FMS_SERVER_RET__SUCCESS;

static int g_cli_req_reset = 0;

static e_FMS_SEND_POLICY 	g_cur_send_policy = KT_FMS_SEND_POLICY__NONE;

static server_policy_t 		g_runtime_policy ={0,};
static server_policy_t 		g_ktfms_server_policy = {0,};
static daliy_car_info_t 	g_daliy_car_info = {0,};

static cli_cmd_mgr_t 		cli_cmd[DEFAULT_FMS_MAX_CLI_CMD_CNT];

// trip seq variable
static int trip_setting_flag = 0;
//static int trip_date = 0;

server_fail_policy_t err_50x_fail_policy;
server_fail_policy_t err_noresp_fail_policy;

last_dev_stat_t g_last_dev_stat = {0,};
// ---------------------------------------------
// internal api
// ---------------------------------------------
static int _get_sdr_server_ret_val(const char* ret_str);
static int set_cli_cmd(char* buff);

// ---------------------------------------------
// utils..
// ---------------------------------------------
int RoundOff(float x, int dig)
{
    return floor((x) * pow(10,dig) + 0.5) / pow(10,dig);
}

// -----------------------------------------------
// const defines..
// -----------------------------------------------
const factor_define_t g_factor_define[] = {
	{e_id_date, 12, "A000"},
	{e_gps_x, 9,"A001"},
	{e_gps_y, 9,"A002"},
	{e_mileage_day, 8,"A003"},
	{e_mileage_total, 11,"A004"},
	{e_speed, 3,"A005"},
	{e_rpm, 4,"A006"},
	{e_break_signal, 1,"A007"},
	{e_gps_azimuth, 3,"A008"},
	{e_acceleration_x, 6,"A009"},
	{e_acceleration_y, 6,"A010"},
	{e_fuel_consumption_day, 7,"A011"},
	{e_fuel_consumption_total, 10,"A012"},
	{e_fuel_efficiency, 4,"A013"},
	{e_engine_oil_temp, 5,"A014"},
	{e_fuel_injection, 6,"A015"},
	{e_acceleration_pedal, 3,"A016"},
	{e_gear_auto, 1,"A017"},
	{e_gear_level, 2,"A018"},
	{e_coolant_temp, 6,"A019"},
	{e_key_stat, 1,"A020"},
	{e_batt_volt, 4,"A021"},
	{e_intake_temp, 5,"A022"},
	{e_outtake_temp, 5,"A023"},
	{e_maf_delta, 4,"A024"},
	{e_maf_total, 4,"A025"},
	{e_map, 7,"A026"},
	{e_amp, 5,"A027"},
	{e_cold_storage_temp, 5,"A028"},
	{e_forzen_storage_temp, 5,"A029"},
	{e_remain_fuel, 5,"A030"},
	{e_dct, 60,"A031"},
	{e_rssi, 4,"A032"},
	{e_dev_stat, 4,"A033"},
	{e_END_OF_FACTOR_IDX,0,"NULL"}
};

// ---------------------------------------------
// sdr factor util
// ---------------------------------------------
int convert_factor_id_to_str(char* buff, const int sdr_factor[e_MAX_FACTOR_ID] )
{
	int i = 0;
	int j = 0;
	
	int found_str = 0;
	
	int convert_len = 0;
	char tmp_buff[512] = {0,};
	
	int ret = KT_FMS_API_RET_FAIL;
	
	for (i < 0 ; i < e_MAX_FACTOR_ID ; i++)
	{
		// 일단 factor 리스트의 끝인지 확인
		if (sdr_factor[i] == e_END_OF_FACTOR_IDX)
		{
			break;
		}
		
		found_str = 0;
		
		// factor list table 에서 해당 factor 가있는지 확인
		for(j = 0 ; (g_factor_define[j].e_factor_size != 0) ; j++ )
		{
			if (g_factor_define[j].e_factor_idx == sdr_factor[i])
			{
				found_str = 1;
				break;
			}
		}
		
		// 찾았으면.. 해당 factor 의 문자열을 카피한다.
		if (found_str == 1)
			convert_len += sprintf(tmp_buff+convert_len, "%s|", g_factor_define[j].factor_name);
	}
	
	if (convert_len > 0)
	{
		// 여기 까지왔으면, 성공이다.
		// 하지만 끝에 "|" 문자가 포함되어있다.
		// 해당 문자를 지우고, 리턴한다.
		strncpy(buff, tmp_buff, convert_len-1);
		ret = convert_len-1;
	}
	
	return ret;
}

int convert_factor_str_to_id(const char* buff, int (*sdr_factor)[e_MAX_FACTOR_ID] )
{
	char *tr;
	char token_0[ ] = "|\r\n";

	char *temp_bp = NULL;

	int i = 0;
	int j = 0;
	
	int found_str = 0;
	
	char tmp_buff[512] = {0,};
	char *p_tmp_buff = tmp_buff;
	
	strcpy(tmp_buff, buff);
	
	while (1)
	{
		tr = strtok_r(p_tmp_buff, token_0, &temp_bp);
		if(tr == NULL) break;
		
		for(j = 0 ; (g_factor_define[j].e_factor_size != 0) ; j++ )
		{
			if (strcmp(g_factor_define[j].factor_name, tr) == 0)
			{
				found_str = 1;
				break;
			}
		}
		
		(*sdr_factor)[i] = g_factor_define[j].e_factor_idx;
		i++;
		
		p_tmp_buff = NULL;
	}
	
	(*sdr_factor)[i] = e_END_OF_FACTOR_IDX;

	return KT_FMS_API_RET_SUCCESS;
}


#define FUEL_TRIP_DEBUG	
// ------------------------------------------------
// make body util
// ------------------------------------------------
int make_sdr_body(char* input_buff, gpsData_t* p_gpsdata, obdData_t* p_obddata, const int sdr_factor[e_MAX_FACTOR_ID])
{
	int i = 0;
	int buff_len = 0;
	char itoa_tmp_buff[ITOA_TMP_BUFF_SIZE] = {0,};
	
	//printf("%s() - [%d] ++\r\n", __func__, __LINE__);
	
	for (i = 0; i < e_MAX_FACTOR_ID; i++)
	{
		if ( sdr_factor[i] == e_END_OF_FACTOR_IDX)
			break;
		
		memset(itoa_tmp_buff, 0x00, ITOA_TMP_BUFF_SIZE);
		
		//printf(" sdr_factor[%d] = [%d]\r\n", i, sdr_factor[i]);
		switch(sdr_factor[i])
		{
			case e_id_date:		// 12, "A000"
			{
				sprintf(itoa_tmp_buff, "%02d", (p_gpsdata->year % 100) );
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->mon);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->day);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->hour);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->min);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->sec);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);

				break;
			}
			//case e_gps_x:	//  9,"A001"
			case e_gps_y:	//  9,"A002"
			{
				//if (p_gpsdata->active == 1)	
				if ( p_gpsdata->lat*1000000 != 0 )
					buff_len += sprintf(input_buff + buff_len, "%.0f", p_gpsdata->lat*1000000);
				// gps 가 잡혀있지 않을경우 아무것도 넣지 않는다.
				break;
			}
			case e_gps_x:	//  9,"A001"
			//case e_gps_y:	//  9,"A002"
			{
				//if (p_gpsdata->active == 1)	
				if ( p_gpsdata->lon*1000000 != 0 )
					buff_len += sprintf(input_buff + buff_len, "%.0f", p_gpsdata->lon*1000000);
				// gps 가 잡혀있지 않을경우 아무것도 넣지 않는다.
				
				break;
			}
			case e_mileage_day:		// 8,"A003"
			{
				// 00시 부터 24시까지 운행..
				long long daliy_trip = get_daliy_trip(p_obddata->car_mileage_total);

				if( daliy_trip > 0 )
				{
					sprintf(itoa_tmp_buff, "%lld.%03lld", daliy_trip/1000, daliy_trip%1000 );
#ifdef FUEL_TRIP_DEBUG
					printf(" -- body pkt debug :: daily trip => [%s]\r\n", itoa_tmp_buff);				
#endif
					buff_len += sprintf(input_buff + buff_len, "%.11s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_mileage_total:	// 11,"A004"
			{
				if( p_obddata->car_mileage_total > 0 )
				{
					sprintf(itoa_tmp_buff, "%lld.%03lld", p_obddata->car_mileage_total/1000, (p_obddata->car_mileage_total%1000) );
#ifdef FUEL_TRIP_DEBUG			
					printf(" -- body pkt debug :: total trip 2 => [%s]\r\n", itoa_tmp_buff);
#endif
					buff_len += sprintf(input_buff + buff_len, "%.11s", itoa_tmp_buff);
					
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				
				break;
			}
			case e_speed:	//  3,"A005"
			{
				int tmp_speed = p_obddata->car_speed/1000;
				
				
				if (( tmp_speed >= 0 ) && ( tmp_speed <= 999 ))
				{
					sprintf(itoa_tmp_buff, "%d", tmp_speed );
					buff_len += sprintf(input_buff + buff_len, "%.3s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_rpm:	// 4,"A006"
			{
				//int tmp_rpm = p_obddata->car_speed/1000;
				
				//printf(" p_obddata->car_rpm is [%d] \r\n",p_obddata->car_rpm);
				// rpm 필수
				if (( p_obddata->car_rpm >= 0 ) && ( p_obddata->car_rpm <= 9999 )) 
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_rpm );
					buff_len += sprintf(input_buff + buff_len, "%.4s", itoa_tmp_buff);
				}
				break;
			}
			case e_break_signal:	// 1,"A007"
			{
				// 유효성 검사
				// 0 : off / 1 : on
				if ( ( p_obddata->car_break_signal == 0 ) || ( p_obddata->car_break_signal == 1 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_break_signal );
					buff_len += sprintf(input_buff + buff_len, "%.1s", itoa_tmp_buff);
				}
				
				// 브레이크 신호는 공란으로 전송가능
				break;
			}
			case e_gps_azimuth:	// 3,"A008"
			{
				if (p_gpsdata->active == 1)	
				{
					sprintf(itoa_tmp_buff, "%d", RoundOff(p_gpsdata->angle, 0) );
					buff_len += sprintf(input_buff + buff_len, "%.3s", itoa_tmp_buff);
				}
				// 자료없을경우 공란전송
				break;
			}
			case e_acceleration_x:	// 6,"A009"
			{
				// 공란전송불가능 => 0 으로 채운다.
				// not support
				buff_len += sprintf(input_buff + buff_len, "0");
				break;
			}
			case e_acceleration_y:	// 6,"A010"
			{
				// 공란전송불가능 => 0 으로 채운다.
				// not support
				buff_len += sprintf(input_buff + buff_len, "0");
				break;
			}
			case e_fuel_consumption_day:	// 7,"A011"
			{
				unsigned int daliy_fuel = 0;

				if ( get_use_obd_device() == 0 )
					break;

				daliy_fuel = get_daliy_fuel(p_obddata->car_fuel_consumption_total);
				// todo 
				// 0 시부터 24시까지 소모량
				// printf("   - body pkt elelment --> daliy fuel is [%d]\r\n", daliy_fuel);

				if ( (daliy_fuel > 0) && (daliy_fuel <= 999999999) )
				{
					sprintf(itoa_tmp_buff, "%d.%03d", daliy_fuel/1000, daliy_fuel%1000 );
#ifdef FUEL_TRIP_DEBUG
					printf(" -- body pkt debug :: daily fuel => [%s]\r\n", itoa_tmp_buff);
#endif
					buff_len += sprintf(input_buff + buff_len, "%.10s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_fuel_consumption_total:	// 10,"A012"
			{				
				if ( get_use_obd_device() == 0 )
					break;

				if ( (p_obddata->car_fuel_consumption_total > 0) && (p_obddata->car_fuel_consumption_total <= 999999999) )
				{
					sprintf(itoa_tmp_buff, "%d.%03d", p_obddata->car_fuel_consumption_total/1000, p_obddata->car_fuel_consumption_total%1000 );
#ifdef FUEL_TRIP_DEBUG
					printf(" -- body pkt debug :: total fuel => [%s]\r\n", itoa_tmp_buff);
#endif
					buff_len += sprintf(input_buff + buff_len, "%.10s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_fuel_efficiency:	// 4,"A013"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if ( (p_obddata->car_fuel_efficiency > 0) && (p_obddata->car_fuel_efficiency <= 999) )
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_fuel_efficiency/10, p_obddata->car_fuel_efficiency%10 );
					buff_len += sprintf(input_buff + buff_len, "%.4s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_engine_oil_temp:	// 5,"A014"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if ( (p_obddata->car_engine_oil_temp > 0) && (p_obddata->car_engine_oil_temp <= 9999) )
				//if( p_obddata->car_engine_oil_temp >= 0 )
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_engine_oil_temp/10,  p_obddata->car_engine_oil_temp%10 );
					buff_len += sprintf(input_buff + buff_len, "%.5s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0.0");
				}
				break;
			}
			case e_fuel_injection:	// 6,"A015"
			{
				if ( get_use_obd_device() == 0 )
					break;

				// obd 에서 넘어온값이 소숫점 3자리 기준으로 표현된다.
				// 하지만 kt fms 에 넘길때는 소숫점 2자리까지만 넘겨야한다.
				// 때문에 일단 끝에 한자리 버리고, 2자리 기준으로 표현하도록한다.
				int temp_fuel_injection_value = p_obddata->car_fuel_injection/10;
				
				if ( (temp_fuel_injection_value > 0) && (temp_fuel_injection_value <= 99999) )
				//if( p_obddata->car_fuel_injection >= 0 )
				{
					sprintf(itoa_tmp_buff, "%d.%02d", temp_fuel_injection_value/100, temp_fuel_injection_value%100 );
					buff_len += sprintf(input_buff + buff_len, "%.6s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_acceleration_pedal:	// 3,"A016"
			{
				if ( get_use_obd_device() == 0 )
					break;

				// not support
				if( ( p_obddata->car_accel_pedal >= 0 ) && ( p_obddata->car_accel_pedal <= 100 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_accel_pedal );
					buff_len += sprintf(input_buff + buff_len, "%.3s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_gear_auto:		// 1,"A017"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if ( ( p_obddata->car_gear_auto == 'P') ||
					 ( p_obddata->car_gear_auto == 'R') ||
					 ( p_obddata->car_gear_auto == 'N') ||
					 ( p_obddata->car_gear_auto == 'D') )
				{
					sprintf(itoa_tmp_buff, "%c", p_obddata->car_gear_auto );
					buff_len += sprintf(input_buff + buff_len, "%.1s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_gear_level:		// 2,"A018"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_gear_level > 0 ) && ( p_obddata->car_gear_level <= 10 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_gear_level );
					buff_len += sprintf(input_buff + buff_len, "%.1s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_coolant_temp:	// 6,"A019"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_coolant_temp > 0 ) && ( p_obddata->car_coolant_temp <= 9999 ) )
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_coolant_temp/10, p_obddata->car_coolant_temp%10 );
					buff_len += sprintf(input_buff + buff_len, "%.6s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_key_stat: 	// 1,"A020"
			{
				if ( ( p_obddata->car_key_stat == 0 ) || ( p_obddata->car_key_stat == 1 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_key_stat );
					buff_len += sprintf(input_buff + buff_len, "%.1s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_batt_volt:	// 4,"A021"
			{
				if( ( p_obddata->car_batt_volt > 0 ) && ( p_obddata->car_batt_volt <= 999 ))
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_batt_volt/10, p_obddata->car_batt_volt%10 );
					buff_len += sprintf(input_buff + buff_len, "%.4s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_intake_temp:	// 5,"A022"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_intake_temp > 0 ) && ( p_obddata->car_intake_temp <= 9999 ))
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_intake_temp/10, p_obddata->car_intake_temp%10 );
					buff_len += sprintf(input_buff + buff_len, "%.5s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_outtake_temp:	// 5,"A023"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_outtake_temp > 0 ) && ( p_obddata->car_outtake_temp <= 9999 ))
				{
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_outtake_temp/10, p_obddata->car_outtake_temp%10 );
					buff_len += sprintf(input_buff + buff_len, "%.5s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_maf_delta:	// 4,"A024"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_maf_delta > 0 ) && ( p_obddata->car_maf_delta <= 9999 ))
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_maf_delta );
					buff_len += sprintf(input_buff + buff_len, "%.4s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_maf_total:	// 4,"A025"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_maf_total > 0 ) && ( p_obddata->car_maf_total <= 9999 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_maf_total );
					buff_len += sprintf(input_buff + buff_len, "%.4s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_map:	// 7,"A026"
			{
				if ( get_use_obd_device() == 0 )
					break;

				// notsupport
				/*
				if( ( p_obddata->car_map >= 0 ) && ( p_obddata->car_map <= 999 ) )
				{
				
					sprintf(itoa_tmp_buff, "%d.0", p_obddata->car_map );
					buff_len += sprintf(input_buff + buff_len, "%.7s", itoa_tmp_buff);
				}
				else
				*/
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				
				break;
			}
			case e_amp:	// 5,"A027"
			{
				if ( get_use_obd_device() == 0 )
					break;

				//printf("p_obddata->car_amp [%d]\r\n",p_obddata->car_amp);
				if( ( p_obddata->car_amp > 0 ) && ( p_obddata->car_amp <= 9999 ) )
				{
					// printf("p_obddata->car_amp [%d]\r\n",p_obddata->car_amp);
					sprintf(itoa_tmp_buff, "%d.%d", p_obddata->car_amp/10, p_obddata->car_amp%10 );
					buff_len += sprintf(input_buff + buff_len, "%.5s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_cold_storage_temp:		// 5,"A028"
			{
				if ( get_use_obd_device() == 0 )
					break;

				buff_len += sprintf(input_buff + buff_len, "0");
				// not support
				break;
			}
			case e_forzen_storage_temp:		// 5,"A029"
			{
				if ( get_use_obd_device() == 0 )
					break;

				buff_len += sprintf(input_buff + buff_len, "0");
				// not support
				break;
			}
			case e_remain_fuel:		// 5,"A030"
			{
				if ( get_use_obd_device() == 0 )
					break;

				if( ( p_obddata->car_remain_fuel_percent > 0 ) && ( p_obddata->car_remain_fuel_percent <= 100 ) )
				{
					// ktfms 에는 소숫점 1자리까지 표시하라고 되어있지만..
					// obd 에서 온 데이터 자체가 상수만 표현되므로, 소숫점은 무조건 0 으로 채운다.
					sprintf(itoa_tmp_buff, "%d.0", p_obddata->car_remain_fuel_percent );
					buff_len += sprintf(input_buff + buff_len, "%.5s", itoa_tmp_buff);
				}
				else
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_dct:		// 60,"A031"
			{
				// 없을경우 공란
				//buff_len += sprintf(input_buff + buff_len, "%.60s", p_obddata->car_dct);
				break;
			}
			case e_rssi:	// 4,"A032"
			{
				// not support yet
				buff_len += sprintf(input_buff + buff_len, "0");
				break;
			}
			case e_dev_stat:	// 4,"A033"
			{
				// not support yet
				int err_code_len = 0;
				char err_code_str[128] = {0,};
				
				err_code_len = get_hw_err_code(err_code_str);
				if ( err_code_len > 0 )
				{
					buff_len += sprintf(input_buff + buff_len, "%s", err_code_str);
				}
				else // no err
				{
					//buff_len += sprintf(input_buff + buff_len, "");
				}
				
				break;
			}
			default:
			{
				printf(" sdr_factor [%d] is not support\r\n", sdr_factor[i]);
				break;
			}
		}
		
		// 각 팩터의 구분자는 '|' 으로 채운다.
		buff_len += sprintf(input_buff+buff_len, "|");
	}
	
	// 여기까지 왔으면 모든 데이터를 만들었다.
	// 하지만, 끝에 "|" 문자가있다.
	// 해당 문자를 지우고 "\r\n" 으로 채워넣는다.
	buff_len--;
	
	buff_len += sprintf(input_buff+buff_len, "\r\n");
	//printf("%s() - [%d]\r\n", __func__, __LINE__);
	return buff_len;
}


// ------------------------------------------------
// make body util
// ------------------------------------------------
int make_sdr_body_null(char* input_buff, gpsData_t* p_gpsdata, obdData_t* p_obddata, const int sdr_factor[e_MAX_FACTOR_ID])
{
	int i = 0;
	int buff_len = 0;
	char itoa_tmp_buff[ITOA_TMP_BUFF_SIZE] = {0,};
	
	//printf("%s() - [%d] ++\r\n", __func__, __LINE__);
	
	for (i = 0; i < e_MAX_FACTOR_ID; i++)
	{
		if ( sdr_factor[i] == e_END_OF_FACTOR_IDX)
			break;
		
		memset(itoa_tmp_buff, 0x00, ITOA_TMP_BUFF_SIZE);
		
		//printf(" sdr_factor[%d] = [%d]\r\n", i, sdr_factor[i]);
		switch(sdr_factor[i])
		{
			case e_id_date:		// 12, "A000"
			{
				sprintf(itoa_tmp_buff, "%02d", (p_gpsdata->year % 100) );
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->mon);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->day);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->hour);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->min);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);
				
				sprintf(itoa_tmp_buff, "%02d", p_gpsdata->sec);
				buff_len += sprintf(input_buff + buff_len, "%.2s", itoa_tmp_buff);

				break;
			}
			//case e_gps_x:	//  9,"A001"
			case e_gps_y:	//  9,"A002"
			{
				//if (p_gpsdata->active == 1)	
				if ( p_gpsdata->lat*1000000 != 0 )
					buff_len += sprintf(input_buff + buff_len, "%.0f", p_gpsdata->lat*1000000);
				// gps 가 잡혀있지 않을경우 아무것도 넣지 않는다.
				break;
			}
			case e_gps_x:	//  9,"A001"
			//case e_gps_y:	//  9,"A002"
			{
				//if (p_gpsdata->active == 1)	
				if ( p_gpsdata->lon*1000000 != 0 )
					buff_len += sprintf(input_buff + buff_len, "%.0f", p_gpsdata->lon*1000000);
				// gps 가 잡혀있지 않을경우 아무것도 넣지 않는다.
				
				break;
			}
			case e_mileage_day:		// 8,"A003"
			{
//				buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_mileage_total:	// 11,"A004"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_speed:	//  3,"A005"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_rpm:	// 4,"A006"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_break_signal:	// 1,"A007"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_gps_azimuth:	// 3,"A008"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_acceleration_x:	// 6,"A009"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_acceleration_y:	// 6,"A010"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_fuel_consumption_day:	// 7,"A011"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
			}
			case e_fuel_consumption_total:	// 10,"A012"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_fuel_efficiency:	// 4,"A013"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_engine_oil_temp:	// 5,"A014"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_fuel_injection:	// 6,"A015"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_acceleration_pedal:	// 3,"A016"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_gear_auto:		// 1,"A017"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_gear_level:		// 2,"A018"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_coolant_temp:	// 6,"A019"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_key_stat: 	// 1,"A020"
			{
				/*
				if ( ( p_obddata->car_key_stat == 0 ) || ( p_obddata->car_key_stat == 1 ) )
				{
					sprintf(itoa_tmp_buff, "%d", p_obddata->car_key_stat );
					buff_len += sprintf(input_buff + buff_len, "%.1s", itoa_tmp_buff);
				}
				else if ( p_obddata->car_key_stat == -1 )
				{
					buff_len += sprintf(input_buff + buff_len, "1");
					;; // noting data.
				}
				else
				*/
				{
					buff_len += sprintf(input_buff + buff_len, "0");
				}
				break;
			}
			case e_batt_volt:	// 4,"A021"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_intake_temp:	// 5,"A022"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_outtake_temp:	// 5,"A023"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_maf_delta:	// 4,"A024"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_maf_total:	// 4,"A025"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_map:	// 7,"A026"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_amp:	// 5,"A027"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_cold_storage_temp:		// 5,"A028"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				// not support
				break;
			}
			case e_forzen_storage_temp:		// 5,"A029"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				// not support
				break;
			}
			case e_remain_fuel:		// 5,"A030"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_dct:		// 60,"A031"
			{
		//		buff_len += sprintf(input_buff + buff_len, "");
				break;
			}
			case e_rssi:	// 4,"A032"
			{
		//		buff_len += sprintf(input_buff + buff_len, "0");
				break;
			}
			case e_dev_stat:	// 4,"A033"
			{
				// not support yet
				int err_code_len = 0;
				char err_code_str[128] = {0,};
				
				set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DISCONN , 1);
				set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DATA_ERR , 1);
					
				err_code_len = get_hw_err_code(err_code_str);
			
				if ( err_code_len > 0 )
				{
					buff_len += sprintf(input_buff + buff_len, "%s", err_code_str);
				}
				else // no err
				{
					buff_len += sprintf(input_buff + buff_len, "");
				}
				
				break;
			}
			default:
			{
				printf(" sdr_factor [%d] is not support\r\n", sdr_factor[i]);
				break;
			}
		}
		
		// 각 팩터의 구분자는 '|' 으로 채운다.
		buff_len += sprintf(input_buff+buff_len, "|");
	}
	
	// 여기까지 왔으면 모든 데이터를 만들었다.
	// 하지만, 끝에 "|" 문자가있다.
	// 해당 문자를 지우고 "\r\n" 으로 채워넣는다.
	buff_len--;
	
	buff_len += sprintf(input_buff+buff_len, "\r\n");
	//printf("%s() - [%d]\r\n", __func__, __LINE__);
	return buff_len;
}


// ------------------------------------------------
// make head util
// ------------------------------------------------
int make_sdr_header(char* input_buff, int body_size, const int sdr_factor[e_MAX_FACTOR_ID], int policy_num, e_FMS_SEND_POLICY send_policy)
{
	char sdr_factor_str[512] = {0,};
	char tmp_sdr_header[1024] ={0,};
	
	fms_car_info_t tmp_car_info;
	char tmp_dev_phone_num[16] = {0,};
	server_policy_t tmp_server_policy = {0,};
	
	static char last_trip_seq[TRIP_SEQ_SIZE+1] = {0,};
	char tmp_trip_seq[TRIP_SEQ_SIZE+1] = {0,};
	char tmp_cli_buff[64] = {0,};
	char tmp_ip_addr[64] = {0,};
	//char tmp_company_id[32] = {0,};
	char tmp_url_path[64] = {0,};
	int tmp_port = 0;
	
	int buff_len = 0;
	int data_header_len = 0;
	
	static int cur_policy_num = 0;
	
	memset(&tmp_car_info, 0x00, sizeof(tmp_car_info));
	
	convert_factor_id_to_str(sdr_factor_str, sdr_factor);
	
	//printf("convert_factor_id_to_str is [%s]\r\n",sdr_factor_str);
	
	get_server_ip(tmp_ip_addr);
	tmp_port = get_server_port();
	
	// get trip seq
	get_trip_seq(tmp_trip_seq);
	
	// get phone num
	at_get_phonenum(tmp_dev_phone_num, 16);
	
	// 차량 정보
	get_fms_car_info(&tmp_car_info);
	
	// 서버정책
	get_server_policy(&tmp_server_policy);
	
	//get_car_info_company_id(tmp_company_id);
	get_car_info_url_path(tmp_url_path);
	
	// ------------------------------------------------
	// 먼저 실제 데이터 헤더를 만든다.
	// ------------------------------------------------

	// 1 : 단말기 고유번호
	if ( policy_num == 0 ) 
	{
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_dev_phone_num);
	}
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_dev_phone_num);
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	
	// 2 : 단말기 모델명
	if ( policy_num == 0 ) 
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.50s\r\n", DEFAULT_FMS_DEV_MODEL);
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.50s\r\n", DEFAULT_FMS_DEV_MODEL);
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	
	// 3 : 단말기 펌웨어 
	if ( policy_num == 0 ) 
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.10s\r\n", KT_FOTA_VER_APP);
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.10s\r\n", KT_FOTA_VER_APP);
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	
	// 4 : 사업자번호 (*) 
	if ( policy_num == 0 ) 
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.13s\r\n", tmp_car_info.user_business_no);
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.13s\r\n", tmp_car_info.user_business_no);
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	
	// 5 : 차대번호 (*)
	data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.17s\r\n", tmp_car_info.car_vin);
	
	// 6 : 차량번호
	if ( policy_num == 0 ) 
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.12s\r\n", tmp_car_info.car_no);
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.12s\r\n", tmp_car_info.car_no);
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	
	// 7 : 운전자 아이디 (*)
	data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_car_info.driver_id);
	
	// 8 : 트립시퀀스
	if ( policy_num == 0 ) 
	{
		strcpy(last_trip_seq, tmp_trip_seq);
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_trip_seq);
	}
	else if ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT )
	{
		strcpy(last_trip_seq, tmp_trip_seq);
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_trip_seq);
	}
	else
	{
		// 트립스퀀스의 날짜가 바뀌면 표시해야한다고 한다.
		if (strcmp(last_trip_seq, tmp_trip_seq) == 0 )
		{
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
		}
		else
		{
			strcpy(last_trip_seq, tmp_trip_seq);
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.20s\r\n", tmp_trip_seq);
		}
	}
	
	// 9 : cli 시퀀스리스트
	if ( get_cli_cmd_stat(tmp_cli_buff) > 0 )
	{
		printf("tmp_cli_buff is [%s]\r\n",tmp_cli_buff);
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%s\r\n", tmp_cli_buff);
	}
	else
	{
		//printf("tmp_cli_buff is [%s]\r\n",tmp_cli_buff);
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	}
	// 10 : 메시지 시퀀스 리스트
	data_header_len += sprintf(tmp_sdr_header + data_header_len, "%s\r\n", "");
	
	// 11 : sdr 정책
	// policy num 가 0 이면 없는거다.
	if ( policy_num == 0 ) 
	{
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.4s\r\n", "N");
		// 12 : keep alive 정책
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.4s\r\n", "U");
	}
	else
	{
		// 정책이 변경된거다.
		if ( ( policy_num > 0 ) && ( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT ))
		{
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "%d\r\n", policy_num);
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.4s\r\n", "U");
		}
		else if ( cur_policy_num != policy_num )
		{
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.4s\r\n", "S");
			// 12 : keep alive 정책
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
		}
		else
		{
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
			data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
		}
	}
	
	cur_policy_num = policy_num;
	
	/*
	// 12 : keep alive 정책
	if (( send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT ) || (send_policy == KT_FMS_SEND_POLICY__INIT_EVENT))
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "%.4s\r\n", "U");
	else
		data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	*/
	
	// 13 : reserved 1 line
	data_header_len += sprintf(tmp_sdr_header + data_header_len, "\r\n");
	// 14 : factor id list
	data_header_len += sprintf(tmp_sdr_header + data_header_len, "%s\r\n", sdr_factor_str);
	// 15 ~ : 이후에는 body 가 붙는다.. 

	// ------------------------------------------------
	// http 해더를 만든다.
	// ------------------------------------------------
	buff_len += sprintf(input_buff + buff_len, "POST /%s HTTP/1.1\r\n",tmp_url_path);
	buff_len += sprintf(input_buff + buff_len, "Host: %s:%d\r\n",tmp_ip_addr, get_server_port());
	buff_len += sprintf(input_buff + buff_len, "Content-Type: text/plain;charset=EUC-KR\r\n");
	buff_len += sprintf(input_buff + buff_len, "Content-Length: %d\r\n", (data_header_len + body_size));
	buff_len += sprintf(input_buff + buff_len, "Connection: close\r\n");
	buff_len += sprintf(input_buff + buff_len, "\r\n");
	//buff_len += sprintf(input_buff + buff_len, "\r\n");
	
	//printf("tmp_sdr_header is [%s]\r\n",input_buff);
	
	memcpy((input_buff + buff_len), tmp_sdr_header, data_header_len);
	//printf("???");
	buff_len += data_header_len;

	return buff_len;

}

// ------------------------------------------------------
//  server stat :
// ------------------------------------------------------
int get_server_stat()
{
	return g_server_stat;
}

static int set_server_stat(int stat)
{
	g_server_stat = stat;
	return g_server_stat;
}

// --------------------------------------------------------
// server return proc
// --------------------------------------------------------
int set_server_stat_from_return_str(const char* ret_buff)
{
	char temp_buff[1024] = {0,};
	char* p_tmp  = NULL;
	
	int i = 0 ;
	int j = 0 ;
	int k = 0 ;
	
	int server_ret_code = 0;
	
	// -----------------------------------------
	// 1. 일단 서버의 리턴값을 얻어온다.
	//   - "HTTP/1.1 200 OK"
	// -----------------------------------------
	p_tmp = strstr(ret_buff,"HTTP/1.1 ");
	if ( p_tmp == NULL )
	{
		return -1;
	}
	
	p_tmp += strlen("HTTP/1.1 ");
	
	// 공백까지의 문자열을 복사하고, 숫자로 변환
	for (i = 0 ; p_tmp[i] != NULL ; i ++)
	{	
		temp_buff[j] = p_tmp[i];
		
		if ( ( p_tmp[i] == ' ' ) )
		{
			temp_buff[j] = '\0';
			
			if (j <= 0)
				return -1;
			
			// TODO : 숫자 자료형검사
			server_ret_code = atoi(temp_buff);
			break;
		}
		else
		{
			j++;
		}
	}
	// 여기까지왔으면 서버 코드 획득완료
	LOGI(LOG_TARGET, "FMS SERVER RET CODE : [%d]\n", server_ret_code);
	
	set_server_stat(server_ret_code);
	
	// g_server_send_interval_sec
	
	if ( server_ret_code != KT_FMS_SERVER_RET__SUCCESS )
		return server_ret_code;
	
	// 여기까지왔으면 성공..
	
	// -----------------------------------------
	// 2. 컨텐츠 body 에서 문자열을 추출한다.
	//   - 컨텐츠의 각 데이터는 \n 으로 구분된다.
	//   - http 헤더는 \r\n 이고 데이터 부분만 \n 이다 ㅡㅡ; 복잡해서 그냥 수동으로 카피
	// -----------------------------------------
	
	j = 0;
	i = 0;
	
	p_tmp = strstr(ret_buff,"\r\n\r\n");
	if ( p_tmp == NULL )
		return KT_FMS_SERVER_RET__SERVER_UNKOWN;
	
	p_tmp += strlen("\r\n\r\n") ;
	
	LOGI(LOG_TARGET, "FMS SERVER RET PARSING START : [%d]\n", server_ret_code);
	
	for (i = 0 ; p_tmp[i] != NULL ; i ++)
	{	
		temp_buff[j] = p_tmp[i];
		
		if ( ( p_tmp[i] == '\n' ) )
		{
			temp_buff[j] = '\0';
			printf("\r\n - server return value is [%d][%s] \r\n", k, temp_buff);
			switch(k)
			{
				case 0 : // CLI Seq command
				{
					set_cli_cmd(temp_buff);
					break;
				}
				case 1 : // 메시지 Seq	command
				{
					break;
				}
				case 2 : // 메시지 리스트
				{
					break;
				}
				case 3 :  // SDR 수집정책
				{
					_get_sdr_server_ret_val(temp_buff);
					break;
				}
				default : 
					break;
			}
			
			k++;
			
			j = 0;
		}
		else
		{
			j++;
		}
	}
	
	
	return server_ret_code;
}



// --------------------------------------------------------
// internal api
// --------------------------------------------------------
static int _get_sdr_server_ret_val(const char* ret_str)
{
	char *tr;
	char token_0[ ] = "|\r\n";

	char *temp_bp = NULL;

	//int i = 0;
	//int j = 0;
	
	//int found_str = 0;
	//int ret = KT_FMS_API_RET_FAIL;
	
	char tmp_buff[512] = {0,};
	char *p_tmp_buff = tmp_buff;
	
	server_policy_t tmp_server_policy = {0,};
	
	strcpy(tmp_buff, ret_str);
	
	tr = strtok_r(p_tmp_buff, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;
	//LOGD(LOG_TARGET, " - SRV - SDR Ret : no. [%d]\n", atoi(tr));
	// 번호 : 의미없음
	tmp_server_policy.policy_num = atoi(tr);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;	
	// 전송주기 (초)
	tmp_server_policy.pkt_send_interval_sec = atoi(tr);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_interval_sec [%d]\n", tmp_server_policy.pkt_send_interval_sec);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	// 수집주기 (초)
	tmp_server_policy.pkt_collect_interval_sec = atoi(tr);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_collect_interval_sec [%d]\n", tmp_server_policy.pkt_collect_interval_sec);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	// 재전송 지연시간 (분)	
	tmp_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec = atoi(tr) * 60;
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_retry_delay_sec [%d]\n", tmp_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	// 재전송 횟수
	tmp_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt = atoi(tr);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_fail_retry_cnt [%d]\n", tmp_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	// 폐기기준횟수
	tmp_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt = atoi(tr);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_fail_remove_cnt [%d]\n", tmp_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	// 전송중단 기준횟수
	tmp_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt = atoi(tr);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_fail_stop_cnt [%d]\n", tmp_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt);
	
	
	tr = strtok_r(NULL, "", &temp_bp);
	if(tr == NULL) return KT_FMS_API_RET_FAIL;		
	convert_factor_str_to_id(tr, &tmp_server_policy.sdr_factor);
	LOGD(LOG_TARGET, " - SRV - SDR Ret : sdr factor [%s]\n", tr);
	
	// SDR factor list
	set_server_policy(&tmp_server_policy);
	
	//LOGD(LOG_TARGET, " - _get_sdr_server_ret_val --- \n", tr);
	return KT_FMS_API_RET_SUCCESS;
}



// ----------------------------------------------------
// server policy api
// ----------------------------------------------------
int get_server_send_interval()
{
	return g_server_send_interval_sec;
}

int set_server_send_interval(int sec)
{
	printf(" >> set server send interval : [%d] -> [%d]\r\n", g_server_send_interval_sec, sec);
	g_server_send_interval_sec = sec;
	return g_server_send_interval_sec;
}

int set_server_send_interval_default()
{
	//g_server_send_interval_sec = g_ktfms_server_policy.pkt_send_interval_sec / 2;
	printf(" >> set server send interval (default) : [%d] -> [%d]\r\n", g_server_send_interval_sec, SERVER_SEND_DEFAULT_INTERVAL);
	g_server_send_interval_sec = SERVER_SEND_DEFAULT_INTERVAL;
	return g_server_send_interval_sec;
}

// ----------------------------------------------------
// server policy api
// ----------------------------------------------------
int get_server_policy(server_policy_t* p_srv_policy)
{
	memcpy(p_srv_policy, &g_ktfms_server_policy, sizeof(server_policy_t));
	return KT_FMS_API_RET_SUCCESS;
}

int get_server_fail_policy(int ret_code, server_fail_policy_t* p_svr_fail_policy)
{
	switch(ret_code)
	{
		case KT_FMS_SERVER_RET__SUCCESS:		// 200
		case KT_FMS_SERVER_RET__NEED_MORE_DATA:	// 480
		case KT_FMS_SERVER_RET__INVAILD_DATA_1:	// 400
		case KT_FMS_SERVER_RET__INVAILD_DATA_2:	// 481
		case KT_FMS_SERVER_RET__INVAILD_DATA_3:	// 482
		{
			p_svr_fail_policy->pkt_send_fail_retry_cnt = g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt;
			p_svr_fail_policy->pkt_send_fail_remove_cnt =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt;
			p_svr_fail_policy->pkt_send_retry_delay_sec =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec;
			p_svr_fail_policy->pkt_send_fail_stop_cnt =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt;
			break;
		}
		
		case KT_FMS_SERVER_RET__SERVER_ERR_1:	// 500
		case KT_FMS_SERVER_RET__SERVER_ERR_2:	// 503
		{
			p_svr_fail_policy->pkt_send_fail_retry_cnt = g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt;
			p_svr_fail_policy->pkt_send_fail_remove_cnt =  9999999;
			p_svr_fail_policy->pkt_send_retry_delay_sec =  30 * 60; // 30min
			p_svr_fail_policy->pkt_send_fail_stop_cnt =  9999999;
			break;
		}
		case KT_FMS_SERVER_RET__SERVER_NO_RESP:
		{
			p_svr_fail_policy->pkt_send_fail_retry_cnt = g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt;
			p_svr_fail_policy->pkt_send_fail_remove_cnt = 9999999;
			p_svr_fail_policy->pkt_send_retry_delay_sec =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec;
			p_svr_fail_policy->pkt_send_fail_stop_cnt =  9999999;
			break;
		}
		case KT_FMS_SERVER_RET__NOT_REGI_DEV:	// 403
		{
			p_svr_fail_policy->pkt_send_fail_retry_cnt = 9999999;
			p_svr_fail_policy->pkt_send_fail_remove_cnt = 9999999;
			p_svr_fail_policy->pkt_send_retry_delay_sec =  9999999;
			p_svr_fail_policy->pkt_send_fail_stop_cnt =  9999999;
			break;
		}
		default:
		{
			p_svr_fail_policy->pkt_send_fail_retry_cnt = g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt;
			p_svr_fail_policy->pkt_send_fail_remove_cnt =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt;
			p_svr_fail_policy->pkt_send_retry_delay_sec =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec;
			p_svr_fail_policy->pkt_send_fail_stop_cnt =  g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt;
			break;
		}
	}
	
	printf("-- runtime policy -------------------------------\r\n");
	printf("   -> pkt_send_fail_retry_cnt [%d]\r\n", g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt);
	printf("   -> pkt_send_fail_remove_cnt [%d]\r\n", g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt);
	printf("   -> pkt_send_retry_delay_sec [%d]\r\n", g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec);
	printf("   -> pkt_send_fail_stop_cnt [%d]\r\n", g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt);
	printf("-- set to policy -------------------------------\r\n");
	printf("   -> pkt_send_fail_retry_cnt [%d]\r\n", p_svr_fail_policy->pkt_send_fail_retry_cnt);
	printf("   -> pkt_send_fail_remove_cnt [%d]\r\n", p_svr_fail_policy->pkt_send_fail_remove_cnt);
	printf("   -> pkt_send_retry_delay_sec [%d]\r\n", p_svr_fail_policy->pkt_send_retry_delay_sec);
	printf("   -> pkt_send_fail_stop_cnt [%d]\r\n", p_svr_fail_policy->pkt_send_fail_stop_cnt);
	printf("-------------------------------------------------\r\n");
	
	
	return KT_FMS_API_RET_SUCCESS;
}



int set_server_policy(server_policy_t* p_srv_policy)
{
	int need_save = 0;
	int i = 0;
	
	LOGD(LOG_TARGET, "%s() [%d]  --- \n", __func__, __LINE__);
	
	if ( p_srv_policy->policy_num != g_ktfms_server_policy.policy_num ) 
		need_save = 1;
	
	if ( p_srv_policy->runtime_fail_policy.pkt_send_fail_retry_cnt != g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt ) 
		need_save = 1;
	
	if ( p_srv_policy->runtime_fail_policy.pkt_send_fail_remove_cnt != g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt )
		need_save = 1;
	
	if ( p_srv_policy->runtime_fail_policy.pkt_send_retry_delay_sec != g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec )
		need_save = 1;
	
	if ( p_srv_policy->runtime_fail_policy.pkt_send_fail_stop_cnt != g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt )
		need_save = 1;
	

	if ( p_srv_policy->pkt_send_interval_sec != g_ktfms_server_policy.pkt_send_interval_sec )
	{
		need_save = 1;
		set_report_interval ( p_srv_policy->pkt_send_interval_sec ); 
	}


	if ( p_srv_policy->pkt_collect_interval_sec != g_ktfms_server_policy.pkt_collect_interval_sec )
	{
		need_save = 1;
		set_collection_interval( p_srv_policy->pkt_collect_interval_sec );
	}

	for ( i = 0 ; i < e_MAX_FACTOR_ID ; i ++)
	{
		if ( p_srv_policy->sdr_factor[i] != g_ktfms_server_policy.sdr_factor[i])
			need_save = 1;
	}
	
	// 변화된 값이 있을때만 저장.
	if ( need_save == 1 )
	{
		memcpy(&g_ktfms_server_policy, p_srv_policy, sizeof(server_policy_t));
		storage_save_file(SERVER_POLICY_SAVE_PATH, (void*)&g_ktfms_server_policy, sizeof(server_policy_t));
	}
	
	LOGI(LOG_TARGET, "SDR Server policy save => [%d], [%d]\n", need_save , g_ktfms_server_policy.policy_stat);
	
	return KT_FMS_API_RET_SUCCESS;
	
}

e_FMS_SEND_POLICY get_send_policy()
{
	return g_cur_send_policy;
}

e_FMS_SEND_POLICY set_send_policy(e_FMS_SEND_POLICY cur_policy)
{
	printf("set_send_policy change [%d] -> [%d]\r\n", g_cur_send_policy, cur_policy);
	g_cur_send_policy = cur_policy;
	
	switch ( cur_policy )
	{
		case KT_FMS_SEND_POLICY__PWR_ON_EVENT:
		case KT_FMS_SEND_POLICY__PWR_OFF_EVENT:
		//case KT_FMS_SEND_POLICY__KEEP_ALIVE:
		case KT_FMS_SEND_POLICY__RUNNING:
		{
			//set_server_send_interval_default();
			break;
		}
		case KT_FMS_SEND_POLICY__SERVER_FAIL_STOP:
		{
			set_server_send_interval(9999999);
		}
		default :
			break;
	}
	
	return g_cur_send_policy;
}




int init_fms_server_policy()
{
	int ret = 0;
	int i = 0;
	server_policy_t tmp_policy = {0,};
	
	// ---------------------------
	// init data..
	// ---------------------------
	memset(&g_ktfms_server_policy, 0x00, sizeof(server_policy_t));
	memset(&g_daliy_car_info, 0x00, sizeof(g_daliy_car_info));
	memset(&g_trip_seq, 0x00, sizeof(g_trip_seq));
	
	g_server_stat = KT_FMS_SERVER_RET__SUCCESS;
	g_cur_send_policy = KT_FMS_SEND_POLICY__NONE;
	
	g_server_send_interval_sec = 0;
	
	for (i = 0 ; i < DEFAULT_FMS_MAX_CLI_CMD_CNT ; i++)
	{
		memset(&cli_cmd[i], 0x00, sizeof(cli_cmd_mgr_t));
	}
	
	// --------------------------------
	ret = storage_load_file(SERVER_POLICY_SAVE_PATH, &tmp_policy, sizeof(server_policy_t));
	
	if(ret != ERR_NONE)
	{
		// 일단 policy stat 은 서버에 전송을 하기위한 프래그이다.
		// 일단 기본정책은 설정하지만, 최초 전송시 none 으로 보내서 서버 설정을 갖고오도록한다.
		
		memset(&g_ktfms_server_policy, 0x00, sizeof(server_policy_t));

		// init 상태로 변경한다.
		set_send_policy(KT_FMS_SEND_POLICY__INIT_EVENT);
		
		// sdr factor 를 init 값으로 변경한다.
		convert_factor_str_to_id(DEFAULT_FMS_SDR_INIT, &g_ktfms_server_policy.sdr_factor);
		
		g_ktfms_server_policy.policy_stat = e_FMS_SVR_POLICY_STAT_NONE;
		
		// 기존 정책 없으면 디폴트 값 적용
		g_ktfms_server_policy.pkt_collect_interval_sec = DEFAULT_FMS_INIT_COLLECT_INTERVAL;
		g_ktfms_server_policy.pkt_send_interval_sec = DEFAULT_FMS_INIT_SEND_INTERVAL;
		g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_retry_cnt = DEFAULT_FMS_SEND_FAIL_RETRY_CNT;
		g_ktfms_server_policy.runtime_fail_policy.pkt_send_retry_delay_sec = DEFAULT_FMS_SEND_RETRY_DELAY_SEC;
		g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_remove_cnt = DEFAILT_FMS_SEND_FAIL_REMOVE_CNT;
		g_ktfms_server_policy.runtime_fail_policy.pkt_send_fail_stop_cnt = DEFAILT_FMS_SEND_FAIL_STOP_CNT;
		
		set_collection_interval(g_ktfms_server_policy.pkt_collect_interval_sec);
		set_report_interval(g_ktfms_server_policy.pkt_send_interval_sec);
		
		LOGI(LOG_TARGET, "fail server policy [%d]=>[%d]/[%d]\n", g_ktfms_server_policy.policy_num, g_ktfms_server_policy.pkt_collect_interval_sec, g_ktfms_server_policy.pkt_send_interval_sec);
		
		// server policy 가 fail 이면 이전 trip seq 도 clear 한다.
		clear_trip_seq();
	}
	else
	{
		int tmp_collect_interval = 0;
		int tmp_send_interval  = 0;
		
		memcpy(&g_ktfms_server_policy, &tmp_policy, sizeof(server_policy_t));
		
		tmp_collect_interval = g_ktfms_server_policy.pkt_collect_interval_sec;
		tmp_send_interval = g_ktfms_server_policy.pkt_send_interval_sec;
		
		// power on 으로 변경한다.
		set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);
		
		// sdr factor 를 읽은 값으로 변경한다.
		g_ktfms_server_policy.policy_stat = e_FMS_SVR_POLICY_STAT_RUNNING;
		
		// 변화된값 저장
		set_collection_interval(tmp_collect_interval);
		set_report_interval(tmp_send_interval);
	

		LOGI(LOG_TARGET, "Load Success server policy [%d]=>[%d]/[%d]\n", g_ktfms_server_policy.policy_num, tmp_send_interval, tmp_collect_interval);
		
	}
	

	set_runtime_server_policy();
		
	return KT_FMS_API_RET_SUCCESS;

}

int clr_fms_server_policy()
{
	remove(SERVER_POLICY_SAVE_PATH);
	remove(SERVER_POLICY_SAVE_PATH2);
	return 0;
}

int get_cur_sdr_factor(int (*sdr_factor)[e_MAX_FACTOR_ID])
{
	//g_ktfms_server_policy.sdr_factor
	/*
	{
		char test_buff[256] = {0,};
		convert_factor_id_to_str(test_buff, g_ktfms_server_policy.sdr_factor);
		printf("[%s] g_ktfms_server_policy.sdr_factor is [%s]\r\n",__func__, test_buff);
	}*/
	memcpy(sdr_factor, g_ktfms_server_policy.sdr_factor, sizeof(g_ktfms_server_policy.sdr_factor));
	
	return KT_FMS_API_RET_SUCCESS;
}

void set_cur_sdr_factor_id(const int sdr_factor[e_MAX_FACTOR_ID])
{
	memcpy(&g_ktfms_server_policy.sdr_factor, sdr_factor, e_MAX_FACTOR_ID);
}


int init_trip_seq()
{
	char trip_seq[TRIP_SEQ_SIZE+1] = {0,};
	
	int ret = 0;
	
	memset(g_trip_seq, 0x00, sizeof(g_trip_seq));
	
	ret = storage_load_file(CAR_TRIP_SEQ_PATH, &trip_seq, TRIP_SEQ_SIZE);
	
	if(ret != ERR_NONE)
	{
		LOGI(LOG_TARGET, "TRIP SEQ LOAD FAIL [%s]\n", g_trip_seq);
		trip_setting_flag = 0;
	}
	else
	{
		strncpy(g_trip_seq, trip_seq, strlen(trip_seq));
		
		LOGI(LOG_TARGET, "TRIP SEQ LOAD OK [%s]\n", g_trip_seq);
		
		trip_setting_flag = 1;
	}
	
	return 0;
}


void set_trip_seq(int year, int mon, int day, int hour, int min, int sec)
{
	char trip_seq[TRIP_SEQ_SIZE+1] = {0,};
	char tmp_buff[12] = {0,};
	int buff_len = 0;
	
	
	//LOGD(LOG_TARGET, "TRIP SEQ API [%s] / [%d]\n", g_trip_seq, trip_setting_flag);
	
	if ( (year % 100) < 16)
		return;
	
	if ( trip_setting_flag == 1  )
	{
		//LOGD(LOG_TARGET, "TRIP SEQ ALREADY SAVED [%s]\n", g_trip_seq);
		return;
	}
	
	sprintf(tmp_buff, "%02d", (year % 100) );
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);

	sprintf(tmp_buff, "%02d", mon);
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);

	sprintf(tmp_buff, "%02d", day);
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);

	sprintf(tmp_buff, "%02d", hour);
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);

	sprintf(tmp_buff, "%02d", min);
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);

	sprintf(tmp_buff, "%02d", sec);
	buff_len += sprintf(trip_seq + buff_len, "%.2s", tmp_buff);
	
	//trip_date = (year % 100)*10000 + mon*100 + day;
	
	//if ( strncmp(g_trip_seq, trip_seq, strlen(trip_seq)) == 0 )
	{
		//devel_webdm_send_log("SET TRIP SEQ");
		strncpy(g_trip_seq, trip_seq, TRIP_SEQ_SIZE);
	}
	
	LOGI(LOG_TARGET, "TRIP SEQ SAVE START [%s]\n", g_trip_seq);

	printf("save trip!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	if ( init_server_routine() == 0 )
		storage_save_file(CAR_TRIP_SEQ_PATH, &g_trip_seq, sizeof(g_trip_seq));
	
	trip_setting_flag = 1;
	
}

int clear_trip_seq()
{

	printf("clear trip!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	
	remove(CAR_TRIP_SEQ_PATH);
	remove(CAR_TRIP_SEQ_PATH2);
	
	trip_setting_flag = 0;
	memset(g_trip_seq, 0x00, sizeof(g_trip_seq));
	return 0;
}

int get_trip_seq(char* buff)
{
	if (trip_setting_flag == 1)
	{
		strncpy(buff, g_trip_seq, TRIP_SEQ_SIZE);
		return KT_FMS_API_RET_SUCCESS;
	}
	else
	{
		strncpy(buff, "160000000000", TRIP_SEQ_SIZE);
		return KT_FMS_API_RET_FAIL;
	}
}




static int set_cli_cmd(char* buff)
{
	char *tr;
	char token_0[ ] = "|\r\n";

	char *temp_bp = NULL;
	
	int i = 0;
//	int j = 0;
//	int k = 0;
	
	int found = 0;
	
	for(i = 0; i < DEFAULT_FMS_MAX_CLI_CMD_CNT ; i ++ )
	{
		if ( cli_cmd[i].cmd_stat == e_CLI_CMD_NONE )
		{
			found = 1;
			break;
		}
	}
	
	char *p_tmp_buff = buff;

	if ( found == 0 )
		return -1;
	
	memset ( &cli_cmd[i], 0x00, sizeof (cli_cmd_mgr_t) );
	
	tr = strtok_r(p_tmp_buff, token_0, &temp_bp);
	if(tr == NULL) return -1;
	LOGD(LOG_TARGET, " - CLI CMD PARSE : no. [%d]\n", atoi(tr));
	// 번호 : 
	cli_cmd[i].cli_id = atoi(tr);
	
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	// cli cmd
	LOGD(LOG_TARGET, " - CLI CMD PARSE : cmd [%s]\n", tr);
	strncpy(cli_cmd[i].cli_cmd, tr, strlen(tr));
	//LOGD(LOG_TARGET, " - SRV - SDR Ret : pkt_send_interval_sec [%d]\n", tmp_server_policy.pkt_send_interval_sec);
	
	// argument 는 있을수도있고 없을 수도있다. 없을경우 바로 NULL 로 ..
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) 
	{
		LOGD(LOG_TARGET, " - CLI CMD PARSE : arg is null\n");
		memset(&cli_cmd[i].cli_arg, 0x00, sizeof(cli_cmd[i].cli_arg));
		//return -1;		
	}
	else
	{
		LOGD(LOG_TARGET, " - CLI CMD PARSE : arg [%s]\n", tr);
		// cli argument
		strncpy(cli_cmd[i].cli_arg, tr, strlen(tr));
	}
	
	cli_cmd[i].cmd_stat = e_CLI_CMD_CMD_RECV;
	/*
	printf("--------- %s() : CLI RUN INFO start ---------------------\r\n", __func__);
	printf("cli_cmd[%d].cmd_stat = [%d]\r\n",i, cli_cmd[i].cmd_stat);
	printf("cli_cmd[%d].cmd_result = [%c]\r\n",i, cli_cmd[i].cmd_result);
	printf("cli_cmd[%d].cli_id = [%d]\r\n",i, cli_cmd[i].cli_id);
	printf("cli_cmd[%d].cli_cmd = [%s]\r\n",i, cli_cmd[i].cli_cmd);
	printf("cli_cmd[%d].cli_arg = [%s]\r\n",i, cli_cmd[i].cli_arg);
	printf("--------- %s() : CLI RUN INFO end ---------------------\r\n", __func__);
	*/
	return 0;
}

char char_to_num(char ch)
{
	char ch_num = 0;
	
	// '0' is 48
	if (( ch >= '0' ) && ( ch <= '9' ))
		ch_num = ch - 48;
	// 'a' is 97
	else if (( ch >= 'a' ) && ( ch <= 'z' ))
		ch_num = ch - 97 + 10;
	// 'A' is 65
	else if (( ch >= 'A' ) && ( ch <= 'Z' ))
		ch_num = ch - 65 + 10;
	else 
		ch_num = 0;
	
	//printf("%s() [%c]=>[%d]\r\n",__func__, ch, ch_num);
	
	return ch_num;
}


int _str_replace_char ( const char *string, const char *substr, char replacement , char* buff, int buff_len)
{
	int total_len = 0;
	
	int input_len = strlen(string);
	int substr_len = strlen(substr);
	int replacement_len = 1;
	
	int replace_offset_start = 0;
	int replace_offset_end = 0;
	
	char* tok;
	// check argument
	if ( substr == NULL)
	{
		//LOGE(LOG_TARGET, "%s: Argument is null!! \r\n",__FUNCTION__);
		return -1;
	}
	
	// check buff size
	total_len = input_len - substr_len + replacement_len + 1 ;
	//printf("total_len is [%d]\r\n",total_len);
	
	if ( total_len > buff_len  )
	{
		//LOGE(LOG_TARGET, "%s: buffer size is too small!! \r\n",__FUNCTION__);
		return -1;
	}
	
	// find substr index
	tok = strstr ( string, substr );
	if (tok == NULL)
		return 2;
	
	replace_offset_start = input_len - strlen(tok);
	replace_offset_end = replace_offset_start + substr_len;
	
	memset(buff, 0x00, buff_len);
	//check buff len
	strncpy( buff, string, replace_offset_start);
	strncpy( buff + replace_offset_start, &replacement, replacement_len );
	strncpy( buff + replace_offset_start + replacement_len , string + replace_offset_end, input_len - replace_offset_end);
	
	return 0;
	
}



static int _count_char(const char* str, int str_len, const char c)
{
	int count = 0;
	int i = 0;
	
	for (i = 0 ; i < str_len ; i++ )
	{
		if ( str[i] == c )
			count ++;
	}
	
	printf("count_char ret is [%d]\r\n", count);
	return count;
}

int run_cli_cmd()
{
	int i = 0;
	int j = 0;
	int k = 0;
	
	int found = 0;
	
	// 먼저 받은 cli cmd 를 처리한다.
	for(i = 0; i < DEFAULT_FMS_MAX_CLI_CMD_CNT ; i ++ )
	{
		if ( cli_cmd[i].cmd_stat == e_CLI_CMD_CMD_RECV )
		{
			found = 1;
			break;
		}
	}
	
	if ( found == 0 ) 
		return -1;
	
	// 최초에는 그냥 RUNNING 상태로 바꿔놓는다.
	cli_cmd[i].cmd_stat = e_CLI_CMD_RUN_SUCCESS;
	cli_cmd[i].cmd_result = e_CLI_RET_RUNNING;
	
	LOGD(LOG_TARGET, "--------- %s() : CLI RUN INFO start ---------------------\r\n", __func__);
	LOGD(LOG_TARGET, "cli_cmd[%d].cmd_stat = [%d]\r\n",i, cli_cmd[i].cmd_stat);
	LOGD(LOG_TARGET, "cli_cmd[%d].cmd_result = [%c]\r\n",i, cli_cmd[i].cmd_result);
	LOGD(LOG_TARGET, "cli_cmd[%d].cli_id = [%d]\r\n",i, cli_cmd[i].cli_id);
	LOGD(LOG_TARGET, "cli_cmd[%d].cli_cmd = [%s]\r\n",i, cli_cmd[i].cli_cmd);
	LOGD(LOG_TARGET, "cli_cmd[%d].cli_arg = [%s]\r\n",i, cli_cmd[i].cli_arg);
	LOGD(LOG_TARGET, "--------- %s() : CLI RUN INFO end ---------------------\r\n", __func__);
	
	// ----------------------------------------------------------
	// 0.0.1 ==> apn 값 변경
	// ----------------------------------------------------------
	if ( strncmp(cli_cmd[i].cli_cmd, "0.0.1", strlen("0.0.1")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : Change APN [%s]\r\n", cli_cmd[i].cli_arg);
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
		//at_set_apn(cli_cmd[i].cli_arg);
		at_set_apn_addr(1, AT_APN_IP_TYPE_IPV6, cli_cmd[i].cli_arg);
	}	
	// ----------------------------------------------------------
	// 0.0.2 ==> QOS Upload 변경
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.2", strlen("0.0.2")) == 0 )
	{
		// argument check..
		int qos_maxupload = atoi(cli_cmd[i].cli_arg);
		
		LOGI(LOG_TARGET, "CLI CMD : QOS Upload [%s]\r\n", cli_cmd[i].cli_arg);
		
		if ( ( qos_maxupload > 0 ) && ( at_set_qos_info(qos_maxupload, 0) == 0 ) ) 
				cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
			else
				cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
		
	}
	// ----------------------------------------------------------
	// 0.0.3 ==> QOS Download 변경
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.3", strlen("0.0.3")) == 0 )
	{
		int qos_maxdownload = atoi(cli_cmd[i].cli_arg);
		
		LOGI(LOG_TARGET, "CLI CMD : QOS Download [%s]\r\n", cli_cmd[i].cli_arg);
		
		if ( ( qos_maxdownload > 0 ) && ( at_set_qos_info(0, qos_maxdownload) == 0 ))
			cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
		else
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
	}
	// ----------------------------------------------------------
	// 0.0.4 ==> 서버 도메인 변경
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.4", strlen("0.0.4")) == 0 )
	{
		char url_char[128] = {0,};
		char tmp_char[128] = {0,};
		
		char* p_tmp_buff;
		int cpy_to_len = 0;
		
		char *tr;
		char token_0[ ] = "/:\r\n";

		char *temp_bp = NULL;
	
		LOGI(LOG_TARGET, "CLI CMD : Change server [%s]\r\n", cli_cmd[i].cli_arg);
		
		strcpy(tmp_char,cli_cmd[i].cli_arg);
		// 125.132.73.14:4080/TFM_CT/VKTI000001/vid-rp  보냈을때 실제 리턴은 다음과 같다.
		//   --> 125.132.73.14%3A4080%2FTFM_CT%2FVKTI000001%2Fvid-rp
		// utf 문자들을 변경시킨다.
		while ( _str_replace_char(tmp_char, "%3A", ':', url_char, 128) != 2)
		{
			//printf(" url_char [%s]\r\n", url_char);
			//printf(" tmp_char [%s]\r\n", tmp_char);
			memset(tmp_char, 0x00, 128);
			strcpy(tmp_char, url_char);
		}
		
		while ( _str_replace_char(tmp_char, "%2F", '/', url_char, 128) != 2)
		{
			//printf(" url_char [%s]\r\n", url_char);
			//printf(" tmp_char [%s]\r\n", tmp_char);
			memset(tmp_char, 0x00, 128);
			strcpy(tmp_char, url_char);
		}
		
		// 여기까지왔으면다 변화시킴.
		LOGI(LOG_TARGET, "CLI CMD : Change server ==> convert 1 [%s]\r\n", url_char);
		
		// 끝에 "/" 문자가있는지 확인하고 없앰.
		cpy_to_len = strlen(url_char);
		
		if ( url_char[cpy_to_len-1] == '/' )
			cpy_to_len--;
		
		memset(tmp_char, 0x00, 128);
		strncpy(tmp_char, url_char, cpy_to_len);
		memset( url_char, 0x00, 128);
		
		// 앞에 http 주소 붙어있는지.. 확인하고 없앰.
		if ( ( p_tmp_buff = strcasestr(tmp_char, "http://") ) == NULL )
		{
			strncpy(url_char, p_tmp_buff, strlen(tmp_char) );
		}
		else
		{
			int cpy_len = strlen(tmp_char) - strlen("http://");
			p_tmp_buff += strlen("http://");
			
			strncpy(url_char, p_tmp_buff, cpy_len);
		}
		
		// 여기까지왔으면 다변화시킴
		// 125.132.73.14:4080/TFM_CT/VKTI000001/vid-rp
		
		// 문자열검사를 한다.
		if ( _count_char(url_char, strlen(url_char), '/') != 3 )
		{
			LOGE(LOG_TARGET, "CLI CMD : Change server : FAIL [%s] \r\n", url_char);\
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
			return -1;
		}
		
		if ( _count_char(url_char, strlen(url_char), ':') != 1 )		
		{
			
			LOGE(LOG_TARGET, "CLI CMD : Change server : FAIL [%s] \r\n", url_char);
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
			return -1;
		}		
		
		// 위와같은 형식만 남아있다.
		LOGI(LOG_TARGET, "CLI CMD : Change server ==> convert 2 [%s]\r\n", url_char);
		
		p_tmp_buff = url_char;
		
		tr = strtok_r(p_tmp_buff, token_0, &temp_bp);
		if(tr == NULL) 
		{
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
			return -1;
		}
		// server addr 
		set_server_ip(tr);
		printf(" >>>>>>>> server addr  [%s]\r\n", tr);
		
		tr = strtok_r(NULL, token_0, &temp_bp);
		if(tr == NULL) 
		{
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
			return -1;
		}
		// server addr 
		set_server_port(atoi(tr));
		printf(" >>>>>>>> server port  [%s]\r\n", tr);
		
		tr = strtok_r(NULL, "", &temp_bp);
		if(tr == NULL) 
		{
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
			return -1;
		}
		
		// server addr 
		set_car_info_url_path(tr);
		printf(" >>>>>>>> server internal url [%s]\r\n", tr);
		
		
		// 서버주소가 변경되었기 때문에 dns 를 변경시킨다.
		
		nettool_init_hostbyname_func();
		
		//set_server_ip(cli_cmd[i].cli_arg);
		//set_server_ip("125.132.73.14");
		set_change_server(1);
		
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.0.5 ==> 펌웨어 다운로드 서버 도메인 설정
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.5", strlen("0.0.5")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : Change dev Firm server [%s]\r\n", cli_cmd[i].cli_arg);
		
		//set_server_ip(cli_cmd[i].cli_arg);
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.0.6 ==> 사업자(법인) 등록번호 설정
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.6", strlen("0.0.6")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : business num [%s]\r\n", cli_cmd[i].cli_arg);
		
		save_car_info_user_business_no(cli_cmd[i].cli_arg);
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.0.7 ==> 차량번호 설정
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.7", strlen("0.0.7")) == 0 )
	{
		unsigned char recv_char_no[128] = {0,};
		
		for(j = 0 ; cli_cmd[i].cli_arg[j] != NULL ; j ++)
		{
			// 일단 한글은 "%xx" 이런식으로 표현된다.
			// 이럴경우.. 즉 2바이트의 문자열을 hex값으로 변환하여. 한개문자열로 표현해야한다.
			// - 예를들면... '%','A','B' 라고 한다면.. ==> 0xAB 로 표현해야한다는것이다.
			if ( cli_cmd[i].cli_arg[j] == '%' )
			{
				int tmp_val = 0;
				
				tmp_val += char_to_num(cli_cmd[i].cli_arg[++j]) * 16 ;
				//printf("tmp_val is [0x%x]\r\n", tmp_val);
				
				tmp_val += char_to_num(cli_cmd[i].cli_arg[++j]) ;
				//printf("tmp_val is [0x%x]\r\n", tmp_val);
				
				recv_char_no[k++] = tmp_val;
			}
			else
				recv_char_no[k++] = cli_cmd[i].cli_arg[j];
			
			// 차량번호..
			//printf("car no is [%s]\r\n",char_no);
			save_car_info_car_no(recv_char_no);
		}
		
		LOGI(LOG_TARGET, "CLI CMD : change car num [%s]\r\n", recv_char_no);
		
		//cli_cmd[i].cli_arg[k] = ;
		//printf(" --------------- car no start ------------------------\r\n");
		//printf("[%s]\r\n",recv_char_no);
		//printf(" --------------- car no end ------------------------\r\n");
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.0.8 ==> 운전자 ID 설정
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.8", strlen("0.0.8")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : change driver num [%s]\r\n", cli_cmd[i].cli_arg);
		
		save_car_info_driver_id(cli_cmd[i].cli_arg);
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.0.9 ==> FOTA NOTIFICATION : 개발사 자체포멧
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.0.9", strlen("0.0.9")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : modem fota [%s]\r\n");
		dmmgr_send(eEVENT_UPDATE, NULL, 0);
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.2.1 ==> 누적운행거리 설정
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.2.1", strlen("0.2.1")) == 0 )
	{
		int trip = atoi(cli_cmd[i].cli_arg);
		long long convert_trip;
		
		LOGI(LOG_TARGET, "CLI CMD : total mileage setting [%s]\r\n", cli_cmd[i].cli_arg);
		
		// int 를 형변환한다.
		if (trip >= 0)
			convert_trip = trip;
		
		if ( req_trip_setting(convert_trip) == 0)
			cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
		else
			cli_cmd[i].cmd_result = e_CLI_RET_FAIL;
		
		// 기존 daliy 운행거리 초기화
		
	}
	// ----------------------------------------------------------
	// 0.1.0 ==> 리부팅 : 단말과 연결장치들 모두 리부팅
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.1.0", strlen("0.1.0")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : reboot case 1\r\n");
		//devel_webdm_send_log("CLI CMD : REQ REBOOT ");
		
		//devel_webdm_send_log("OBD KEY STAT - ON");
		//poweroff("CLI CMD : REQ REBOOT 1", strlen("CLI CMD : REQ REBOOT 1"));
		
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	// ----------------------------------------------------------
	// 0.1.1 ==> 리부팅 : OBD + 확장모듈만 리부팅
	// ----------------------------------------------------------
	else if ( strncmp(cli_cmd[i].cli_cmd, "0.1.1", strlen("0.1.1")) == 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : reboot case 1\r\n");
		// devel_webdm_send_log("CLI CMD : REQ REBOOT ");
		//poweroff("CLI CMD : REQ REBOOT 2", strlen("CLI CMD : REQ REBOOT 2"));
		cli_cmd[i].cmd_result = e_CLI_RET_SUCCESS;
	}
	else
	{
		LOGE(LOG_TARGET, "CLI CMD : Not support cmd [%s]\r\n", cli_cmd[i].cli_cmd);
	}
	
	return 0;
}



int get_cli_cmd_stat(char* buff)
{
	int i = 0;
	int found = 0;
	int cmd_len = 0;
	//char tmp_buff[128] = {0,};
	
	// 먼저 받은 cli cmd 를 처리한다.
	for(i = 0; i < DEFAULT_FMS_MAX_CLI_CMD_CNT ; i ++ )
	{
		if ( cli_cmd[i].cmd_stat == e_CLI_CMD_RUN_SUCCESS )
		{
			found = 1;
			break;
		}
	}
	
	if ( found == 0 )
		return -1;
	
	printf("--------- %s() : CLI RUN INFO start ---------------------\r\n", __func__);
	printf("cli_cmd[%d].cmd_stat = [%d]\r\n",i, cli_cmd[i].cmd_stat);
	printf("cli_cmd[%d].cmd_result = [%c]\r\n",i, cli_cmd[i].cmd_result);
	printf("cli_cmd[%d].cli_id = [%d]\r\n",i, cli_cmd[i].cli_id);
	printf("cli_cmd[%d].cli_cmd = [%s]\r\n",i, cli_cmd[i].cli_cmd);
	printf("cli_cmd[%d].cli_arg = [%s]\r\n",i, cli_cmd[i].cli_arg);
	printf("--------- %s() : CLI RUN INFO end ---------------------\r\n", __func__);
	
	
	cmd_len += sprintf(buff + cmd_len, "%d|%c|OK", cli_cmd[i].cli_id, cli_cmd[i].cmd_result);
	
	// % 문자를 sprintf 로 돌리면 항상 fail 나는듯 싶다.
	buff[cmd_len++] = '%';
	buff[cmd_len++] = '2';
	buff[cmd_len++] = '1';
	buff[cmd_len] = '\0';
	
	//printf("%s() -> cli cmd ret is [%d] / %d\r\n", __func__, buff, cmd_len);
	if (( cli_cmd[i].cmd_result == e_CLI_RET_FAIL ) || ( cli_cmd[i].cmd_result == e_CLI_RET_SUCCESS ))
	{
		if ( strncmp(cli_cmd[i].cli_cmd, "0.1.0", strlen("0.1.0")) == 0 )
			g_cli_req_reset = 1;
		if ( strncmp(cli_cmd[i].cli_cmd, "0.1.1", strlen("0.1.1")) == 0 )
			g_cli_req_reset = 1;
		
		cli_cmd[i].cmd_stat = e_CLI_RET_NONE;
	}
	
	return cmd_len;
	
}

int get_cli_cmd_reset_stat()
{
	return g_cli_req_reset;
}



int get_fms_car_info(fms_car_info_t* car_info)
{
	get_car_info_user_business_no(car_info->user_business_no);
	//printf("%s() car_info->user_business_no is [%s]\r\n",__func__, car_info->user_business_no);
	get_car_info_car_vin(car_info->car_vin);
	//printf("%s() car_info->car_vin is [%s]\r\n",__func__, car_info->car_vin);
	get_car_info_car_no(car_info->car_no);
//	printf("%s() car_info->car_no is [%s]\r\n",__func__, car_info->car_no);
	get_car_info_driver_id(car_info->driver_id);
//	printf("%s() car_info->driver_id is [%s]\r\n",__func__, car_info->driver_id);
	return 0;
}

// -----------------------------------------
// daily info.
// -----------------------------------------
int init_car_daily_info()
{
	daliy_car_info_t tmp_daliy_car_info = {0,};
	int ret;
	
	memset(&g_daliy_car_info, 0x00, sizeof(daliy_car_info_t));
	
	ret = storage_load_file(CAR_DAILY_INFO, &tmp_daliy_car_info, sizeof(daliy_car_info_t));
	
	if( ret >= 0 )
	{
		memcpy(&g_daliy_car_info, &tmp_daliy_car_info,  sizeof(daliy_car_info_t));
	}
	else
	{
		memset(&g_daliy_car_info, 0x00, sizeof(daliy_car_info_t));
	}
	
	printf(" --------- car daliy info ---------------\r\n");
	printf("g_daliy_car_info.yyyymmdd = [%d]\r\n",g_daliy_car_info.yyyymmdd);
	printf("g_daliy_car_info.trip = [%lld]\r\n",g_daliy_car_info.trip);
	printf("g_daliy_car_info.fuel = [%d]\r\n",g_daliy_car_info.fuel);
	printf(" --------- car daliy info ---------------\r\n");
	
	return 0;
}


int save_car_daliy_info(int yymmdd, long long trip, int fuel)
{
	if (fuel <= 0)
		fuel = 0;

	if (trip <= 0)
		trip = 0;
	
	//if (  yymmdd > g_daliy_car_info.yyyymmdd ) 
	{
		g_daliy_car_info.yyyymmdd = yymmdd;
		g_daliy_car_info.trip = trip;
		g_daliy_car_info.fuel = fuel;
		
		if ( init_server_routine() == 0 )
			storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	return 0;
}


long long get_daliy_trip(long long cur_trip)
{
	static long long last_daliy_trip = 0;
	long long cur_daliy_trip = 0;
	
	if ( cur_trip < 0 )
	{
		LOGE(LOG_TARGET, "Daily trip : cur is wrong\r\n");
		return last_daliy_trip;
	}
	
	if ( g_daliy_car_info.trip == -1 )
	{
		LOGD(LOG_TARGET, "Daily trip : init seq [%lld] \r\n", cur_trip);
		LOGD(LOG_TARGET, "Daily trip : init seq [%lld] \r\n", cur_trip);
		LOGD(LOG_TARGET, "Daily trip : init seq [%lld] \r\n", cur_trip);
		LOGD(LOG_TARGET, "Daily trip : init seq [%lld] \r\n", cur_trip);
		
		g_daliy_car_info.trip = cur_trip;
		last_daliy_trip = 0;
		if ( init_server_routine() == 0 )
			storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	if ( g_daliy_car_info.trip == 0 )
	{
		g_daliy_car_info.trip = cur_trip;
		last_daliy_trip = 0;
		if ( init_server_routine() == 0 )
			storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
		return 0;
	}
	
	if ( g_daliy_car_info.trip > cur_trip)
	{
		g_daliy_car_info.trip = -1;
		
		return last_daliy_trip;
	}
	
	cur_daliy_trip = cur_trip - g_daliy_car_info.trip;
	
	printf(" \t --> get trip info [%lld] / [%lld] / [%lld] / [%lld] \r\n", cur_trip, g_daliy_car_info.trip, cur_daliy_trip, last_daliy_trip);
	
	/*
	// TODO : daliy_trip 은 뒤로가면 안된다!!
	if ( cur_daliy_trip < last_daliy_trip )
	{
		LOGE(LOG_TARGET, "Daily trip calc is wrong [%lld] / [%lld]\r\n", cur_daliy_trip, last_daliy_trip);
		cur_daliy_trip = last_daliy_trip;
	}
	*/
	
	last_daliy_trip = cur_daliy_trip;
	//printf("%s() daliy trip [%lld]\r\n", __func__, cur_daliy_trip);
	
	return cur_daliy_trip;
}


long long set_daliy_trip(long long cur_trip)
{
	// -1 이 오면, 일단 저장하지 않고 있다가 최초 trip 들어올때 저장한다.
	printf("%s() ==> [%lld]\r\n", __func__, cur_trip);
	
	if ( cur_trip < 0)
	{
		printf("%s() case 1 ==> [%lld]\r\n", __func__,cur_trip);
		g_daliy_car_info.trip = -1;
		return 0;
	}

	g_daliy_car_info.trip = cur_trip;
	
	if ( init_server_routine() == 0 )
		storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	
	return 0;
}


int get_daliy_fuel(int cur_fuel)
{
	static unsigned int last_daliy_fuel = 0;
	unsigned int cur_daliy_fuel = 0;
	
	if ( cur_fuel < 0 )
	{
		LOGE(LOG_TARGET, "Daily fuel : cur is wrong\r\n");
		return last_daliy_fuel;
	}
	
	if ( g_daliy_car_info.fuel == -1)
	{
		LOGD(LOG_TARGET, "Daily fuel : init seq [%lld] \r\n", cur_fuel);
		LOGD(LOG_TARGET, "Daily fuel : init seq [%lld] \r\n", cur_fuel);
		LOGD(LOG_TARGET, "Daily fuel : init seq [%lld] \r\n", cur_fuel);
		LOGD(LOG_TARGET, "Daily fuel : init seq [%lld] \r\n", cur_fuel);
		
		g_daliy_car_info.fuel = cur_fuel;
		last_daliy_fuel = 0;
		
		if ( init_server_routine() == 0 )
			storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	if ( g_daliy_car_info.fuel == 0 )
	{
		g_daliy_car_info.fuel = cur_fuel;
		last_daliy_fuel = 0;
		
		if ( init_server_routine() == 0 )
			storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
		return 0;
	}
	
	if ( g_daliy_car_info.fuel > cur_fuel)
	{
		g_daliy_car_info.fuel = -1;
		
		return last_daliy_fuel;
	}
	
	
	cur_daliy_fuel = cur_fuel - g_daliy_car_info.fuel;
	
	printf(" \t --> get fuel info [%d] / [%d] / [%d] / [%d] \r\n", cur_fuel, g_daliy_car_info.fuel, cur_fuel, last_daliy_fuel);
	
	
	/*
	// TODO : daliy_trip 은 뒤로가면 안된다!!
	if ( cur_daliy_fuel < last_daliy_fuel )
	{
		LOGE(LOG_TARGET, "Daily trip calc is wrong [%lld] / [%lld]\r\n", cur_daliy_fuel, last_daliy_fuel);
		cur_daliy_fuel = last_daliy_fuel;
	}
	*/
	
	last_daliy_fuel = cur_daliy_fuel;
	
	return cur_daliy_fuel;
}

long long set_daliy_fuel(int cur_fuel)
{
	if ( cur_fuel < 0)
	{
		g_daliy_car_info.fuel = -1;
		return 0;
	}

	g_daliy_car_info.fuel = cur_fuel;

	if ( init_server_routine() == 0 )
		storage_save_file(CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	
	return 0;
}

// 년월일만 리턴한다.

int get_cur_daily_date_num()
{
	//printf("%s() => [%d]\r\n",__func__, g_daliy_car_info.yyyymmdd);
	return g_daliy_car_info.yyyymmdd;
}


int clr_daily_info()
{
	long long set_trip = 0;
	int set_fuel = 0;
	
	
	set_trip = -1;
	set_fuel = -1;
	
	memset(&g_daliy_car_info, 0x00, sizeof(daliy_car_info_t));
	
	g_daliy_car_info.trip = set_trip;
	g_daliy_car_info.fuel = set_fuel;
	
	printf("clear daily info [%lld] / [%d] !!!!!!!!!!!!!!\r\n", g_daliy_car_info.trip, g_daliy_car_info.fuel);
	printf("clear daily info [%lld] / [%d] !!!!!!!!!!!!!!\r\n", g_daliy_car_info.trip, g_daliy_car_info.fuel);
	printf("clear daily info [%lld] / [%d] !!!!!!!!!!!!!!\r\n", g_daliy_car_info.trip, g_daliy_car_info.fuel);
	printf("clear daily info [%lld] / [%d] !!!!!!!!!!!!!!\r\n", g_daliy_car_info.trip, g_daliy_car_info.fuel);	
		
	remove(CAR_DAILY_INFO);
	remove(CAR_DAILY_INFO2);
	
	return 0;
}





int set_server_policy_stat(e_FMS_SVR_STAT stat)
{
	g_ktfms_server_policy.policy_stat = stat;
	return g_ktfms_server_policy.policy_stat;
}



int get_runtime_server_policy(server_policy_t* policy)
{
	memcpy(policy, &g_runtime_policy, sizeof(server_policy_t));
	/*
	policy->pkt_send_interval_sec = 30;
	policy->pkt_collect_interval_sec = 1;
	*/
	return 0;
}

int set_runtime_server_policy()
{
	memcpy(&g_runtime_policy, &g_ktfms_server_policy, sizeof(server_policy_t));
	return 0;
}

#define MAX_HW_ERR_CODE			10
#define NOT_DEFINED_ERR_CODE	-1

int hw_err_code[10] = {0,};

int init_hw_err_code()
{
	int i = 0;
	
	for ( i = 0 ; i < MAX_HW_ERR_CODE ; i ++)
	{
		hw_err_code[i] = NOT_DEFINED_ERR_CODE;
	}
	return 0;
}


int set_hw_err_code(e_HW_ERR_CODE_VAL code, int flag)
{
	int i = 0;
	int found = 0;
	// set error code
	if (flag == 1)
	{
		// 기존 저장 코드 찾는다.
		for ( i = 0 ; i < MAX_HW_ERR_CODE ; i ++)
		{
			if ( hw_err_code[i] == code)
			{
				found = 1;
				break;
			}
		}
		
		if ( found == 1 ) 
			return -1;
		
		// 기존 저장 코드가 없으므로 신규 저장
		for ( i = 0 ; i < MAX_HW_ERR_CODE ; i ++)
		{
			if ( hw_err_code[i] == NOT_DEFINED_ERR_CODE)
			{
				hw_err_code[i] = code;
				break;
			}
		}
		
	}
	else // clear error code
	{
		for ( i = 0 ; i < MAX_HW_ERR_CODE ; i ++)
		{
			if ( hw_err_code[i] == code)
			{
				hw_err_code[i] = NOT_DEFINED_ERR_CODE;
				break;
			}
		}
	}
	
	return code;
}


int get_hw_err_code(char* buff)
{
	char tmp_buff[128] = {0,};
	int buff_len = 0;
	int i = 0;
	
	if ( buff == NULL )
	{
		LOGE(LOG_TARGET, "%s() : buffer is null \r\n",__func__);
		return -1;
	}
	
	for ( i = 0 ; i < MAX_HW_ERR_CODE ; i ++)
	{
		if ( hw_err_code[i] != NOT_DEFINED_ERR_CODE )
		{
			buff_len += sprintf(tmp_buff + buff_len, "%02d,", hw_err_code[i] );
		}
	}
	
	if ( buff_len > 0 )
	{
		strncpy(buff, tmp_buff, buff_len-1);
		printf("error code string is [%s]\r\n", buff);
	}
	else
		printf("error code string is null\r\n");
	
	return buff_len;
}

static int g_req_trip = -1;

int req_trip_setting(int req_trip)
{
	g_req_trip = req_trip;
	return 0;
}

int get_req_trip_setting_result()
{
	if (g_req_trip == -1)
		return 0;
	else
		return -1;
}

int proc_set_trip()
{
	long long set_trip = 0;
	int ret = -1;
	int retry_cnt = 5;
	
	int set_result = 0;
	odbGender_t cur_gender_spec = {0,};
	
	if ( g_req_trip >= 0)
	{
		set_trip = g_req_trip * 1000;
	
		while ( retry_cnt -- )
		{
			//if ( set_seco_obd_total_trip(set_trip) == OBD_RET_SUCCESS )
			if ( set_seco_obd_total_trip_fuel(set_trip, 0) == OBD_RET_SUCCESS )
			{
				LOGE(LOG_TARGET, "%s() : trip set success [%lld] \r\n",__func__, set_trip);
			
				g_req_trip = -1;
				ret = 0;

				break;
			}
			else
			{
				ret = -1;
				set_result = 0;
				LOGE(LOG_TARGET, "%s() : trip set fail \r\n",__func__);
			}
			
			sleep(1);
		}
	}
	
	return ret;
}

static int req_change_server = -1;
static int init_server_flag = 0;

int chk_change_server()
{
	return req_change_server;
}

int set_change_server(int flag)
{
	printf(" >>>> set to change server [%d] -> [%d]\r\n", req_change_server, flag);
	req_change_server = flag;
	return req_change_server;
}

int init_server_routine()
{
	return init_server_flag;
}

int init_server_and_poweroff()
{
//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
	send_device_off_packet(30);
#endif
	gps_valid_data_write();
	mileage_write();
	
	// 이렇게 강제로 변경하지 않으면.
	// 기존에 trip이나 서버정보를 초기화 하더라도 gps thread 에서 다시 만들기 때문
	init_server_flag = 1;
	set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
	
	printf(" init server info. and reboot " );
	
	
	clear_trip_seq();
	clr_fms_server_policy();
	clr_daily_info();
	clear_obd_info();
	
	while(1)
		system("poweroff");
}


int init_server_and_poweroff2()
{
#if defined (BOARD_NEO_W200K) && defined (KT_FOTA_ENABLE)
	send_device_off_packet(30);
#endif
	gps_valid_data_write();
	
	// 이렇게 강제로 변경하지 않으면.
	// 기존에 trip이나 서버정보를 초기화 하더라도 gps thread 에서 다시 만들기 때문
	init_server_flag = 1;
	set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
	
	printf(" init server info. and reboot " );
	
	
	clear_trip_seq();
	//clr_fms_server_policy();
	clr_daily_info();
	clear_obd_info();
	
	while(1)
		system("poweroff");
}



int obd_info_load_success = 0;
odbGender_t g_saved_gender_spec = {0,};

int save_obd_info(odbGender_t* p_gender_spec)
{
	storage_save_file(CAR_OBD_INFO_PATH, (void*)p_gender_spec, sizeof(odbGender_t));
	memcpy(&g_saved_gender_spec, p_gender_spec, sizeof(odbGender_t));
	return 0;
}

int load_obd_info()
{
	int ret = 0;
	odbGender_t tmp_spec = {0,};
	
	memset(&tmp_spec, 0x00, sizeof(tmp_spec));
	
	ret = storage_load_file(CAR_OBD_INFO_PATH, &tmp_spec, sizeof(odbGender_t));
	
	if(ret != ERR_NONE)
	{
		printf("%s() fail \r\n", __func__);
		obd_info_load_success = 0;
	}
	else
	{
		printf("%s() success \r\n", __func__);
		obd_info_load_success = 1;
		memcpy(&g_saved_gender_spec, &tmp_spec, sizeof(odbGender_t));		
	}
	return 0;
}

int get_obd_info(odbGender_t* p_gender_spec)
{
	if ( obd_info_load_success == 0 ) 
		return -1;
	
	memcpy(p_gender_spec, &g_saved_gender_spec, sizeof(odbGender_t));
	return 0;
	
}


int clear_obd_info()
{
	remove(CAR_OBD_INFO_PATH);
	remove(CAR_OBD_INFO_PATH2);
	return 0;
}

static int g_last_obd_stat = OBD_CMD_RET_INVALID_COND;
int set_last_obd_stat(int stat)
{
	g_last_obd_stat = stat;
}

int get_last_obd_stat()
{
	return g_last_obd_stat;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <util/tools.h>
#include <logd_rpc.h>


#include "alloc2_pkt.h"
#include "alloc2_senario.h"
#include "alloc2_obd_mgr.h"
#include "alloc2_bcm_mgr.h"
#include "alloc2_daily_info.h"

typedef struct daliy_car_info
{
	int yyyymmdd;
	int total_distance;
}daliy_car_info_t;


static daliy_car_info_t 	g_daliy_car_info = {0,};

static ALLOC2_DAILY_OVERSPEED_MGR_T g_over_speed_mgr;

// -----------------------------------------
// daily info.
// -----------------------------------------

int set_total_gps_distance(int distance)
{
	mileage_set_m(distance);
	
	alloc2_clr_car_daily_info();
	save_daily_info__total_distance(-1, -1);
	init_keyon_section_distance(distance);
	init_diff_distance_prev();

	mileage_write();
}


int alloc2_init_car_daily_info()
{
	daliy_car_info_t tmp_daliy_car_info = {0,};
	int ret;
	int i = 0;
	
	memset(&g_daliy_car_info, 0x00, sizeof(daliy_car_info_t));
	
	ret = storage_load_file(ALLOC2_CAR_DAILY_INFO, &tmp_daliy_car_info, sizeof(daliy_car_info_t));
	
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
	printf("g_daliy_car_info.trip = [%d]\r\n",g_daliy_car_info.total_distance);
	printf(" --------- car daliy info ---------------\r\n");
	
	memset(&g_over_speed_mgr, 0x00, sizeof(g_over_speed_mgr));
		
	return 0;
}

int alloc2_clr_car_daily_info()
{
	memset(&g_daliy_car_info, 0x00, sizeof(daliy_car_info_t));
	g_daliy_car_info.total_distance = -1;
	return 0;
}

int save_daily_info__total_distance(int yyyymmdd, int total_distance)
{
	if ( yyyymmdd > g_daliy_car_info.yyyymmdd ) 
	{
		g_daliy_car_info.yyyymmdd = yyyymmdd;
		g_daliy_car_info.total_distance = total_distance;
		
		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
		printf("save!!!!! dail info!!!\r\n");
	}

	if ( yyyymmdd == -1 )
	{
		g_daliy_car_info.yyyymmdd = 0;
		g_daliy_car_info.total_distance = total_distance;
	}
	//printf("set daily info :: g_daliy_car_info.yyyymmdd is [%d]\r\n", g_daliy_car_info.yyyymmdd);
	//printf("set daily info :: g_daliy_car_info.total_distance is [%d]\r\n", g_daliy_car_info.total_distance);

	return g_daliy_car_info.total_distance;
}

int get_daily_info__daily_distance(int total_disatance)
{
	int daily_distance = 0;
	static int saved_daily_distance = 0;

	//if ( total_disatance < g_daliy_car_info.total_distance)
	//	 total_disatance = g_daliy_car_info.total_distance;

	if ( g_daliy_car_info.total_distance == -1)
	{
		g_daliy_car_info.total_distance = total_disatance;
		g_daliy_car_info.yyyymmdd = 0;
	}

	daily_distance = total_disatance - g_daliy_car_info.total_distance;

	if ( ( daily_distance >= 0 ) && ( daily_distance >= saved_daily_distance) )
	{
		saved_daily_distance = daily_distance;
	}
	else
	{
		daily_distance = saved_daily_distance;
	}

	//printf("get daily info :: g_daliy_car_info.yyyymmdd is [%d]\r\n", g_daliy_car_info.yyyymmdd);
	//printf("get daily info :: g_daliy_car_info.total_distance is [%d]\r\n", g_daliy_car_info.total_distance);

	//printf("get daily info :: daily_distance is [%d]\r\n", daily_distance);

	return daily_distance;
}



static int _g_keyon_distance = 0;
static int _g_last_keyon_distance = 0;

int chk_keyon_section_distance(int total_distance)
{
	int ret_val = 0;

	// 기준데이터가 없을경우 다시 세팅
	if ( _g_keyon_distance <= 0 )
		_g_keyon_distance = total_distance;

	// 누적거리가 역방향 계산일경우 다시 세팅
	if ( total_distance < _g_keyon_distance )
		_g_keyon_distance = total_distance;;

	ret_val =  total_distance - _g_keyon_distance;
	
	// 마지막에 전송한 값보다 작을경우 마지막에 전송한 값을다시 전송
	if (ret_val < _g_last_keyon_distance )
		ret_val = _g_last_keyon_distance;

	_g_last_keyon_distance = ret_val;
    return ret_val;
}

int init_keyon_section_distance(int total_distance)
{
	_g_keyon_distance = total_distance;
	_g_last_keyon_distance = 0;
	get_diff_distance_prev(total_distance);
    return 0;
}



static int _g_diff_distance = 0;
int get_diff_distance_prev(int total_distance)
{
	int ret_val = 0;

	if ( _g_diff_distance <= 0 )
		_g_diff_distance = total_distance;

	if ( total_distance < _g_diff_distance )
		_g_diff_distance = total_distance;

	ret_val = total_distance - _g_diff_distance;

	printf("  >>  get_diff_distance_prev -> _g_diff_distance [%d] / total_distance [%d] / return [%d] \r\n", _g_diff_distance, total_distance, ret_val);
	
	_g_diff_distance = total_distance;

	return ret_val;
}

int init_diff_distance_prev()
{
    _g_diff_distance = 0;
    return 0;
}

// 본 펑셔는 초당 불려야한다.
int set_overspeed_info(gpsData_t* gps_info)
{
	int use_obd_flag = 0;
	int cur_speed = 0;

	static int saved_over_speed_cnt = 0;

	int setting_over_speed;
	int setting_over_speed_cnt;

	SECO_OBD_DATA_T cur_obd_data;

	ALLOC_PKT_RECV__MDM_SETTING_VAL* cur_mdm_setting = get_mdm_setting_val();

	memset(&cur_obd_data, 0x00, sizeof(cur_obd_data));

	if ( cur_mdm_setting == NULL)
		return ALLOC2_DAILY_INFO_FAIL;
	
	if ( gps_info == NULL )
		return ALLOC2_DAILY_INFO_FAIL;

	setting_over_speed = cur_mdm_setting->over_speed_limit_km;
	setting_over_speed_cnt = cur_mdm_setting->over_speed_limit_time;

	alloc2_obd_mgr__get_cur_obd_data(&cur_obd_data);

	// obd 속도가 있으면 obd 속도계를 사용한다.
	if ( cur_obd_data.obd_data_car_speed > 0 )
		use_obd_flag = 1;
	
	if ( use_obd_flag == 1 )
		cur_speed = cur_obd_data.obd_data_car_speed;
	else
		cur_speed = gps_info->speed;
	
	/*
	printf("cur over speed info :: o-spd [%d] , g-spd [%d], spd [%d]\r\n",
			cur_obd_data.obd_data_car_speed,  gps_info->speed, 
			setting_over_speed);

	printf("cur over speed info :: cur cnt [%d]/[%d] , todo send cnt [%d]\r\n",
			saved_over_speed_cnt, setting_over_speed_cnt, 
			g_over_speed_mgr.over_speed_cnt);
	*/
	if ( (setting_over_speed > 0 ) && ( cur_speed > setting_over_speed ) )
		saved_over_speed_cnt++;
	else
		saved_over_speed_cnt = 0;

	if ( ( setting_over_speed_cnt > 0 ) && ( saved_over_speed_cnt > setting_over_speed_cnt ))
	{
		g_over_speed_mgr.over_speed_cnt ++;
		saved_over_speed_cnt = 0;
	}

	return ALLOC2_DAILY_INFO_SUCCESS;
	
}

int get_overspeed_info(ALLOC2_DAILY_OVERSPEED_MGR_T* over_speed_info)
{
	over_speed_info->over_speed_cnt = g_over_speed_mgr.over_speed_cnt;

	g_over_speed_mgr.over_speed_cnt = 0;
	return ALLOC2_DAILY_INFO_SUCCESS;
}


#if 0

int alloc2_save_car_daliy_info(int yymmdd, long long trip, int fuel)
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
		
		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	return 0;
}


long long alloc2_get_daliy_trip(long long cur_trip)
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

		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	if ( g_daliy_car_info.trip == 0 )
	{
		g_daliy_car_info.trip = cur_trip;
		last_daliy_trip = 0;

		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
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


long long alloc2_set_daliy_trip(long long cur_trip)
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
	
	storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	
	return 0;
}


int alloc2_get_daliy_fuel(int cur_fuel)
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
		
		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	}
	
	if ( g_daliy_car_info.fuel == 0 )
	{
		g_daliy_car_info.fuel = cur_fuel;
		last_daliy_fuel = 0;
		
		storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
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

long long alloc2_set_daliy_fuel(int cur_fuel)
{
	if ( cur_fuel < 0)
	{
		g_daliy_car_info.fuel = -1;
		return 0;
	}

	g_daliy_car_info.fuel = cur_fuel;

	storage_save_file(ALLOC2_CAR_DAILY_INFO, (void*)&g_daliy_car_info, sizeof(g_daliy_car_info));
	
	return 0;
}

// 년월일만 리턴한다.

int alloc2_get_cur_daily_date_num()
{
	//printf("%s() => [%d]\r\n",__func__, g_daliy_car_info.yyyymmdd);
	return g_daliy_car_info.yyyymmdd;
}


int alloc2_clr_daily_info()
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
		
	remove(ALLOC2_CAR_DAILY_INFO);
	remove(ALLOC2_CAR_DAILY_INFO2);
	
	return 0;
}



#endif


void save_resume_data()
{
	int try_cnt = 4;
	ALLOC_RESUME_DATA resume_data = {0,};

	resume_data.keyon_distance = _g_keyon_distance;
	resume_data.diff_last_distance = _g_diff_distance;

	while(try_cnt--)
	{
		if( storage_save_file(NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH, &resume_data, sizeof(resume_data)) >= 0)
			break;
	}

}

void load_resume_data()
{
	ALLOC_RESUME_DATA resume_data = {0,};
	int ret = 0;

	if ( storage_load_file(NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH, &resume_data, sizeof(resume_data)) >= 0 )
	{
		init_keyon_section_distance(resume_data.keyon_distance);
		_g_diff_distance = resume_data.diff_last_distance;
	}

	remove(NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH);
	remove(NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH_2);
}
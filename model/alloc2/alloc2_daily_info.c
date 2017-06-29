#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include <logd_rpc.h>

#include "alloc2_daily_info.h"

typedef struct daliy_car_info
{
	int yyyymmdd;
	int total_distance;
}daliy_car_info_t;


static daliy_car_info_t 	g_daliy_car_info = {0,};

// -----------------------------------------
// daily info.
// -----------------------------------------
int alloc2_init_car_daily_info()
{
	daliy_car_info_t tmp_daliy_car_info = {0,};
	int ret;
	
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

	daily_distance = total_disatance - g_daliy_car_info.total_distance;

	if ( ( daily_distance >= 0 ) && ( daily_distance >= saved_daily_distance) )
	{
		saved_daily_distance = daily_distance;
	}
	else
	{
		daily_distance = saved_daily_distance;
	}

	printf("get daily info :: g_daliy_car_info.yyyymmdd is [%d]\r\n", g_daliy_car_info.yyyymmdd);
	printf("get daily info :: g_daliy_car_info.total_distance is [%d]\r\n", g_daliy_car_info.total_distance);

	printf("get daily info :: daily_distance is [%d]\r\n", daily_distance);

	return daily_distance;
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
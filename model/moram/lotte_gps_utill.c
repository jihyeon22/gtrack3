#ifdef TEST_CODE_ENABLE
	#include "depend.h"
#else
	#include "config.h"
	#include <board/power.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <base/devel.h>
#include "lotte_gpsmng.h"
#include "logd/logd_rpc.h"
#include "debug.h"

#include <board/board_system.h>

int roundNo(float num)
{
    return num < 0 ? num - 0.5555 : num + 0.5555;
}

int RoundOff(float x, int dig)
{
	return floor((x) * pow(10,dig) + 0.5) / pow(10,dig);
}

double get_distance_meter(double tlat1, double tlat2, double tlon1, double tlon2)
{
	if(tlat1==0 || tlat2==0 || tlon1==0 || tlon2==0)
	{
		LOGI(LOG_TARGET, "get_distance_meter skip!");
		return -1.1;
	}
	
	if(tlat1==tlat2 && tlon1 == tlon2)
	{
		return 0;
	}

#ifdef TEST_CODE_ENABLE //for test
	return abs(tlat2 - tlat1);
#else
	double distance = ((acos(sin(tlat1 * M_PI / 180) * sin(tlat2 * M_PI / 180) + cos(tlat1 * M_PI / 180) * cos(tlat2 * M_PI / 180) * cos((tlon1 - tlon2) * M_PI / 180)) * 180 / M_PI) * 60 * 1.1515 * 1.609344);

	distance = distance*1000.0;

	if(isnan(distance) != 0)
	{
		return -1.1;
	}
	
	return distance;
#endif
}

int diff_distance(gpsData_t cur, gpsData_t prev)
{
	return get_distance_meter(cur.lat, prev.lat, cur.lon, prev.lon);
}

int get_collection_interval()
{
	int interval_time;
	int ign_on = power_get_ignition_status();
	configurationModel_t *conf = get_config_model();

	if(ign_on == POWER_IGNITION_OFF)
		interval_time = conf->model.collect_interval_keyoff;
	else
		interval_time = conf->model.collect_interval_keyon;

	//printf("collection interval_time = [%d]\n", interval_time);
	return interval_time;
}

int get_report_interval()
{
	int interval_time;
	int ign_on = power_get_ignition_status();
	configurationModel_t *conf = get_config_model();

	if(ign_on == POWER_IGNITION_OFF)
		interval_time = conf->model.report_interval_keyoff;
	else
		interval_time = conf->model.report_interval_keyon;

	//printf("report interval_time = [%d]\n", interval_time);
	return interval_time;
}

void print_skip_gps_reason(gps_condition_t gcond, gpsData_t gps)
{
	switch(gcond)
	{
		case eSKIP_NO_INITIAL_GPS:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by no initial : idx:[%d]\n", gps.num);
#else
			LOGI(LOG_TARGET, "gps data skip by no initial\n");
#endif
			break;
		case eSKIP_NO_COLLECTION_TIME:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by no collection time : idx:[%d]\n", gps.num);
#else
			//LOGD(LOG_TARGET, "gps data skip by no collection time\n");
#endif
			break;
		case eSKIP_DISTANCE_FILTER:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by DISTANCE_FILTER : idx:[%d]\n", gps.num);
#else
			LOGI(LOG_TARGET, "gps data skip by DISTANCE_FILTER\n");
#endif
			break;
		case eSKIP_TRUST_SATLITES_FILTER:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by SATLITES_FILTER : idx:[%d]\n", gps.num);
#else
			LOGI(LOG_TARGET, "gps data skip by SATLITES_FILTER\n");
#endif
			break;
		case eSKIP_TRUST_HDOP_FILTER:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by HDOP_FILTER : idx:[%d]\n", gps.num);
#else
			LOGI(LOG_TARGET, "gps data skip by HDOP_FILTER\n");
#endif
			break;
		case eSKIP_TRUST_SPEED_FILTER:
#ifdef TEST_CODE_ENABLE //for test
			printf("gps data skip by SPEED_FILTER : idx:[%d]\n", gps.num);
#else
			LOGI(LOG_TARGET, "gps data skip by SPEED_FILTER\n");
#endif
			break;
		default:
			LOGE(LOG_TARGET, "gps data skip by unkown[%d]\n", gcond);
			break;
	}
}

void utc_sec_localtime(time_t utc_sec, struct tm *gmt, int adj_hour)
{
	utc_sec += adj_hour * 60 * 60;
	gmtime_r(&utc_sec, gmt);
}

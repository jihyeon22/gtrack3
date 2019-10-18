#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include <board/power.h>

#include <board/board_system.h>

#include "gpsmng.h"
#include "logd/logd_rpc.h"
#include "debug.h"

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

	double distance = ((acos(sin(tlat1 * M_PI / 180) * sin(tlat2 * M_PI / 180) + cos(tlat1 * M_PI / 180) * cos(tlat2 * M_PI / 180) * cos((tlon1 - tlon2) * M_PI / 180)) * 180 / M_PI) * 60 * 1.1515 * 1.609344);

	distance = distance*1000.0;

	if(isnan(distance) != 0)
	{
		return -1.1;
	}
	
	return distance;
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
			LOGI(LOG_TARGET, "gps data skip by no initial\n");
			break;
		case eSKIP_NO_COLLECTION_TIME:
			//LOGD(LOG_TARGET, "gps data skip by no collection time\n");
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

#include <stdio.h>
#include "gps_filter.h"
#include "gpsmng.h"
#include "gps_utill.h"
#include "logd/logd_rpc.h"
#include "debug.h"

int distance_filter(gpsData_t cur_gps, gpsData_t prev, int val)
{
	int tm_sec = 1;
	int distance;
	if(prev.active) {
		tm_sec = difftime(cur_gps.utc_sec, prev.utc_sec);
		distance = diff_distance(cur_gps, prev);

		if(distance > val*tm_sec) {
			LOGI(LOG_TARGET, "distance_filter> distance = [%d]\n", distance);
			return 1;
		}
	}
	return 0;
}

int trust_satlites_filter(gpsData_t cur_gps, int val)
{
	
	if(cur_gps.satellite < val) {
		LOGI(LOG_TARGET, "trust_satlites_filter> satellite = [%d]\n", cur_gps.satellite);
		return 1;
	}
	
	return 0;
}
int trust_HDOP_filter(gpsData_t cur_gps, int val)
{
	if(cur_gps.hdop > val) {
		LOGI(LOG_TARGET, "trust_HDOP_filter> HDOP = [%5.3f]\n", cur_gps.hdop);
		return 1;
	}
	return 0;
}
int trust_speed_filter(gpsData_t cur_gps, int val)
{
	if(cur_gps.speed > val) {
		LOGI(LOG_TARGET, "trust_speed_filter> SPEED = [%d]\n", cur_gps.speed);
		return 1;
	}

	return 0;
}

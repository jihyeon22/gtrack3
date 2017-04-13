#pragma once

#ifdef TEST_CODE_ENABLE
#include "depend.h"
#else
#include <base/gpstool.h>
#endif

/*
#define ENABLE_DISTANCE_FILTER
#define ENABLE_TRUST_SATLITES_FILTER
#define ENABLE_TRUST_HDOP_FILTER
#define ENABLE_TRUST_SPEED_FILTER


//Filter Constant
#define FILTER_DISTANCE_ALLOW_METER	(50) //unit : Meter
#define FILTER_SATLITES_ALLOW_COUNT	(5)
#define FILTER_HDOP_ALLOW_VALUE		(5)
#define FILTER_ALLOW_SPEED			(200) //unit : Km/h
*/


int distance_filter(gpsData_t cur_gps, gpsData_t prev, int val);
int trust_satlites_filter(gpsData_t cur_gps, int val);
int trust_HDOP_filter(gpsData_t cur_gps, int val);
int trust_speed_filter(gpsData_t cur_gps, int val);

#pragma once

#include <time.h>
#include "gpsmng.h"

time_t get_device_time();
int fake_gps_time(gpsData_t *pData);
// int roundNo(float num);
// int RoundOff(float x, int dig);
double get_distance_meter(double tlat1, double tlat2, double tlon1, double tlon2);
int diff_distance(gpsData_t cur, gpsData_t prev);
int get_collection_interval();
// int get_report_interval();
void print_skip_gps_reason(gps_condition_t gcond, gpsData_t gps);
void utc_sec_localtime(time_t utc_sec, struct tm *gmt, int adj_hour);

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <util/storage.h>
#include "debug.h"

#include "gps_utill.h"
#include "geofence.h"
#include "logd/logd_rpc.h"
#include <base/gpstool.h>

int geo_fence0_wait_count = 0;
int geo_fence1_wait_count = 0;

#if defined(CORP_ABBR_NISSO)
	int geo_fence2_wait_count = 0;
	int geo_fence3_wait_count = 0;
#endif

geo_fence_hdr_t g_geo_fence = 
{
	//real code
	.fence0.enable = eGEN_FENCE_DISABLE,
	.fence1.enable = eGEN_FENCE_DISABLE,

#if defined(CORP_ABBR_NISSO)
	.fence2.enable = eGEN_FENCE_DISABLE,
	.fence3.enable = eGEN_FENCE_DISABLE,
#endif
};
void _default_geo_fence(geo_fence_hdr_t *pdata)
{
	pdata->fence0.enable             = eGEN_FENCE_DISABLE;
	pdata->fence0.latitude           = 0.0;
	pdata->fence0.longitude          = 0.0;
	pdata->fence0.range              = 0;
	pdata->fence0.cur_fence_status   = eFENCE_SETUP_UNKOWN;
	pdata->fence0.setup_fence_status = eFENCE_STATUS_UNKOWN;

	pdata->fence1.enable             = eGEN_FENCE_DISABLE;
	pdata->fence1.latitude           = 0.0;
	pdata->fence1.longitude          = 0.0;
	pdata->fence1.range              = 0;
	pdata->fence1.cur_fence_status   = eFENCE_SETUP_UNKOWN;
	pdata->fence1.setup_fence_status = eFENCE_STATUS_UNKOWN;

#if defined(CORP_ABBR_NISSO)
	pdata->fence2.enable             = eGEN_FENCE_DISABLE;
	pdata->fence2.latitude           = 0.0;
	pdata->fence2.longitude          = 0.0;
	pdata->fence2.range              = 0;
	pdata->fence2.cur_fence_status   = eFENCE_SETUP_UNKOWN;
	pdata->fence2.setup_fence_status = eFENCE_STATUS_UNKOWN;

	pdata->fence3.enable             = eGEN_FENCE_DISABLE;
	pdata->fence3.latitude           = 0.0;
	pdata->fence3.longitude          = 0.0;
	pdata->fence3.range              = 0;
	pdata->fence3.cur_fence_status   = eFENCE_SETUP_UNKOWN;
	pdata->fence3.setup_fence_status = eFENCE_STATUS_UNKOWN;
#endif
}

void _print_geo_fence_status()
{

	FILE *fp = NULL;
	FILE *log_fd = NULL;
	remove("/var/log/geofence.log");
	fp = fopen("/var/log/geofence.log", "w");
	
	if(fp == NULL)
		log_fd = stderr;
	else
		log_fd = fp;
	
	fprintf(log_fd, "[GEO FENCE #0]\n");
	switch(g_geo_fence.fence0.enable) {
		case eGEN_FENCE_DISABLE: fprintf(log_fd, "\tgeo fence0 disable\n"); break;
		case eGEN_FENCE_ENABLE: fprintf(log_fd, "\tgeo fence0 enable\n"); break;
		default: fprintf(log_fd, "\tgeo fence0 unknow enable status\n"); break;
	}
	fprintf(log_fd, "\tfence0 latitude = %06.6f\n", g_geo_fence.fence0.latitude);
	fprintf(log_fd, "\tfence0 longitude = %06.6f\n", g_geo_fence.fence0.longitude);
	fprintf(log_fd, "\tfence0 range = %d\n", g_geo_fence.fence0.range);
	

	switch(g_geo_fence.fence0.cur_fence_status) {
		case eFENCE_STATUS_UNKOWN: fprintf(log_fd, "\tgeo fence0 unkown status #1\n"); break;
		case eFENCE_STATUS_IN: fprintf(log_fd, "\tgeo fence0 IN status\n"); break;
		case eFENCE_STATUS_OUT: fprintf(log_fd, "\tgeo fence0 OUT status\n"); break;
		case eFENCE_STATUS_IGNORE: fprintf(log_fd, "\tgeo fence0 ignore status\n"); break;
		default: fprintf(log_fd, "\tgeo fence0 unknown status #2\n"); break;
	}
		
	switch(g_geo_fence.fence0.setup_fence_status) {
		case eFENCE_SETUP_UNKOWN: fprintf(log_fd, "\tgeo fence0 unkown setup\n"); break;
		case eFENCE_SETUP_ENTRY: fprintf(log_fd, "\tgeo fence0 entry setup\n"); break;
		case eFENCE_SETUP_EXIT: fprintf(log_fd, "\tgeo fence0 exit setup\n"); break;
		case eFENCE_SETUP_ENTRY_EXIT: fprintf(log_fd, "\tgeo fence0 entry & exit status\n"); break;
		default: fprintf(log_fd, "\tgeo fence0 unknown status #2\n"); break;
	}
	fprintf(log_fd, "\n\n");

	fprintf(log_fd, "[GEO FENCE #1]\n");
	switch(g_geo_fence.fence1.enable) {
		case eGEN_FENCE_DISABLE: fprintf(log_fd, "\tgeo fence1 disable\n"); break;
		case eGEN_FENCE_ENABLE: fprintf(log_fd, "\tgeo fence1 enable\n"); break;
		default: fprintf(log_fd, "\tgeo fence1 unknow enable status\n"); break;
	}
	fprintf(log_fd, "\tfence1 latitude = %06.6f\n", g_geo_fence.fence1.latitude);
	fprintf(log_fd, "\tfence1 longitude = %06.6f\n", g_geo_fence.fence1.longitude);
	fprintf(log_fd, "\tfence1 range = %d\n", g_geo_fence.fence1.range);
	
	switch(g_geo_fence.fence1.cur_fence_status) {
		case eFENCE_STATUS_UNKOWN: fprintf(log_fd, "\tgeo fence1 unkown status #1\n"); break;
		case eFENCE_STATUS_IN: fprintf(log_fd, "\tgeo fence1 IN status\n"); break;
		case eFENCE_STATUS_OUT: fprintf(log_fd, "\tgeo fence1 OUT status\n"); break;
		case eFENCE_STATUS_IGNORE: fprintf(log_fd, "\tgeo fence1 ignore status\n"); break;
		default: fprintf(log_fd, "\tgeo fence1 unknown status #2\n"); break;
	}

	switch(g_geo_fence.fence1.setup_fence_status) {
		case eFENCE_SETUP_UNKOWN: fprintf(log_fd, "\tgeo fence1 unkown setup\n"); break;
		case eFENCE_SETUP_ENTRY: fprintf(log_fd, "\tgeo fence1 entry setup\n"); break;
		case eFENCE_SETUP_EXIT: fprintf(log_fd, "\tgeo fence1 exit setup\n"); break;
		case eFENCE_SETUP_ENTRY_EXIT: fprintf(log_fd, "\tgeo fence1 entry & exit status\n"); break;
		default: fprintf(log_fd, "\tgeo fence1 unknown status #2\n"); break;
	}

#if defined(CORP_ABBR_NISSO)
		fprintf(log_fd, "[GEO FENCE #2]\n");
	switch(g_geo_fence.fence2.enable) {
		case eGEN_FENCE_DISABLE: fprintf(log_fd, "\tgeo fence2 disable\n"); break;
		case eGEN_FENCE_ENABLE: fprintf(log_fd, "\tgeo fence2 enable\n"); break;
		default: fprintf(log_fd, "\tgeo fence2 unknow enable status\n"); break;
	}
	fprintf(log_fd, "\tfence2 latitude = %06.6f\n", g_geo_fence.fence2.latitude);
	fprintf(log_fd, "\tfence2 longitude = %06.6f\n", g_geo_fence.fence2.longitude);
	fprintf(log_fd, "\tfence2 range = %d\n", g_geo_fence.fence2.range);
	
	switch(g_geo_fence.fence2.cur_fence_status) {
		case eFENCE_STATUS_UNKOWN: fprintf(log_fd, "\tgeo fence2 unkown status #1\n"); break;
		case eFENCE_STATUS_IN: fprintf(log_fd, "\tgeo fence2 IN status\n"); break;
		case eFENCE_STATUS_OUT: fprintf(log_fd, "\tgeo fence2 OUT status\n"); break;
		case eFENCE_STATUS_IGNORE: fprintf(log_fd, "\tgeo fence2 ignore status\n"); break;
		default: fprintf(log_fd, "\tgeo fence2 unknown status #2\n"); break;
	}

	switch(g_geo_fence.fence2.setup_fence_status) {
		case eFENCE_SETUP_UNKOWN: fprintf(log_fd, "\tgeo fence2 unkown setup\n"); break;
		case eFENCE_SETUP_ENTRY: fprintf(log_fd, "\tgeo fence2 entry setup\n"); break;
		case eFENCE_SETUP_EXIT: fprintf(log_fd, "\tgeo fence2 exit setup\n"); break;
		case eFENCE_SETUP_ENTRY_EXIT: fprintf(log_fd, "\tgeo fence2 entry & exit status\n"); break;
		default: fprintf(log_fd, "\tgeo fence2 unknown status #2\n"); break;
	}

		fprintf(log_fd, "[GEO FENCE #3]\n");
	switch(g_geo_fence.fence1.enable) {
		case eGEN_FENCE_DISABLE: fprintf(log_fd, "\tgeo fence3 disable\n"); break;
		case eGEN_FENCE_ENABLE: fprintf(log_fd, "\tgeo fence3 enable\n"); break;
		default: fprintf(log_fd, "\tgeo fence3 unknow enable status\n"); break;
	}
	fprintf(log_fd, "\tfence3 latitude = %06.6f\n", g_geo_fence.fence3.latitude);
	fprintf(log_fd, "\tfence3 longitude = %06.6f\n", g_geo_fence.fence3.longitude);
	fprintf(log_fd, "\tfence3 range = %d\n", g_geo_fence.fence3.range);
	
	switch(g_geo_fence.fence3.cur_fence_status) {
		case eFENCE_STATUS_UNKOWN: fprintf(log_fd, "\tgeo fence3 unkown status #1\n"); break;
		case eFENCE_STATUS_IN: fprintf(log_fd, "\tgeo fence3 IN status\n"); break;
		case eFENCE_STATUS_OUT: fprintf(log_fd, "\tgeo fence3 OUT status\n"); break;
		case eFENCE_STATUS_IGNORE: fprintf(log_fd, "\tgeo fence3 ignore status\n"); break;
		default: fprintf(log_fd, "\tgeo fence3 unknown status #2\n"); break;
	}

	switch(g_geo_fence.fence3.setup_fence_status) {
		case eFENCE_SETUP_UNKOWN: fprintf(log_fd, "\tgeo fence3 unkown setup\n"); break;
		case eFENCE_SETUP_ENTRY: fprintf(log_fd, "\tgeo fence3 entry setup\n"); break;
		case eFENCE_SETUP_EXIT: fprintf(log_fd, "\tgeo fence3 exit setup\n"); break;
		case eFENCE_SETUP_ENTRY_EXIT: fprintf(log_fd, "\tgeo fence3 entry & exit status\n"); break;
		default: fprintf(log_fd, "\tgeo fence3 unknown status #2\n"); break;
	}
#endif

	if(fp != NULL)
		fclose(fp);
}

void _print_geo_fence(fence_notification_t check_status, fence_notification_t cur_status)
{
	static int print_count = 0;

	if(print_count++ > 10) {
		LOGI(LOG_TARGET, "geo check_status fence status[%d]\n", check_status);
		LOGI(LOG_TARGET, "geo current fence status [%d]\n", cur_status);
		print_count = 0;
	}
}

int init_geo_fence()
{
	int ret;
	ret = storage_load_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
	LOGD(LOG_TARGET, "geo fence load_file = [%d]\n", ret);
	if(ret != ERR_NONE)
	{
		_default_geo_fence(&g_geo_fence);
		ret = storage_save_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
		LOGI(LOG_TARGET, "geo fence default file save = [%d]\n", ret);
	}

	_print_geo_fence_status();
	return 0;
}

int set_geo_fence0(geo_fence_data_t data)
{
	int ret;
	memcpy(&g_geo_fence.fence0, &data, sizeof(geo_fence_data_t));
	ret = storage_save_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
	_print_geo_fence_status();
	return ret;
}

int  set_geo_fence1(geo_fence_data_t data)
{
	int ret;
	memcpy(&g_geo_fence.fence1, &data, sizeof(geo_fence_data_t));
	ret = storage_save_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
	_print_geo_fence_status();
	return ret;
}

#if defined(CORP_ABBR_NISSO)
int  set_geo_fence2(geo_fence_data_t data)
{
	int ret;
	memcpy(&g_geo_fence.fence2, &data, sizeof(geo_fence_data_t));
	ret = storage_save_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
	_print_geo_fence_status();
	return ret;
}

int  set_geo_fence3(geo_fence_data_t data)
{
	int ret;
	memcpy(&g_geo_fence.fence3, &data, sizeof(geo_fence_data_t));
	ret = storage_save_file(GEO_FENCE_STATUS_FILE, &g_geo_fence, sizeof(geo_fence_hdr_t));
	_print_geo_fence_status();
	return ret;
}
#endif


fence_notification_t _check_fence_data(int dist, int entry_range, int exit_range)
{
	if(dist <= entry_range)
		return eFENCE_IN_NOTIFICATION;

	if (dist > exit_range)
		return eFENCE_OUT_NOTIFICATION;
		//fence out

	return eFENCE_NONE_NOTIFICATION; //ignore arae
}

fence_notification_t _get_notification(geo_fence_data_t *pfence, gpsData_t cur_gps)
{

	fence_notification_t result = eFENCE_NONE_NOTIFICATION;
	int dist_diff;
	dist_diff = get_distance_meter(pfence->latitude, cur_gps.lat, pfence->longitude, cur_gps.lon);
	if(dist_diff < 0)
	{
		return eFENCE_NONE_NOTIFICATION;
	}

	switch(pfence->setup_fence_status)
	{
		case eFENCE_SETUP_ENTRY_EXIT:
		case eFENCE_SETUP_ENTRY:
		case eFENCE_SETUP_EXIT:
			result = _check_fence_data(dist_diff, pfence->range, pfence->range);
		default:
			break;
	}

	_print_geo_fence(result, pfence->cur_fence_status);

	if(result != eFENCE_NONE_NOTIFICATION)
	{
		if(result != pfence->cur_fence_status) 
		{
			pfence->cur_fence_status = result;
			return result; //event happen
		}
	}

	return eFENCE_NONE_NOTIFICATION;
}



fence_notification_t get_geofence0_notification(gpsData_t cur_gps)
{
	fence_notification_t result = eFENCE_NONE_NOTIFICATION;

	if(geo_fence0_wait_count-- > 0) {
		return eFENCE_NONE_NOTIFICATION;
	}

	result = _get_notification(&g_geo_fence.fence0, cur_gps);
	if(result != eFENCE_NONE_NOTIFICATION)
	{
		switch(g_geo_fence.fence0.setup_fence_status)
		{
			case eFENCE_SETUP_UNKOWN:
				return eFENCE_NONE_NOTIFICATION;
			case eFENCE_SETUP_ENTRY:
				if(result != eFENCE_IN_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_EXIT:
				if(result != eFENCE_OUT_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_ENTRY_EXIT:
				break;
			default:
				return eFENCE_NONE_NOTIFICATION;
		}		

		
		geo_fence0_wait_count = WAIT_TIME_UNTIL_NEXT_GEO_EVENT;
		set_geo_fence0(g_geo_fence.fence0);
	}

	return result;
}

fence_notification_t get_geofence1_notification(gpsData_t cur_gps)
{
	fence_notification_t result = eFENCE_NONE_NOTIFICATION;
	
	if(geo_fence1_wait_count-- > 0) {
		return eFENCE_NONE_NOTIFICATION;
	}

	result = _get_notification(&g_geo_fence.fence1, cur_gps);
	if(result != eFENCE_NONE_NOTIFICATION)
	{
		switch(g_geo_fence.fence1.setup_fence_status)
		{
			case eFENCE_SETUP_UNKOWN:
				return eFENCE_NONE_NOTIFICATION;
			case eFENCE_SETUP_ENTRY:
				if(result != eFENCE_IN_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_EXIT:
				if(result != eFENCE_OUT_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_ENTRY_EXIT:
				break;
			default:
				return eFENCE_NONE_NOTIFICATION;
		}	

		geo_fence1_wait_count = WAIT_TIME_UNTIL_NEXT_GEO_EVENT;
		set_geo_fence1(g_geo_fence.fence1);
	}

	return result;
}

#if defined(CORP_ABBR_NISSO)
fence_notification_t get_geofence2_notification(gpsData_t cur_gps)
{
	fence_notification_t result = eFENCE_NONE_NOTIFICATION;
	
	if(geo_fence2_wait_count-- > 0) {
		return eFENCE_NONE_NOTIFICATION;
	}

	result = _get_notification(&g_geo_fence.fence2, cur_gps);
	if(result != eFENCE_NONE_NOTIFICATION)
	{
		switch(g_geo_fence.fence2.setup_fence_status)
		{
			case eFENCE_SETUP_UNKOWN:
				return eFENCE_NONE_NOTIFICATION;
			case eFENCE_SETUP_ENTRY:
				if(result != eFENCE_IN_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_EXIT:
				if(result != eFENCE_OUT_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_ENTRY_EXIT:
				break;
			default:
				return eFENCE_NONE_NOTIFICATION;
		}	

		geo_fence2_wait_count = WAIT_TIME_UNTIL_NEXT_GEO_EVENT;
		set_geo_fence2(g_geo_fence.fence2);
	}

	return result;
}

fence_notification_t get_geofence3_notification(gpsData_t cur_gps)
{
	fence_notification_t result = eFENCE_NONE_NOTIFICATION;
	
	if(geo_fence3_wait_count-- > 0) {
		return eFENCE_NONE_NOTIFICATION;
	}

	result = _get_notification(&g_geo_fence.fence3, cur_gps);
	if(result != eFENCE_NONE_NOTIFICATION)
	{
		switch(g_geo_fence.fence3.setup_fence_status)
		{
			case eFENCE_SETUP_UNKOWN:
				return eFENCE_NONE_NOTIFICATION;
			case eFENCE_SETUP_ENTRY:
				if(result != eFENCE_IN_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_EXIT:
				if(result != eFENCE_OUT_NOTIFICATION)
					return eFENCE_NONE_NOTIFICATION;
				break;
			case eFENCE_SETUP_ENTRY_EXIT:
				break;
			default:
				return eFENCE_NONE_NOTIFICATION;
		}	

		geo_fence3_wait_count = WAIT_TIME_UNTIL_NEXT_GEO_EVENT;
		set_geo_fence3(g_geo_fence.fence3);
	}

	return result;
}

#endif

geo_fence_eable_t get_geo_fence0_enable()
{
	return g_geo_fence.fence0.enable;
}

geo_fence_eable_t get_geo_fence1_enable()
{
	return g_geo_fence.fence1.enable;
}

#if defined(CORP_ABBR_NISSO)
geo_fence_eable_t get_geo_fence2_enable()
{
	return g_geo_fence.fence2.enable;
}

geo_fence_eable_t get_geo_fence3_enable()
{
	return g_geo_fence.fence3.enable;
}
#endif
/*
float get_geo_fence0_latitude()
{
	return g_geo_fence.fence0.latitude;
}

float get_geo_fence0_longitude()
{
	return g_geo_fence.fence0.longitude;
}

float get_geo_fence1_latitude()
{
	return g_geo_fence.fence1.latitude;
}

float get_geo_fence1_longitude()
{
	return g_geo_fence.fence1.longitude;
}

fence_status_t get_geo_fence0_status()
{
	return g_geo_fence.fence0.cur_fence_status;
}
fence_status_t get_geo_fence1_status()
{
	return g_geo_fence.fence1.cur_fence_status;
}

fence_setup_info_t get_geo_fence0_setup_info()
{
	return  g_geo_fence.fence0.setup_fence_status;
}
fence_setup_info_t get_geo_fence1_setup_info()
{
	return  g_geo_fence.fence1.setup_fence_status;
}
*/


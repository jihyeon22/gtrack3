#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <util/storage.h>
#include <base/mileage.h>
#include "geofence.h"
#include "logd/logd_rpc.h"
#include <base/gpstool.h>

geo_fence_debug_mode_t g_debug_mode = eGEN_FENCE_NORMAL_MODE;

geo_fence_setup_t g_setup_data[GEO_FENCE_MAX_COUNT];
geo_fence_status_t g_status_data[GEO_FENCE_MAX_COUNT];

int g_geo_fence_event_hold_count[GEO_FENCE_MAX_COUNT] = {0, };

static int recent_geo_fence = -1;
static int first_geo_fence = -1;

void _init_geo_fence_data()
{
	int i;
	for(i = 0; i < GEO_FENCE_MAX_COUNT; i++)
	{
		g_setup_data[i].enable = eGEN_FENCE_DISABLE;
		g_setup_data[i].latitude = 0.0;
		g_setup_data[i].longitude = 0.0;
		g_setup_data[i].range = 0;
		g_setup_data[i].setup_fence_status = eFENCE_SETUP_UNKNOWN;
		
		g_status_data[i].cur_fence_status = eFENCE_OUT_NOTIFICATION;

		g_geo_fence_event_hold_count[i] = 0;
	}
}

void debug_geo_fence_status()
{

	FILE *fp = NULL;
	FILE *log_fd = NULL;

	if(g_debug_mode != eGEN_FENCE_DEBUG_MODE)
		return;

	remove("/var/log/geofence.log");
	fp = fopen("/var/log/geofence.log", "w");
	
	if(fp == NULL)
		log_fd = stderr;
	else
		log_fd = fp;
	
	int i;
	for(i = 0; i < GEO_FENCE_MAX_COUNT; i++)
	{
		if(g_setup_data[i].enable == eGEN_FENCE_ENABLE) 
		{
			fprintf(log_fd, "[GEO FENCE #%02d]\n", i);
			fprintf(log_fd, "\tenable\n");
			fprintf(log_fd, "\tlatitude = %06.6f\n", g_setup_data[i].latitude);
			fprintf(log_fd, "\tlongitude = %06.6f\n", g_setup_data[i].longitude);
			fprintf(log_fd, "\trange = %d\n", g_setup_data[i].range);
			switch(g_setup_data[i].setup_fence_status) {
				case eFENCE_SETUP_UNKNOWN: fprintf(log_fd, "\tunkown setup\n"); break;
				case eFENCE_SETUP_ENTRY: fprintf(log_fd, "\tentry setup\n"); break;
				case eFENCE_SETUP_EXIT: fprintf(log_fd, "\texit setup\n"); break;
				case eFENCE_SETUP_ENTRY_EXIT: fprintf(log_fd, "\tentry & exit status\n"); break;
				default: fprintf(log_fd, "\tunknown setup info error\n"); break;
			}

			switch(g_status_data[i].cur_fence_status) {
				case eFENCE_IN_NOTIFICATION: fprintf(log_fd, "\tIN status\n"); break;
				case eFENCE_OUT_NOTIFICATION: fprintf(log_fd, "\tOUT status\n"); break;
				case eFENCE_NONE_NOTIFICATION: fprintf(log_fd, "\tunkown status\n"); break;
				default: fprintf(log_fd, "\tunknown current status error\n"); break;
			}

			fprintf(log_fd, "\n\n");
		}
	}

	if(fp != NULL)
		fclose(fp);
}

int  save_geo_fence_status_info()
{
	int ret;
	ret = storage_save_file(GEO_FENCE_STATUS_FILE, g_status_data, sizeof(geo_fence_status_t)*GEO_FENCE_MAX_COUNT);
	debug_geo_fence_status();
	return ret;
}

fence_notification_t _check_fence_data(int dist, int entry_range, int exit_range)
{
	if(dist <= entry_range)
		return eFENCE_IN_NOTIFICATION;

	if (dist > exit_range)
		return eFENCE_OUT_NOTIFICATION;
		//fence out

	return eFENCE_NONE_NOTIFICATION; //ignore arae
}

char * _print_geo_fence_notification(fence_notification_t check_status)
{
	if(check_status == eFENCE_IN_NOTIFICATION)
		return "eFENCE_IN_NOTIFICATION";
	else if(check_status == eFENCE_OUT_NOTIFICATION)
		return "eFENCE_OUT_NOTIFICATION";
	
	return "eFENCE_NONE_NOTIFICATION";
}
fence_notification_t _get_notification(int idx, gpsData_t *cur_gps)
{

	fence_notification_t result = eFENCE_NONE_NOTIFICATION;
	int dist_diff;
	dist_diff = get_distance_m(g_setup_data[idx].latitude, cur_gps->lat, g_setup_data[idx].longitude, cur_gps->lon);
	if(dist_diff < 0)
	{
		return eFENCE_NONE_NOTIFICATION;
	}

	result = _check_fence_data(dist_diff, g_setup_data[idx].range, g_setup_data[idx].range);

/*
	if(g_debug_mode == eGEN_FENCE_DEBUG_MODE) {
		printf("_check_fence_data : %s\n", _print_geo_fence_notification(result)); ;
	}
*/

	switch(g_setup_data[idx].setup_fence_status)
	{
		case eFENCE_SETUP_ENTRY_EXIT:
		case eFENCE_SETUP_ENTRY:
		case eFENCE_SETUP_EXIT:
			break;
		default:
			result = eFENCE_NONE_NOTIFICATION;
			break;
	}

/*
	if(g_debug_mode == eGEN_FENCE_DEBUG_MODE) {
		printf("setup_fence_status : %s\n", _print_geo_fence_notification(result));
	}
*/
	if(result != eFENCE_NONE_NOTIFICATION)
	{
		if(result != g_status_data[idx].cur_fence_status)
		{
			g_status_data[idx].cur_fence_status = result;
			//_save_geo_fence_status_info();

			if(g_setup_data[idx].setup_fence_status == eFENCE_SETUP_ENTRY)
			{
				if(result != eFENCE_IN_NOTIFICATION)
					result = eFENCE_NONE_NOTIFICATION;
			}
			else if(g_setup_data[idx].setup_fence_status == eFENCE_SETUP_EXIT)
			{
				if(result != eFENCE_OUT_NOTIFICATION)
					result = eFENCE_NONE_NOTIFICATION;
			}
			return result;
		}
	}

	return eFENCE_NONE_NOTIFICATION;
}



fence_notification_t get_geofence_notification(int *pfence_num, gpsData_t cur_gps)
{
	int i;
	fence_notification_t result = eFENCE_NONE_NOTIFICATION;

	for(i = 0; i < GEO_FENCE_MAX_COUNT; i++)
	{
		if(g_setup_data[i].enable == eGEN_FENCE_DISABLE)
			continue;
		
		if(g_geo_fence_event_hold_count[i]-- > 0) {
			if(g_debug_mode == eGEN_FENCE_DEBUG_MODE) {
				printf("[%02d fence] check skip for holding time!!!\n", i);
			}
			continue;
		}


		result = _get_notification(i, &cur_gps);
/*
		if(g_debug_mode == eGEN_FENCE_DEBUG_MODE) {
			printf("_get_notification : %s\n", _print_geo_fence_notification(result));
		}
//*/
		if(result == eFENCE_NONE_NOTIFICATION)
			continue;

		if(result == eFENCE_IN_NOTIFICATION)
			set_recent_geo_fence(i);

		g_geo_fence_event_hold_count[i] = WAIT_TIME_UNTIL_NEXT_GEO_EVENT;
		*pfence_num = i;

		if(g_debug_mode == eGEN_FENCE_DEBUG_MODE) {
			printf("================================================================\n");
			printf("%02d fence happen Event %s\n", i, _print_geo_fence_notification(result));
			printf("================================================================\n");
		}

		return result; //ocurre geo fence event
	}

	return result;
}


int init_geo_fence(geo_fence_debug_mode_t debug_mode)
{
	int ret;

	g_debug_mode = debug_mode;

	_init_geo_fence_data();

//jwrho ++
// jhcho compile :  why comment??
	ret = storage_load_file(GEO_FENCE_SETUP_DATA_FILE, g_setup_data, sizeof(geo_fence_setup_t)*GEO_FENCE_MAX_COUNT);
	printf("geo fence %s load_file = [%d]\n", GEO_FENCE_SETUP_DATA_FILE, ret);
	if(ret != ERR_NONE)
	{
		ret = storage_save_file(GEO_FENCE_SETUP_DATA_FILE, g_setup_data, sizeof(geo_fence_setup_t)*GEO_FENCE_MAX_COUNT);
		printf("geo fence default file save = [%d]\n", ret);
	}

	ret = storage_load_file(GEO_FENCE_STATUS_FILE, g_status_data, sizeof(geo_fence_status_t)*GEO_FENCE_MAX_COUNT);
	printf("geo fence %s load_file = [%d]\n", GEO_FENCE_STATUS_FILE, ret);
	if(ret != ERR_NONE)
	{
		ret = storage_save_file(GEO_FENCE_STATUS_FILE, g_status_data, sizeof(geo_fence_status_t)*GEO_FENCE_MAX_COUNT);
		printf("geo fence default file save = [%d]\n", ret);
	}


	//_print_geo_fence_status();

//jwrho--
	return 0;
}

int set_geo_fence_setup_info(int idx, geo_fence_setup_t *data)
{
	if(idx >= GEO_FENCE_MAX_COUNT || idx < 0)
		return -1;

	memcpy(&g_setup_data[idx], data, sizeof(geo_fence_setup_t));

	return 0;
}

int get_geo_fence_setup_info(int idx, geo_fence_setup_t *data)
{
	if(idx >= GEO_FENCE_MAX_COUNT || idx < 0)
		return -1;

	memcpy(data, &g_setup_data[idx], sizeof(geo_fence_setup_t));

	return 0;
}

int get_recent_geo_fence(void)
{
	return recent_geo_fence;
}

void set_recent_geo_fence(int in)
{
	if(in < 0)
	{
		in = -1;
	}

	recent_geo_fence = in;
	
	if(first_geo_fence == -1)
	{
		first_geo_fence = in;
	}	
}

int get_first_geo_fence(void)
{
	return first_geo_fence;
}


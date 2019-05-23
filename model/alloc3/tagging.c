#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <include/defines.h>
#include <base/gpstool.h>
#include "logd/logd_rpc.h"

#include "data-list.h"
#include "tagging.h"
#include "debug.h"

#include "alloc_packet_tool.h"

#include "color_printf.h"

static int tagging_geo_fence = -1;
static int tagging_count = 0;
static char tagging_array[RFID_LIST_SIZE+1];
static int tagging_array_index = 0;
static int tagging_num= 0;

static pthread_mutex_t tagging_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int g_tl500_geofence_reset; 

void _clear_tagging(void)
{
	memset(tagging_array, 0, sizeof(tagging_array));
	tagging_array_index = 0;
	tagging_num = 0;
	tagging_count = 0;
}

void *_get_tagging_data_malloc(void)
{
	char *pdata = NULL;

	if(tagging_num <= 0 && tagging_count <= 0)
	{
		return NULL;
	}

	pdata = malloc(sizeof(taggingData_t) + tagging_array_index);
	if(!pdata)
	{
		//noti error
		return NULL;
	}
	
	memset(pdata, 0, sizeof(taggingData_t) + tagging_array_index);

	gpsData_t gps;
	gps_get_curr_data(&gps);

	taggingData_t *temp_td = (taggingData_t *)pdata;
	snprintf(temp_td->date, sizeof(temp_td->date),"%04d%02d%02d%02d%02d%02d",
		gps.year, gps.mon, gps.day, gps.hour, gps.min, gps.sec);
	//	gps.year%100, gps.mon, gps.day, gps.hour, gps.min, gps.sec);



	memcpy(temp_td->tagging_data, tagging_array, tagging_array_index);
	temp_td->count = tagging_count;
	temp_td->idx_geo_fence = tagging_geo_fence;
	temp_td->tagging_num = tagging_num;
	temp_td->tagging_data_len = tagging_array_index;
	
	return pdata;
}

int tagging_add_rfid(char *rfid_pat, int geo_fence, char *rfid_date)
{
	char comma[2] = {0};
	int rfid_len = strnlen(rfid_pat, RFID_DATA_LEN);
	void *item = NULL;
	int need_push_list = 0;
	
	LOGT(LOG_TARGET, "%s : %s, %d", __FUNCTION__, rfid_pat, geo_fence);

	pthread_mutex_lock(&tagging_mutex);

	if(tagging_array_index + strlen(comma) + rfid_len  > RFID_LIST_SIZE)
	{
		need_push_list = 1;
	}

	if(geo_fence != tagging_geo_fence && tagging_geo_fence >= 0)
	{
		need_push_list = 1;
	}

	if(need_push_list)
	{
		item = _get_tagging_data_malloc();
		_clear_tagging();
		if(item)
		{
			if(list_add(&tagging_data_list, item) < 0)
			{
				printf("fail\n");
				LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
				free(item);
			}
		}
	}

	if(tagging_array_index)
		comma[0] = ',';
#if SEND_DUPLICATION_RFID
#else
	if(strstr(tagging_array, rfid_pat) != NULL)
	{
		LOGT(LOG_TARGET, "%s : exist rfid tag.\n", __FUNCTION__);
		pthread_mutex_unlock(&tagging_mutex);
		return 0;
	}
#endif
	
	tagging_geo_fence = geo_fence;

	char temp_fence_id[12] = {0};

	if(geo_fence == -1)
	{
		print_yellow("geo_fence  -1 \n"); 
		temp_fence_id[0] = '0';
	}
	else if (g_tl500_geofence_reset == 1)
	{
		print_yellow("geo_fence_in : %d\n", geo_fence); 
		temp_fence_id[0] = '0';
	}
	else
	{
		print_yellow("geo_fence : %d\n", geo_fence); 
		get_geo_fence_id(geo_fence, temp_fence_id);
	}


	tagging_array_index += sprintf(tagging_array+tagging_array_index, "%s%s,%.14s,%.15s", comma, temp_fence_id, rfid_date, rfid_pat);
	
	print_yellow("tagging_add_rfid : %s%s,%.14s,%.15s\n", comma, temp_fence_id, rfid_date, rfid_pat); 
	tagging_num++;

	pthread_mutex_unlock(&tagging_mutex);
	return 0;
}

int tagging_add_count(int geo_fence)
{
	void *item = NULL;

	LOGT(LOG_TARGET, "%s : %d", __FUNCTION__, geo_fence);

	pthread_mutex_lock(&tagging_mutex);

	if(geo_fence != tagging_geo_fence && tagging_geo_fence >= 0)
	{
		item = _get_tagging_data_malloc();
		_clear_tagging();
		if(item)
		{
			LOGT(LOG_TARGET, "%s : list add\n", __FUNCTION__);
			
			if(list_add(&tagging_data_list, item) < 0)
			{
				printf("fail\n");
				LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
				free(item);
			}
		}

	}

	tagging_geo_fence = geo_fence;
	tagging_count++;
	
	pthread_mutex_unlock(&tagging_mutex);
	return 0;
}

void tagging_add_list(void)
{
	void *item = NULL;
	
	pthread_mutex_lock(&tagging_mutex);
	
	item = _get_tagging_data_malloc();
	_clear_tagging();
	if(item)
	{
		printf("%s: add list\n", __FUNCTION__);
	
		if(list_add(&tagging_data_list, item) < 0)
		{
			LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
			free(item);
		}
	}
	
	pthread_mutex_unlock(&tagging_mutex);
}

void tagging_print_data(taggingData_t *d)
{
	LOGE(LOG_TARGET, "%s\n", __FUNCTION__);
	
	if(!d)
		return;
	
	printf("\n\n");
	printf("GeoFence:%d\nCount:%d\nNumOfTag:%d\nLenOfTagData:%d\n",
		d->idx_geo_fence, d->count, d->tagging_num, d->tagging_data_len);
	printf("Data:%s\n",
		d->tagging_data);
	printf("\n\n");
}


#include <stdlib.h>

#include <base/config.h>
#include <base/mileage.h>
#include <base/gpstool.h>
#include "config.h"
#include "section.h"

#define SECTION_NUM	11

static int section_cond[SECTION_NUM];
static int acc_dist = 0;

int section_check(gpsData_t *gpsdata)
{
	static int flag_first = 0;
	static gpsData_t prev_gpsdata;
	
	int dist = 0;
	int index = 0;
	
	if(flag_first == 0)
	{
		flag_first = 1;
		
		prev_gpsdata.lat = gpsdata->lat;
		prev_gpsdata.lon = gpsdata->lon;

		return 0;
	}

	dist = get_distance_m(prev_gpsdata.lat, gpsdata->lat, prev_gpsdata.lon, gpsdata->lon);
	acc_dist += dist;

	//printf("speed:%d %f,%f %f,%f (%d/%d)\n",gpsdata->speed, prev_gpsdata.lat,prev_gpsdata.lon,gpsdata->lat,gpsdata->lon, dist, acc_dist);

	prev_gpsdata.lat = gpsdata->lat;
	prev_gpsdata.lon = gpsdata->lon;

	index = gpsdata->speed / 10;
	if(gpsdata->speed % 10 == 0)
	{
		index--;
	}
	if(index < 0)
	{
		index = 0;
	}
	else if(index >= SECTION_NUM)
	{
		index = SECTION_NUM-1;
	}

	if(section_cond[index] == 0)
	{
		return 0;
	}

	if(acc_dist >= section_cond[index])
	{
		return 1;
	}
	
	return 0;
}

void section_clear_acc_dist(void)
{
	acc_dist = 0;
}

int section_setup_from_str(int index, char *str_cond)
{
	section_cond[index] = atoi(str_cond);
	
	return 0;
}

void section_setup_from_config(void)
{
	configurationModel_t *conf = get_config_model();

	section_cond[0] = conf->model.section_10kms;
	section_cond[1] = conf->model.section_20kms;
	section_cond[2] = conf->model.section_30kms;
	section_cond[3] = conf->model.section_40kms;
	section_cond[4] = conf->model.section_50kms;
	section_cond[5] = conf->model.section_60kms;
	section_cond[6] = conf->model.section_70kms;
	section_cond[7] = conf->model.section_80kms;
	section_cond[8] = conf->model.section_90kms;
	section_cond[9] = conf->model.section_100kms;
	section_cond[10] = conf->model.section_110kms;

	int i = 0;
	for(i=0; i<=10; i++)
	{
		printf("section %d:%d\n", i, section_cond[i]);
	}
}

void section_setup_to_config(void)
{
	configurationModel_t *conf = get_config_model();
	iniData_t arr_data[SECTION_NUM+1] =
	{
		{.name="user:section_10kms"},
		{.name="user:section_20kms"},
		{.name="user:section_30kms"},
		{.name="user:section_40kms"},
		{.name="user:section_50kms"},
		{.name="user:section_60kms"},
		{.name="user:section_70kms"},
		{.name="user:section_80kms"},
		{.name="user:section_90kms"},
		{.name="user:section_100kms"},
		{.name="user:section_110kms"},
		{.name=NULL, .msg=NULL},
	};

	char buf_value[SECTION_NUM][5] = {{0}};
	
	int i = 0;

	conf->model.section_10kms = section_cond[0];
	conf->model.section_20kms = section_cond[1];
	conf->model.section_30kms = section_cond[2];
	conf->model.section_40kms = section_cond[3];
	conf->model.section_50kms = section_cond[4];
	conf->model.section_60kms = section_cond[5];
	conf->model.section_70kms = section_cond[6];
	conf->model.section_80kms = section_cond[7];
	conf->model.section_90kms = section_cond[8];
	conf->model.section_100kms = section_cond[9];
	conf->model.section_110kms = section_cond[10];

	for(i=0; i<SECTION_NUM; i++)
	{
		snprintf(buf_value[i], 4, "%d", section_cond[i]);
		arr_data[i].msg = buf_value[i];
	}

	save_config_array_user(arr_data);
}

int section_setup_get(int index)
{
	return section_cond[index];
}

void section_setup_set(int index, unsigned int value)
{
	section_cond[index] = value;
}


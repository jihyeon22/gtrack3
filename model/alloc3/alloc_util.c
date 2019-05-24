#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include "config.h"
#include "alloc_packet.h"
#include <base/gpstool.h>
#include <at/at_util.h>
#include <board/power.h>
#include <base/config.h>
#include <util/storage.h>


#include "alloc_util.h"

#include "color_printf.h"

static int g_circulating_bus = 0;

int roundNo(float num)
{
	return num < 0 ? num - 0.5555 : num + 0.5555;
}

int RoundOff(float x, int dig)
{
	return floor((x) * pow(10,dig) + 0.5) / pow(10,dig);
}
#if 0
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
#endif

int get_report_interval()
{
	int interval_time;
	int ign_on = power_get_ignition_status();
	configurationModel_t *conf = get_config_model();

	if(ign_on == POWER_IGNITION_OFF)
		interval_time = conf->model.report_interval_keyoff;
	else
		interval_time = conf->model.report_interval_keyon;	
	
	return interval_time;
}

int get_rssi_gps(void)
{
	gpsData_t temp_gps;
	char temp_ant[10] = {0};
	int ant = 0;

	gps_get_curr_data(&temp_gps);

	if(temp_gps.active == 1)
	{
		return eGPS_FIX;
	}

	at_get_gps_ant(temp_ant, sizeof(temp_ant));

	ant = atoi(temp_ant);
	if(ant >= 10)
	{
		return eGPS_NOFIX;
	}

	return eGPS_NOSIGNAL;
}
void getfilenameformat24(char *name, char *path)
{
	char name1[32] = {0,};

	memset(name1, '0', 24);
	memset(name, '0', 24);	
	strncpy(name1, strrchr(path,'/')+1, 8);
	printf("name1 : %s\n", name1);
	strncpy(name, name1, strlen(name1));
}

int get_circulating_bus(void)
{
	return g_circulating_bus;
}
void set_circulating_bus(int circulating)
{
	g_circulating_bus = circulating;
	print_yellow("g_circulating_bus1 : %d\n", g_circulating_bus);
	save_circulating_bus_info();
}
int load_circulating_bus_info()
{
	int ret;
	ret = storage_load_file(CIRCULATING_BUS_STATUS_FILE, &g_circulating_bus, sizeof(int));

	print_yellow("g_circulating_bus3 : %d, %d\n", g_circulating_bus, ret);
	if(ret != ERR_NONE)
	{
		ret = save_circulating_bus_info();
	}
	
	return ret;
}
int save_circulating_bus_info()
{
	int ret;
	print_yellow("save_circulating_bus_info \n");
	ret = storage_save_file(CIRCULATING_BUS_STATUS_FILE, &g_circulating_bus, sizeof(int));
	print_yellow("g_circulating_bus3 : %d\n", g_circulating_bus);
	system("sync &");
	return ret;
}


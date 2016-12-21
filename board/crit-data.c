#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/utsname.h>

#include <util/crc16.h>
#include "crit-data.h"

static int _check_crit_data(void);
static int _check_core_version(void);

#define USE_CRIT		1


static int fd_crit = -1;
static int bool_support_crit_data = 0;

static critStruct_t critical_data;

int crit_init(void)
{
	return 0;
}

void crit_deinit(void)
{
	return;
}

int crit_write_data(void)
{	
	return 0;
}

int crit_read_data(void)
{
	return 0;
}

int crit_get_data_mileage(unsigned int *mileage)
{
	*mileage = 0;
	
	return 0;
}

int crit_get_data_mileage_detail(unsigned int *mileage, int *lastyear, int *lastmon, int *lastday, float *lastlat, float *lastlon)
{
	*mileage = 0;
	*lastyear = 0;
	*lastmon = 0;
	*lastday = 0;
	*lastlat = 0;
	*lastlon = 0;
	
	return 0;
}

int crit_set_data_mileage(unsigned int mileage)
{	
	return 0;
}

int crit_set_data_mileage_detail(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon)
{
	/*
	critical_data.bd_data.m.mileage = mileage;
	critical_data.bd_data.m.lastyear = lastyear;
	critical_data.bd_data.m.lastmon = lastmon;
	critical_data.bd_data.m.lastday = lastday;
	critical_data.bd_data.m.lastlat = lastlat;
	critical_data.bd_data.m.lastlon = lastlon;
	*/
	return 0;
}

int crit_set_data_mileage_write(unsigned int mileage)
{

	return 0;
}

int crit_set_data_mileage_detail_write(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon)
{
	
	return 0;
}

int crit_get_data_gps(float *lat, float *lon, unsigned int *utc_sec)
{

	return 0;
}

int crit_set_data_gps(float lat, float lon, unsigned int utc_sec)
{

	return 0;
}

int crit_set_data_gps_write(float lat, float lon, unsigned int utc_sec)
{

	return 0;
}

int crit_clear_write(void)
{

	return 0;
}

void crit_dump_data(void)
{

}

int crit_get_support_stat(void)
{
	return bool_support_crit_data;
}

static int _check_crit_data(void)
{
	return 0;
}

static int _check_core_version(void)
{
	return 0;
}


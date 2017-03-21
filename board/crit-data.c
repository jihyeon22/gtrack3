#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/utsname.h>

#include <util/crc16.h>
#include "crit-data.h"

#include <mdsapi/mds_api.h>

static int _check_crit_data(void);
//static int _check_core_version(void);

#define USE_CRIT		1

#define CRITICAL_DATA_TMP_FS	"/tmp/critical.dat"
#define CRITICAL_DATA_NAND_FS	"/data/mds/data/critical.dat"


static critStruct_t critical_data;

void _crit_init_simul()
{
	static critStruct_t critical_data_tmp;
	// critical_data.hd_raw <= DEFAULT_CRITICAL_DATA
	if ( mds_api_check_exist_file(CRITICAL_DATA_TMP_FS,0) == DEFINES_MDS_API_OK )
	{
		mds_api_write_data(CRITICAL_DATA_TMP_FS, &critical_data_tmp, sizeof(critStruct_t), 0);
		return;
	}
	
	if ( mds_api_check_exist_file(CRITICAL_DATA_NAND_FS,1) == DEFINES_MDS_API_OK )
	{
		mds_api_cp(CRITICAL_DATA_NAND_FS, CRITICAL_DATA_TMP_FS, 1);
		return;
	}
}
	
void crit_backup_simul(void)
{
	mds_api_cp(CRITICAL_DATA_TMP_FS, CRITICAL_DATA_NAND_FS, 1);
}

int crit_init(void)
{
	int n_try = 3;
	unsigned short crc16 = 0;
	
	printf("++Init Critical-data++\n");
	printf("Size of critical-data structure : %d\n", sizeof(critical_data));
	printf("Size of critical-data head : %d\n", sizeof(critical_data.hd_data));
	printf("Size of critical-data body : %d\n", sizeof(critical_data.bd_data));
	printf("--Init Critical-data--\n");

	_crit_init_simul();

	if(mds_api_read_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t)) < 0)
	{
		printf("ERROR : Doesn't support critical data function.\n");	
		return -1;
	}

	crit_dump_data();

	if(!strcmp(critical_data.hd_raw, DEFAULT_CRITICAL_DATA))
	{
		printf("WARN : Critical_data is empty.\n");
		memset(&critical_data, 0, sizeof(critical_data));
		return 0;
	}
	
	//Check crc16 and if it fail to check crc16, clear critical_data and return 0.
	crc16_get(NULL, 0);
	crc16 = crc16_get((unsigned char *)&(critical_data.bd_data),critical_data.hd_data.length);	
	if(crc16 != critical_data.hd_data.crc16)
	{
		printf("ERROR : Crc16 is a missmach.");
		memset(&critical_data, 0, sizeof(critical_data));
		return 0;
	}

	return 0;

}

void crit_deinit(void)
{
	// nothing..
}

int crit_write_data(void)
{
	if(_check_crit_data() < 0)
	{
		printf("ERROR : Critical-data is abnormal.");
		return -1;
	}
	
	crc16_get(NULL, 0);
	critical_data.hd_data.crc16 = crc16_get((unsigned char *)&(critical_data.bd_data),sizeof(critical_data.bd_data));
	critical_data.hd_data.length = sizeof(critical_data.bd_data);
	critical_data.hd_data.ver = CRIT_VERSION;

	if(	mds_api_write_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t), 0) < 0)
	{
		printf("ERROR : Fail to write critical-data.");
		return -1;
	}
	
	return 0;
}

int crit_read_data(void)
{

	int fd_crit = -1;
	
	if(fd_crit < 0)
	{
		return -1;
	}
	
	if(mds_api_read_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t)) < 0)
	{
		printf("ERROR : Fail to read critical-data.");
		return -1;
	}
	
	return 0;
}

int crit_get_data_mileage(unsigned int *mileage)
{
	
	*mileage = critical_data.bd_data.m.mileage;
	
	return 0;
}

int crit_get_data_mileage_detail(unsigned int *mileage, int *lastyear, int *lastmon, int *lastday, float *lastlat, float *lastlon)
{

	
	*mileage = critical_data.bd_data.m.mileage;
	*lastyear = critical_data.bd_data.m.lastyear;
	*lastmon = critical_data.bd_data.m.lastmon;
	*lastday = critical_data.bd_data.m.lastday;
	*lastlat = critical_data.bd_data.m.lastlat;
	*lastlon = critical_data.bd_data.m.lastlon;
	
	return 0;
}

int crit_set_data_mileage(unsigned int mileage)
{

	
	critical_data.bd_data.m.mileage = mileage;
	
	if(	mds_api_write_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t), 0) < 0)
	{
		printf("ERROR : Fail to write critical-data.");
		return -1;
	}

	return 0;
}

int crit_set_data_mileage_detail(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon)
{

	
	critical_data.bd_data.m.mileage = mileage;
	critical_data.bd_data.m.lastyear = lastyear;
	critical_data.bd_data.m.lastmon = lastmon;
	critical_data.bd_data.m.lastday = lastday;
	critical_data.bd_data.m.lastlat = lastlat;
	critical_data.bd_data.m.lastlon = lastlon;
	
	if(	mds_api_write_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t), 0) < 0)
	{
		printf("ERROR : Fail to write critical-data.");
		return -1;
	}

	return 0;
}

int crit_set_data_mileage_write(unsigned int mileage)
{


	if(crit_set_data_mileage(mileage) < 0)
	{
		return -1;
	}

	if(crit_write_data() < 0)
	{
		return -1;
	}

	return 0;
}

int crit_set_data_mileage_detail_write(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon)
{

	
	if(crit_set_data_mileage_detail(mileage, lastyear, lastmon, lastday, lastlat, lastlon) < 0)
	{
		return -1;
	}

	if(crit_write_data() < 0)
	{
		return -1;
	}
	
	return 0;
}

int crit_get_data_gps(float *lat, float *lon, unsigned int *utc_sec)
{

	
	*lat = critical_data.bd_data.g.lat;
	*lon = critical_data.bd_data.g.lon;
	*utc_sec = critical_data.bd_data.g.utc_sec;
	
	return 0;
}

int crit_set_data_gps(float lat, float lon, unsigned int utc_sec)
{

	
	critical_data.bd_data.g.lat = lat;
	critical_data.bd_data.g.lon = lon;
	critical_data.bd_data.g.utc_sec = utc_sec;
	
	if(	mds_api_write_data(CRITICAL_DATA_TMP_FS, &critical_data, sizeof(critStruct_t), 0) < 0)
	{
		printf("ERROR : Fail to write critical-data.");
		return -1;
	}

	return 0;
}

int crit_set_data_gps_write(float lat, float lon, unsigned int utc_sec)
{

	if(crit_set_data_gps(lat, lon, utc_sec) < 0)
	{
		return -1;
	}

	if(crit_write_data() < 0)
	{
		return -1;
	}

	return 0;
}

int crit_clear_write(void)
{


	memset(&critical_data, 0, sizeof(critical_data));

	if(crit_write_data() < 0)
	{
		return -1;
	}

	return 0;
}

void crit_dump_data(void)
{
	printf("++dump critical data++\n");
	printf("<HEAD>\nCRC16:%d\nLength:%d\n", critical_data.hd_data.crc16, critical_data.hd_data.length);
	printf("<BODY>\nMileage:%u\nLatitude:%f\nLongitude:%f\nUTC Sec:%u\n",
		critical_data.bd_data.m.mileage, critical_data.bd_data.g.lat, critical_data.bd_data.g.lon,
		critical_data.bd_data.g.utc_sec);
	printf("--dump critical data--\n");
}

int crit_get_support_stat(void)
{ 
	return 1;
}

static int _check_crit_data(void)
{
	return 0;
}



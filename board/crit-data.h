#ifndef __BOARD_CRIT_DATA_H__
#define __BOARD_CRIT_DATA_H__

#include "board_system.h"

#define MAX_CRITICAL_DATA         512
/*
#define IOCTL_GET_CRITICAL_DATA                                 _IOR(MDS_DEVICE_UTIL_MAJOR_NUMBER, 8, char *)
#define IOCTL_SET_CRITICAL_DATA                                 _IOR(MDS_DEVICE_UTIL_MAJOR_NUMBER, 9, char *)
*/
#define DEFAULT_CRITICAL_DATA	"AABBCCDD"

#define CRIT_VERSION 1

typedef struct
{
	unsigned char ver;
	unsigned short crc16;
	unsigned short length;
}__attribute__((packed))critHeader_t;

typedef struct
{
	unsigned int mileage;
	int lastyear;
	int lastmon;
	int lastday;
	float lastlat;
	float lastlon;
}__attribute__((packed))critMileage_t;

typedef struct
{
	float lat;
	float lon;
	unsigned int utc_sec;
}__attribute__((packed))critGPS_t;

typedef struct
{
	critMileage_t m;
	critGPS_t g;
}__attribute__((packed))critData_t;

typedef struct
{
	union
	{
		char hd_raw[sizeof(critHeader_t)];
		critHeader_t hd_data;
	};
	union
	{
		char bd_raw[MAX_CRITICAL_DATA-sizeof(critHeader_t)];
		critData_t bd_data;
	};
}__attribute__((packed))critStruct_t;

int crit_init(void);
void crit_deinit(void);
int crit_write_data(void);
int crit_read_data(void);
void crit_dump_data(void);
int crit_get_support_stat(void);
int crit_get_data_mileage(unsigned int *mileage);
int crit_get_data_mileage_detail(unsigned int *mileage, int *lastyear, int *lastmon, int *lastday, float *lastlat, float *lastlon);
int crit_set_data_mileage(unsigned int mileage);
int crit_set_data_mileage_detail(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon);
int crit_get_data_gps(float *lat, float *lon, unsigned int *utc_sec);
int crit_set_data_gps(float lat, float lon, unsigned int utc_sec);
int crit_set_data_mileage_write(unsigned int mileage);
int crit_set_data_gps_write(float lat, float lon, unsigned int utc_sec);
int crit_set_data_mileage_detail_write(unsigned int mileage, int lastyear, int lastmon, int lastday, float lastlat, float lastlon);
int crit_clear_write(void);

void crit_backup_simul(void);
#endif


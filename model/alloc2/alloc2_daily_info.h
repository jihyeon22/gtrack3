#ifndef __ALLOC2_DAILY_INFO_H__
#define __ALLOC2_DAILY_INFO_H__


#define ALLOC2_DAILY_INFO_FAIL  -1
#define ALLOC2_DAILY_INFO_SUCCESS  1

#define ALLOC2_CAR_DAILY_INFO "/data/mds/data/alloc_daily_info.dat"

int alloc2_init_car_daily_info();

int save_daily_info__total_distance(int yyyymmdd, int total_distance);
int get_daily_info__daily_distance(int total_disatance);

typedef struct {
    unsigned int over_speed_cnt;
}ALLOC2_DAILY_OVERSPEED_MGR_T;
// g_over_speed_mgr

int set_overspeed_info(gpsData_t* gps_info);
int get_overspeed_info(ALLOC2_DAILY_OVERSPEED_MGR_T* over_speed_info);

#endif


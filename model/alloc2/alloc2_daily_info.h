#ifndef __ALLOC2_DAILY_INFO_H__
#define __ALLOC2_DAILY_INFO_H__



#define ALLOC2_CAR_DAILY_INFO "/data/mds/data/alloc_daily_info.dat"

int alloc2_init_car_daily_info();

int save_daily_info__total_distance(int yyyymmdd, int total_distance);
int get_daily_info__daily_distance(int total_disatance);


#endif


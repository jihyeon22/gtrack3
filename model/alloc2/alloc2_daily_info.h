#ifndef __ALLOC2_DAILY_INFO_H__
#define __ALLOC2_DAILY_INFO_H__

#include <board/board_system.h>

#define ALLOC2_DAILY_INFO_FAIL  -1
#define ALLOC2_DAILY_INFO_SUCCESS  1

//jwrho persistant data path modify++
//#define ALLOC2_CAR_DAILY_INFO "/data/mds/data/alloc_daily_info.dat"
#define ALLOC2_CAR_DAILY_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_daily_info.dat")
//jwrho persistant data path modify--

int alloc2_init_car_daily_info();

int save_daily_info__total_distance(int yyyymmdd, int total_distance);
int get_daily_info__daily_distance(int total_disatance);

typedef struct {
    unsigned int over_speed_cnt;
}ALLOC2_DAILY_OVERSPEED_MGR_T;
// g_over_speed_mgr

int set_overspeed_info(gpsData_t* gps_info);
int get_overspeed_info(ALLOC2_DAILY_OVERSPEED_MGR_T* over_speed_info);


int chk_keyon_section_distance(int total_distance);
int init_keyon_section_distance(int total_distance);

int get_diff_distance_prev(int total_distance);
int init_diff_distance_prev();

int set_total_gps_distance(int distance);

typedef struct {
    unsigned int 	diff_last_distance;  // 이전 gps 좌표와의 거리?
    unsigned int 	keyon_distance;   // key on distance
}__attribute__((packed))ALLOC_RESUME_DATA;

void save_resume_data();
void load_resume_data();

#endif


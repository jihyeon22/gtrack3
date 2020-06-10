#pragma once

#include <board/board_system.h>
#include <base/gpstool.h>
#include <stdio.h>

//jwrho persistant data path modify++
//#define GEO_FENCE_V2_SETUP_DATA_FILE    "/data/mds/data/geo_fence.dat"
#define GEO_FENCE_V2_SETUP_DATA_FILE    CONCAT_STR(USER_DATA_DIR, "/geo_fence_v2.dat")
//#define GEO_FENCE_V2_STATUS_FILE	     "/data/mds/data/geo_fence_status.dat"
#define GEO_FENCE_V2_STATUS_FILE	     CONCAT_STR(USER_DATA_DIR, "/geo_fence_status_v2.dat")
//jwrho persistant data path modify--

#define GEO_FENCE_V2_MAX_COUNT				100
//#define WAIT_TIME_UNTIL_NEXT_GEO_EVENT	30	//unit : unit(sec)
#define WAIT_TIME_UNTIL_NEXT_GEO_EVENT	3	//unit : unit(sec)

typedef enum fence_v2_setup_info fence_v2_setup_info_t;
enum fence_v2_setup_info{
	eFENCE_V2_SETUP_UNKNOWN,
	eFENCE_V2_SETUP_ENTRY,
	eFENCE_V2_SETUP_EXIT,
	eFENCE_V2_SETUP_ENTRY_EXIT,
};

/*
typedef enum fence_status fence_status_t;
enum fence_status{
	eFENCE_STATUS_UNKOWN,
	eFENCE_STATUS_IN,
	eFENCE_STATUS_OUT,
	eFENCE_STATUS_IGNORE,
};
*/

typedef enum fence_v2_notification fence_v2_notification_t;
enum fence_v2_notification{
	eFENCE_V2_NONE_NOTIFICATION,
	eFENCE_V2_IN_NOTIFICATION,
	eFENCE_V2_OUT_NOTIFICATION,
};

typedef enum geo_fence_v2_enable geo_fence_v2_enable_t;
enum geo_fence_v2_enable{
	eGEN_FENCE_V2_DISABLE,
	eGEN_FENCE_V2_ENABLE,
};


typedef enum geo_fence_v2_debug_mode geo_fence_v2_debug_mode_t;
enum geo_fence_v2_debug_mode{
	eGEN_FENCE_V2_DEBUG_MODE,
	eGEN_FENCE_V2_NORMAL_MODE,
};

typedef enum geo_fence_v2_load_mode geo_fence_v2_load_mode_t;
enum geo_fence_v2_load_mode{
	eGEN_FENCE_V2_READ_SAVED_DATA_MODE,
	eGEN_FENCE_V2_NO_READ_SAVED_DATA_MODE,
};


#pragma pack(push, 1)

typedef struct {
	geo_fence_v2_enable_t enable;
	double latitude;
	double longitude;
	int range;
	fence_v2_setup_info_t setup_fence_status;
}__attribute__((packed))geo_fence_v2_setup_t;


typedef struct {
	fence_v2_notification_t cur_fence_status;
}__attribute__((packed))geo_fence_v2_status_t;

#pragma pack(pop)


int init_geo_fence_v2(geo_fence_v2_debug_mode_t debug_mode, geo_fence_v2_load_mode_t load_mode );
int clear_init_all_geo_fence_v2();
int deinit_geo_fence_v2();
int set_geo_fence_setup_info_v2(int idx, geo_fence_v2_setup_t *data);
int get_geo_fence_setup_info_v2(int idx, geo_fence_v2_setup_t *data);
fence_v2_notification_t get_geofence_notification_v2(int *pfence_num, gpsData_t cur_gps);
int save_geo_fence_status_info_v2();
int save_geo_fence_setup_info_v2();

void print_geo_fence_status_v2(FILE * fd);

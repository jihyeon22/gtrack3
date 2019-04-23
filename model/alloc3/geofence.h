#pragma once

#include <board/board_system.h>


//#define GEO_FENCE_SETUP_DATA_FILE    "/data/mds/data/geo_fence.dat"
#define GEO_FENCE_SETUP_DATA_FILE    CONCAT_STR(USER_DATA_DIR, "/geo_fence.dat")
//#define GEO_FENCE_STATUS_FILE	     "/data/mds/data/geo_fence_status.dat"
#define GEO_FENCE_STATUS_FILE	     CONCAT_STR(USER_DATA_DIR, "/geo_fence_status.dat")
//jwrho persistant data path modify--

#define GEO_FENCE_MAX_COUNT				100
//#define WAIT_TIME_UNTIL_NEXT_GEO_EVENT	30	//unit : unit(sec)
#define WAIT_TIME_UNTIL_NEXT_GEO_EVENT	3	//unit : unit(sec)

typedef enum fence_setup_info fence_setup_info_t;
enum fence_setup_info{
	eFENCE_SETUP_UNKNOWN,
	eFENCE_SETUP_ENTRY,
	eFENCE_SETUP_EXIT,
	eFENCE_SETUP_ENTRY_EXIT,
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

typedef enum fence_notification fence_notification_t;
enum fence_notification{
	eFENCE_NONE_NOTIFICATION,
	eFENCE_IN_NOTIFICATION,
	eFENCE_OUT_NOTIFICATION,
};

typedef enum geo_fence_eable geo_fence_eable_t;
enum geo_fence_eable{
	eGEN_FENCE_DISABLE,
	eGEN_FENCE_ENABLE,
};


typedef enum geo_fence_debug_mode geo_fence_debug_mode_t;
enum geo_fence_debug_mode{
	eGEN_FENCE_DEBUG_MODE,
	eGEN_FENCE_NORMAL_MODE,
};


#pragma pack(push, 1)

typedef struct {
	geo_fence_eable_t enable;
	double latitude;
	double longitude;
	int range;
	fence_setup_info_t setup_fence_status;
}__attribute__((packed))geo_fence_setup_t;


typedef struct {
	fence_notification_t cur_fence_status;
}__attribute__((packed))geo_fence_status_t;

#pragma pack(pop)


int init_geo_fence(geo_fence_debug_mode_t debug_mode);
int set_geo_fence_setup_info(int idx, geo_fence_setup_t *data);
int get_geo_fence_setup_info(int idx, geo_fence_setup_t *data);
fence_notification_t get_geofence_notification(int *pfence_num, gpsData_t cur_gps);
int save_geo_fence_status_info();
int save_geo_fence_setup_info();
int get_recent_geo_fence(void);
void set_recent_geo_fence(int in);
int get_first_geo_fence(void);


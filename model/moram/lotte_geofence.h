#pragma once

#ifdef TEST_CODE_ENABLE
	#define GEO_FENCE_STATUS_FILE	"./geo_fence.dat"
#else
	#define GEO_FENCE_STATUS_FILE	"/data/geo_fence.dat"
#endif

#define WAIT_TIME_UNTIL_NEXT_GEO_EVENT	30	//unit : unit(sec)

typedef enum fence_setup_info fence_setup_info_t;
enum fence_setup_info{
	eFENCE_SETUP_UNKOWN,
	eFENCE_SETUP_ENTRY,
	eFENCE_SETUP_EXIT,
	eFENCE_SETUP_ENTRY_EXIT,
};

typedef enum fence_status fence_status_t;
enum fence_status{
	eFENCE_STATUS_UNKOWN,
	eFENCE_STATUS_IN,
	eFENCE_STATUS_OUT,
	eFENCE_STATUS_IGNORE,
};

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


#pragma pack(push, 1)

typedef struct {
	geo_fence_eable_t enable;
	float latitude;
	float longitude;
	int range;
	fence_status_t cur_fence_status;
	fence_setup_info_t setup_fence_status;
}__attribute__((packed))geo_fence_data_t;

typedef struct {
	geo_fence_data_t fence0;
	geo_fence_data_t fence1;
}__attribute__((packed))geo_fence_hdr_t;

#pragma pack(pop)



int init_geo_fence();
int set_geo_fence0(geo_fence_data_t data);
int set_geo_fence1(geo_fence_data_t data);
geo_fence_eable_t get_geo_fence0_enable();
geo_fence_eable_t get_geo_fence1_enable();
fence_notification_t get_geofence0_notification(gpsData_t cur_gps);
fence_notification_t get_geofence1_notification(gpsData_t cur_gps);

/*
float get_geo_fence0_latitude();
float get_geo_fence0_longitude();
float get_geo_fence1_latitude();
float get_geo_fence1_longitude();
fence_status_t get_geo_fence0_status();
fence_status_t get_geo_fence1_status();

fence_setup_info_t get_geo_fence0_setup_info();
fence_setup_info_t get_geo_fence1_setup_info();

fence_notification_t get_geofence0_notification(gpsData_t cur_gps);
fence_notification_t get_geofence1_notification(gpsData_t cur_gps);
*/

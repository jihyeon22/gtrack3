#pragma once

#include <base/gpstool.h>

//======================================================
//         FEATURE_FAKE_TIME_INACTIVE_GPS
//
// time field of inactive gps replace with current system time
//#define FEATURE_FAKE_TIME_INACTIVE_GPS	
//======================================================

//======================================================
//      FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS
// inactive gps replace latest credible gps position
// time field have to create with current system time.
//#define FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS
//======================================================

//======================================================
//      FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS
// inactive gps replace latest credible gps position
// time field have to create with current system time.
#define FEATURE_INVALID_GPS_COPY_ZERO
//======================================================

//======================================================
//      FEATURE_GEO_FENCE_SIMULATION
// geo fence0 test simulation feature
//#ifdef DEBUG
//	#define FEATURE_GEO_FENCE_SIMULATION
//#endif
//======================================================


#define MILEAGE_NOT_INIT				(-1)
#define GPS_INIT_MAX_WAITING_TIME		(120)  //unit : sec
#define MILEAGE_INTERVAL	10

typedef enum gps_active_status gps_active_status_t;
enum gps_active_status{
	eINACTIVE = 0,
	eACTIVE = 1,
};

typedef enum gps_condition gps_condition_t;
enum gps_condition{
	eUNKNOWN_GPS_DATA,
	eSKIP_NO_INITIAL_GPS,
	eSKIP_NO_COLLECTION_TIME,
	eSKIP_DISTANCE_FILTER,
	eSKIP_TRUST_SATLITES_FILTER,
	eSKIP_TRUST_HDOP_FILTER,
	eSKIP_TRUST_SPEED_FILTER,
	eUSE_GPS_DATA
};


typedef struct {
	gps_active_status_t first_gps_active;
	int gps_initial_count;

	int server_mileage;
	double gps_mileage;
	
	gpsData_t fixed_gpsdata;
	gpsData_t last_gpsdata;
	gpsData_t reported_gpsdata;
}lotte_gps_manager_t;



void init_gps_manager();
lotte_gps_manager_t* load_gps_manager();
gps_condition_t active_gps_process(gpsData_t cur_gps, gpsData_t *pData);
gps_condition_t inactive_gps_process(gpsData_t cur_gps, gpsData_t *pData);
int get_server_mileage();
int get_gps_mileage();
void set_server_mileage(int mileage);
void set_gps_mileage(int mileage);

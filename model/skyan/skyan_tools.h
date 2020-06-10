<<<<<<< HEAD
#ifndef __SKYAN__TOOLS_H__
#define __SKYAN__TOOLS_H__

#include "packet.h"

#define SKYAN_APP_VER_STR   "01.01"

#define SKYAN_KEY_STAT_ON       1
#define SKYAN_KEY_STAT_OFF      0
#define SKYAN_KEY_STAT_INVALID  -1


#define SKYAN_TOOLS__GPS_ANT_STAT_OK     1
#define SKYAN_TOOLS__GPS_ANT_STAT_NOK    0

#define SKYAN_TOOLS__CHK_INTERVAL       0
#define SKYAN_TOOLS__CHK_IMMEDIATELY    1

#define BATT_CHK_INTERVAL_SEC        60
#define GPS_ANT_CHK_INTERVAL_SEC     60

#define SKYAN_TOOLS__NOSTART_MODE   1
#define SKYAN_TOOLS__NORMAL_MODE    2

#include "board/board_system.h"
#define SKYAN_RESUME_DATA_PATH_V1    CONCAT_STR(USER_DATA_DIR, "/skyan_resum_data_v1.dat")

typedef struct {
    unsigned int    nostart_flag;  // nostart flag
}__attribute__((packed))SKYAN_RESUME_DATA_V1_T;


// ---------------------------------------------------
// batt tools
// ---------------------------------------------------
int skyan_tools__get_car_batt_level();
int skyan_tools__chk_car_batt_level(int low_batt, int chk_flag);

int skyan_tools__chk_car_batt_level(int low_batt, int chk_flag);
int skyan_tools__set_gps_ant_stat(int stat);
int skyan_tools__get_gps_stat(int gps_stat);

int skyan_tools__get_low_batt_level();
int skyan_tools__set_low_batt_level(int low_batt);

int skyan_tools__get_key_on_time();
int skyan_tools__get_key_off_time();
int skyan_tools__set_key_on_time(int cur_time);
int skyan_tools__set_key_off_time(int cur_time);
int skyan_tools__set_key_stat(int key_stat);
int skyan_tools__get_key_stat();

int skyan_tools__get_devstat();
int skyan_tools__set_devstat(int bit);
int skyan_tools__clear_devstat(int bit);
int skyan_tools__get_devstat_is_set(int bit);

int skyan_tools__get_nostart_flag();
int skyan_tools__set_nostart_flag(int mode);

int skyan_tools__get_firm_ver(char* ver_str);

void skyan_tools__save_resume_data();
void skyan_tools__load_resume_data();

void skyan_tools__set_setting_info(SKY_AUTONET_PKT__SETTING_INFO_T* setting_info);
SKY_AUTONET_PKT__SETTING_INFO_T* skyan_tools__get_setting_info();


#endif // __SKYAN__TOOLS_H__
=======
#ifndef __SKYAN__TOOLS_H__
#define __SKYAN__TOOLS_H__

#include "packet.h"

#define SKYAN_APP_VER_STR   "01.01"

#define SKYAN_KEY_STAT_ON       1
#define SKYAN_KEY_STAT_OFF      0
#define SKYAN_KEY_STAT_INVALID  -1


#define SKYAN_TOOLS__GPS_ANT_STAT_OK     1
#define SKYAN_TOOLS__GPS_ANT_STAT_NOK    0

#define SKYAN_TOOLS__CHK_INTERVAL       0
#define SKYAN_TOOLS__CHK_IMMEDIATELY    1

#define BATT_CHK_INTERVAL_SEC        60
#define GPS_ANT_CHK_INTERVAL_SEC     60

#define SKYAN_TOOLS__NOSTART_MODE   1
#define SKYAN_TOOLS__NORMAL_MODE    2

#include "board/board_system.h"
#define SKYAN_RESUME_DATA_PATH_V1    CONCAT_STR(USER_DATA_DIR, "/skyan_resum_data_v1.dat")

typedef struct {
    unsigned int    nostart_flag;  // nostart flag
}__attribute__((packed))SKYAN_RESUME_DATA_V1_T;


// ---------------------------------------------------
// batt tools
// ---------------------------------------------------
int skyan_tools__get_car_batt_level();
int skyan_tools__chk_car_batt_level(int low_batt, int chk_flag);

int skyan_tools__chk_car_batt_level(int low_batt, int chk_flag);
int skyan_tools__set_gps_ant_stat(int stat);
int skyan_tools__get_gps_stat(int gps_stat);

int skyan_tools__get_low_batt_level();
int skyan_tools__set_low_batt_level(int low_batt);

int skyan_tools__get_key_on_time();
int skyan_tools__get_key_off_time();
int skyan_tools__set_key_on_time(int cur_time);
int skyan_tools__set_key_off_time(int cur_time);
int skyan_tools__set_key_stat(int key_stat);
int skyan_tools__get_key_stat();

int skyan_tools__get_devstat();
int skyan_tools__set_devstat(int bit);
int skyan_tools__clear_devstat(int bit);
int skyan_tools__get_devstat_is_set(int bit);

int skyan_tools__get_nostart_flag();
int skyan_tools__set_nostart_flag(int mode);

int skyan_tools__get_firm_ver(char* ver_str);

void skyan_tools__save_resume_data();
void skyan_tools__load_resume_data();

void skyan_tools__set_setting_info(SKY_AUTONET_PKT__SETTING_INFO_T* setting_info);
SKY_AUTONET_PKT__SETTING_INFO_T* skyan_tools__get_setting_info();


#endif // __SKYAN__TOOLS_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

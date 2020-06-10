
#ifndef __CL_RFID_PASSENGER_H__
#define __CL_RFID_PASSENGER_H__

// WEB 에서 한번에 받아올수있는 최대갯수
#define MAX_RFID_USER_SAVE          33000
#define MAX_WRITE_FAIL_RETRY_CNT    10

typedef struct rfid_user_info
{
    char rfid_uid[64];
    int day_limit;
    int is_use;
    int boarding_cont;
    char last_boarding_date[64];
}RFID_USER_INFO_T;

typedef struct rifd_user_mgr
{
    int rfid_user_idx;
    RFID_USER_INFO_T user_info[MAX_RFID_USER_SAVE];
}RFID_USER_MGR_T;

// rfid 리더기에서 한번에 읽히는 최대 승객 길이가 256바이트기 때문에 너무길 필요가 없다.
#define MAX_BOARDING_USER          20

typedef struct rfid_boarding_info
{
    char rfid_uid[64];
    int boarding;
    char date[64];
    int chk_result;
}RFID_BOARDING_INFO_T;

typedef struct rifd_boarding_mgr
{
    int rfid_boarding_idx;
    RFID_BOARDING_INFO_T boarding_info[MAX_BOARDING_USER];
}RFID_BOARDING_MGR_T;

typedef enum RFID_PKT_STATUS
{
    e_RFID_NONE = 0,
    e_RFID_INIT = 1,
    e_NEED_TO_RFID_USER_CHK = 2,
    e_RFID_DOWNLOAD_START =3 ,
    e_RFID_DOWNLOAD_END = 4,
    e_RFID_USER_INFO_WRITE_TO_DEV_START = 5,
    e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS = 6,
    e_RFID_USER_INFO_WRITE_TO_DEV_FAIL = 7,
    e_RFID_USER_INFO_CHK_READY = e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS,
    e_RFID_FIRMWARE_DOWNLOAD_START = 8,
    e_RFID_FIRMWARE_DOWNLOAD_ING = 9,
    e_RFID_FIRMWARE_DOWNLOAD_END = e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS,
};


// ------------------------------------------------

int rfid_tool__set_senario_stat(int stat);
int rfid_tool__get_senario_stat();
char* rfid_tool__get_senario_stat_str();

// ------------------------------------------------

int rfid_tool__user_info_init();
int rfid_tool__user_info_insert(RFID_USER_INFO_T rfid_user);
int rfid_tool__user_info_total_cnt();
int rfid_tool__user_info_get(int idx, RFID_USER_INFO_T* rfid_user);
int rfid_tool__user_info_exist_chk(char* user_str);

// ------------------------------------------------

int rfid_tool__env_set_all_clear(int flag);
int rfid_tool__env_get_all_clear();

// ------------------------------------------------
#define RFID_CHK_DEFAULT_INTERVAL_SEC    30
#define RFID_CHK_READ_MORE_INTERVAL_SEC    5

int rfid_tool__env_set_rfid_chk_interval(int sec);
int rfid_tool__env_get_rfid_chk_interval();

// ------------------------------------------------
#define RFID_CONN_STAT_OK    1
#define RFID_CONN_STAT_NOK   -1

int rfid_tool__set_rifd_dev_stat(int stat);
int rfid_tool__get_rifd_dev_stat();
char* rfid_tool__get_rifd_dev_stat_str();

#endif // __CL_RFID_PASSENGER_H__




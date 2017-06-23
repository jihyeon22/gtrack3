
#ifndef __CL_RFID_PASSENGER_H__
#define __CL_RFID_PASSENGER_H__



#define MAX_RFID_USER_SAVE  30000

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



#endif // __CL_RFID_PASSENGER_H__



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logd_rpc.h>

#include "cl_rfid_passenger.h"

static RFID_USER_MGR_T g_rifd_user_mgr = {0,};

int user_info_init()
{
    memset(&g_rifd_user_mgr,0x00, sizeof(g_rifd_user_mgr));
    g_rifd_user_mgr.rfid_user_idx = 0;
}

int user_info_insert(RFID_USER_INFO_T rfid_user)
{
    int idx = g_rifd_user_mgr.rfid_user_idx;

    // init
    memset(&g_rifd_user_mgr.user_info[idx], 0x00, sizeof(RFID_USER_INFO_T));

#if 0
    memcpy(rifd_user_mgr.user_info[idx], rfid_user, sizeof(RFID_USER_INFO_T));
#else
    strcpy(g_rifd_user_mgr.user_info[idx].rfid_uid, rfid_user.rfid_uid);
    g_rifd_user_mgr.user_info[idx].day_limit = rfid_user.day_limit;
    g_rifd_user_mgr.user_info[idx].is_use = rfid_user.is_use;
    g_rifd_user_mgr.user_info[idx].boarding_cont = rfid_user.boarding_cont;
    strcpy(g_rifd_user_mgr.user_info[idx].last_boarding_date, rfid_user.last_boarding_date);
#endif
    // increase count
    printf("g_rifd_user_mgr.user_info[%d].rfid_uid => [%s]\r\n", idx, g_rifd_user_mgr.user_info[idx].rfid_uid);
    printf("g_rifd_user_mgr.user_info[%d].day_limit => [%d]\r\n", idx, g_rifd_user_mgr.user_info[idx].day_limit);
    printf("g_rifd_user_mgr.user_info[%d].is_use => [%d]\r\n", idx, g_rifd_user_mgr.user_info[idx].is_use);
    printf("g_rifd_user_mgr.user_info[%d].boarding_cont => [%d]\r\n", idx, g_rifd_user_mgr.user_info[idx].boarding_cont);
    printf("g_rifd_user_mgr.user_info[%d].last_boarding_date => [%s]\r\n", idx, g_rifd_user_mgr.user_info[idx].last_boarding_date);

    g_rifd_user_mgr.rfid_user_idx++;
}



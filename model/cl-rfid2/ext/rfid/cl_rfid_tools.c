#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logd_rpc.h>

#include "ext/rfid/cl_rfid_tools.h"
#include "ext/rfid/kjtec_rfid/kjtec_rfid_tools.h"
#include "ext/rfid/kjtec_rfid/kjtec_rfid_cmd.h"

static int _g_rfid_pkt_stat = e_RFID_INIT;

int rfid_tool__set_senario_stat(int stat)
{
    /*
    if ( stat == e_RFID_FIRMWARE_DOWNLOAD_START )
    {
        if ( _g_rfid_pkt_stat != e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS )
        {
            //LOGI(LOG_TARGET, "[FWDOWN] INVALID STAT [%d]\n", rfid_tool__get_senario_stat()  );
            devel_webdm_send_log("[FWDOWN] INVALID STAT [%d]\n", rfid_tool__get_senario_stat()  );
            kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN CHK => FAIL : INVALID STAT. TRY LATER");
            return _g_rfid_pkt_stat;
        }
    }
    */
    
    _g_rfid_pkt_stat = stat;
    
    return _g_rfid_pkt_stat;
}

int rfid_tool__get_senario_stat()
{
    return _g_rfid_pkt_stat;
}

char* rfid_tool__get_senario_stat_str()
{
    int cur_stat = rfid_tool__get_senario_stat();
    switch(cur_stat)
    {
        case  e_RFID_NONE:
        {
            clear_req_passenger_fail_cnt();
            return "RFID_NONE";
            break;
        }
        case  e_RFID_INIT:
        {
            clear_req_passenger_fail_cnt();
            return "RFID_INIT";
            break;
        }
        case  e_NEED_TO_RFID_USER_CHK:
        {
            return "NEED_TO_USER_CHK";
            break;
        }
        case  e_RFID_DOWNLOAD_START:
        {
            return "DOWNLOAD_START";
            break;
        }
        case  e_RFID_DOWNLOAD_END:
        {
            return "DOWNLOAD_END-WRITE_DEV";
            break;
        }
        case  e_RFID_USER_INFO_WRITE_TO_DEV_START:
        {
            return "DEV_WRITE_START";
            break;
        }
        case  e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS:
        {
            return "DEV_WRITE_SUCCESS-RFID_READ";
            break;
        }
        case  e_RFID_USER_INFO_WRITE_TO_DEV_FAIL:
        {
            return "DEV_WRITE_FAIL";
            break;
        }
        case  e_RFID_FIRMWARE_DOWNLOAD_START:
        {
            return "DEV_FIRMWARE_DOWNLOAD_START";
            break;
        }
        case  e_RFID_FIRMWARE_DOWNLOAD_ING:
        {
            return "DEV_FIRMWARE_DOWNLOAD_ING";
            break;
        }
        /*
        case  e_RFID_FIRMWARE_DOWNLOAD_END:
        {
            return "DEV_FIRMWARE_DOWNLOAD_END";
            break;
        }
        */
        /*
        case  e_RFID_USER_INFO_CHK_READY:
        {
            return "USER INFO CHK READY";
            break;
        }*/
        default : 
        {
            return "STAT_NULL";
            break;
        }
    }
    
}
// --------------------------------------------------------
static RFID_USER_MGR_T _g_rifd_user_mgr = {0,};

int rfid_tool__user_info_init()
{
    memset(&_g_rifd_user_mgr,0x00, sizeof(_g_rifd_user_mgr));
    _g_rifd_user_mgr.rfid_user_idx = 0;

    return 0;
}

int rfid_tool__user_info_insert(RFID_USER_INFO_T rfid_user)
{
    int idx = _g_rifd_user_mgr.rfid_user_idx;

    // init
    memset(&_g_rifd_user_mgr.user_info[idx], 0x00, sizeof(RFID_USER_INFO_T));

    // "init_data" 라는 스트링이 들어온 필드는 스킵
    // 향후 모두지우도록함.
    if ( strcasestr(rfid_user.rfid_uid, "INIT_DATA") != NULL )
    {
         rfid_tool__env_set_all_clear(1);
         devel_webdm_send_log("ALL CLR USER DATA. ");
         return 0;
    }

    if ( strlen(rfid_user.rfid_uid) <= 0 )
        return 0;
#if 0
    memcpy(rifd_user_mgr.user_info[idx], rfid_user, sizeof(RFID_USER_INFO_T));
#else
    strcpy(_g_rifd_user_mgr.user_info[idx].rfid_uid, rfid_user.rfid_uid);
    _g_rifd_user_mgr.user_info[idx].day_limit = rfid_user.day_limit;
    _g_rifd_user_mgr.user_info[idx].is_use = rfid_user.is_use;
    _g_rifd_user_mgr.user_info[idx].boarding_cont = rfid_user.boarding_cont;
    strcpy(_g_rifd_user_mgr.user_info[idx].last_boarding_date, rfid_user.last_boarding_date);
#endif
    // increase count
    /*
    printf("_g_rifd_user_mgr.user_info[%d].rfid_uid => [%s]\r\n", idx, _g_rifd_user_mgr.user_info[idx].rfid_uid);
    printf("_g_rifd_user_mgr.user_info[%d].day_limit => [%d]\r\n", idx, _g_rifd_user_mgr.user_info[idx].day_limit);
    printf("_g_rifd_user_mgr.user_info[%d].is_use => [%d]\r\n", idx, _g_rifd_user_mgr.user_info[idx].is_use);
    printf("_g_rifd_user_mgr.user_info[%d].boarding_cont => [%d]\r\n", idx, _g_rifd_user_mgr.user_info[idx].boarding_cont);
    printf("_g_rifd_user_mgr.user_info[%d].last_boarding_date => [%s]\r\n", idx, _g_rifd_user_mgr.user_info[idx].last_boarding_date);
*/
    _g_rifd_user_mgr.rfid_user_idx++;
    if  ( _g_rifd_user_mgr.rfid_user_idx > MAX_RFID_USER_SAVE )
        return -1;
  //  printf(" --> _g_rifd_user_mgr.rfid_user_idx [%d]\r\n", _g_rifd_user_mgr.rfid_user_idx);
    return 0;
}

int rfid_tool__user_info_total_cnt()
{
    return _g_rifd_user_mgr.rfid_user_idx;
    //return 4;
}

int rfid_tool__user_info_get(int idx, RFID_USER_INFO_T* rfid_user)
{
    int ret = 0;

    if ( ( idx >= 0 ) && ( idx < _g_rifd_user_mgr.rfid_user_idx ) )
    {
        strcpy(rfid_user->rfid_uid, _g_rifd_user_mgr.user_info[idx].rfid_uid);
        rfid_user->day_limit = _g_rifd_user_mgr.user_info[idx].day_limit;
        rfid_user->is_use = _g_rifd_user_mgr.user_info[idx].is_use;
        rfid_user->boarding_cont = _g_rifd_user_mgr.user_info[idx].boarding_cont;
        strcpy(rfid_user->last_boarding_date, _g_rifd_user_mgr.user_info[idx].last_boarding_date);
    }
    else
        ret = -1;

    return ret;
}


int rfid_tool__user_info_exist_chk(char* user_str)
{
    int i = 0 ;
    
    if ( rfid_tool__get_senario_stat() != e_RFID_DOWNLOAD_END )
        return 0;

    printf(" found start ! [%s]\r\n", user_str);
    for ( i = 0 ; i < _g_rifd_user_mgr.rfid_user_idx ; i ++ )
    {
        if ( strcmp(_g_rifd_user_mgr.user_info[i].rfid_uid, user_str ) == 0 )
        {
            printf("found uid [%s]\r\n", user_str);
            return 1;
        }
    }
    
    return 0;
}

// --------------------------------------------------------
static int _g_rfid_all_clr = 0;
int rfid_tool__env_set_all_clear(int flag)
{
    _g_rfid_all_clr = flag;
    return _g_rfid_all_clr;
} 

int rfid_tool__env_get_all_clear()
{
    return _g_rfid_all_clr;
    
} 

// --------------------------------------------------------
static int _g_rfid_chk_interval_sec = RFID_CHK_DEFAULT_INTERVAL_SEC;
int rfid_tool__env_set_rfid_chk_interval(int sec)
{
    _g_rfid_chk_interval_sec = sec;
    return _g_rfid_chk_interval_sec;
}

int rfid_tool__env_get_rfid_chk_interval()
{
    return _g_rfid_chk_interval_sec;
}


// ---------------------------------------------
static int _g_obd_dev_stat = 0;

int rfid_tool__set_rifd_dev_stat(int stat)
{
    _g_obd_dev_stat = stat;
    return _g_obd_dev_stat;
}

int rfid_tool__get_rifd_dev_stat()
{
    return _g_obd_dev_stat;
}

char* rfid_tool__get_rifd_dev_stat_str()
{
    switch (_g_obd_dev_stat)
    {
        case RFID_CONN_STAT_OK:
        {
            return "CONN";
            break;
        }
        case RFID_CONN_STAT_NOK:
        {
            return "DISCONN";
            break;
        }
        default:
        {
            return "UNKNOWN";
            break;
        }
    }
    return "UNKNOWN";
}


// -----------------------------

static int _g_parse_fail_cnt = 0;

int clear_req_passenger_fail_cnt()
{
    _g_parse_fail_cnt = 0;
    return _g_parse_fail_cnt;
}

int inc_req_passenger_fail_cnt()
{
    _g_parse_fail_cnt++;
    return _g_parse_fail_cnt; 
}

int get_req_passenger_fail_cnt()
{
    return _g_parse_fail_cnt; 
}
<<<<<<< HEAD
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/sender.h>
#include <base/devel.h>
#include <include/defines.h>

#include "logd/logd_rpc.h"

#include "netcom.h"
#include "callback.h"

#include <mobileye_adas.h>

#include "custom.h"
#include "ktth_adas_mgr.h"

#include <kt_thermal_mdt800/packet.h>

#define LOG_TARGET eSVC_MODEL

#define ADAS_DEV_FAIL_CHK_CNT     90
#define ADAS_DEV_NON_EVT_CLR_CNT    1

// TODO: tty 채널 을 꼭 확인해볼것
// NOTE: 현재 온도센서가 /dev/ttyHSL2 로 되어있다.
#define ADAS_DEV_PATH       "/dev/ttyHSL2"
#define ADAS_DEV_BAUDRATE   9600


// 기본적으로 event code 는 일반 주기보고로 한다.
static int ktth_adas_mgr_sendpkt(int evt_code, char* adas_data_str)
{
    ktthAdasData_t adas_data;
    memset(&adas_data, 0x00, sizeof(adas_data));

    adas_data.event_code = evt_code;
    
    if ( adas_data_str != NULL )
        strcpy(adas_data.adas_data_str, adas_data_str);
    
	sender_add_data_to_buffer(eMDS_CUSTOM_KT_ADAS_EVC, &adas_data, ePIPE_1);

    return 0;
}


int p_adas_bmsg_proc(ADAS_EVT_DATA_T* evt_data)
{
    static int err_code_last = -1;

    static int read_fail_cnt = 0;
    static int adas_stat_send_evt = -1;

    static int evt_none_cnt = 0;
    
    int adas_evt_stat = 0;

    char option_str[32] = {0,};
    int option_str_len = 0;

    //printf("p_bmsg_proc :: cur_movon_data.type1 [0x%02x]\r\n", cur_movon_data.type1);
    /*
    evt_data->evt_data_1;
    evt_data->evt_data_2;
    evt_data->evt_data_3;
    evt_data->evt_ext[32];
    */
    if ( evt_data == NULL)
    {
        read_fail_cnt++;
        goto FINISH;
    }

    // bypass 로만 넘어온다.
    // sync 
    if (strstr(evt_data->evt_ext, "$MESYNC") != NULL)    
    {
        evt_data->evt_code = eADAS_EVT__NONE;
    }

    switch(evt_data->evt_code)
    {
        case eADAS_EVT__BYPASS:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_ext >> ext data [%s]\r\n", evt_data->evt_ext);

            memset(option_str, 0x00, sizeof(option_str));
            option_str_len = 0;
            
            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_2);

            ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, evt_data->evt_ext);
            // ktth_adas_mgr_sendpkt(CL_ADAS_FPW_HMW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        // ----------------------------
        case eADAS_EVT__NONE:
        {
            evt_none_cnt++;

            // 한번에 여러 이벤트 처리를 위한 루틴으로 인해서
            // evt none 은 항상 1회씩 불린다.
            // 실제 none 이벤트는 여러번이 연속으로 와야 의미가 있다.

            printf("p_bmsg_proc(eADAS_EVT__NONE) :: evt_none_cnt [%d]\r\n", evt_none_cnt);

            if ( evt_none_cnt > ADAS_DEV_NON_EVT_CLR_CNT )
            {
                // LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__NONE) :: evt clear [%d]\r\n", evt_none_cnt);

                err_code_last = -1;

                read_fail_cnt = 0;

                adas_evt_stat = 1;
            }

            break;
        }
        case eADAS_EVT__INVALID:
        {
            read_fail_cnt++;

            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_code [%d] / [%d]\r\n", evt_data->evt_code, read_fail_cnt);
/*
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
*/
            break;
        }
        default:
        {
            break;
        }
    }

FINISH:
    memset(option_str, 0x00, sizeof(option_str));
    option_str_len = 0;

    if ( read_fail_cnt > 0 )
    {
        printf("read_fail_cnt is [%d]\r\n", read_fail_cnt);
    }

    if ( read_fail_cnt > ADAS_DEV_FAIL_CHK_CNT)
    {
        if (( adas_stat_send_evt == 0 ) || ( adas_stat_send_evt == -1 ) )
        {
            if ( ( nettool_get_state() == DEFINES_MDS_OK ) )
            {
                devel_webdm_send_log("[ADAS] DEV DISCONN : CANNOT COMM");
                option_str_len += sprintf(option_str + option_str_len, "%s" , "NC");

                //if ( g_stat_key_on == 1 )
                //    ktth_adas_mgr_sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);

                ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, "$MEMDSCHK,0*00");

                adas_stat_send_evt = 1;
                adas_evt_stat = 0; 
            }
        }
    }

    if ( ((adas_stat_send_evt==1) && (adas_evt_stat == 1) ) || ((adas_stat_send_evt==-1) && (adas_evt_stat==1)))
    {
        if ( ( nettool_get_state() == DEFINES_MDS_OK ) )
        {
            devel_webdm_send_log("[ADAS] DEV CONN : SUCCESS");
            //ktth_adas_mgr_sendpkt(CL_ADAS_PWR_ON, 0, NULL);
            ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, "$MEMDSCHK,1*00");

            adas_stat_send_evt = 0;

        }
    }

    return 0;
}

int ktth_adas_mgr__init()
{
    mobileye_adas__mgr_init(ADAS_DEV_PATH, ADAS_DEV_BAUDRATE, p_adas_bmsg_proc);

    return 0;

=======
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/sender.h>
#include <base/devel.h>
#include <include/defines.h>

#include "logd/logd_rpc.h"

#include "netcom.h"
#include "callback.h"

#include <mobileye_adas.h>

#include "custom.h"
#include "ktth_adas_mgr.h"

#include <kt_thermal_mdt800/packet.h>

#define LOG_TARGET eSVC_MODEL

#define ADAS_DEV_FAIL_CHK_CNT     90
#define ADAS_DEV_NON_EVT_CLR_CNT    1

// TODO: tty 채널 을 꼭 확인해볼것
// NOTE: 현재 온도센서가 /dev/ttyHSL2 로 되어있다.
#define ADAS_DEV_PATH       "/dev/ttyHSL2"
#define ADAS_DEV_BAUDRATE   9600


// 기본적으로 event code 는 일반 주기보고로 한다.
static int ktth_adas_mgr_sendpkt(int evt_code, char* adas_data_str)
{
    ktthAdasData_t adas_data;
    memset(&adas_data, 0x00, sizeof(adas_data));

    adas_data.event_code = evt_code;
    
    if ( adas_data_str != NULL )
        strcpy(adas_data.adas_data_str, adas_data_str);
    
	sender_add_data_to_buffer(eMDS_CUSTOM_KT_ADAS_EVC, &adas_data, ePIPE_1);

    return 0;
}


int p_adas_bmsg_proc(ADAS_EVT_DATA_T* evt_data)
{
    static int err_code_last = -1;

    static int read_fail_cnt = 0;
    static int adas_stat_send_evt = -1;

    static int evt_none_cnt = 0;
    
    int adas_evt_stat = 0;

    char option_str[32] = {0,};
    int option_str_len = 0;

    //printf("p_bmsg_proc :: cur_movon_data.type1 [0x%02x]\r\n", cur_movon_data.type1);
    /*
    evt_data->evt_data_1;
    evt_data->evt_data_2;
    evt_data->evt_data_3;
    evt_data->evt_ext[32];
    */
    if ( evt_data == NULL)
    {
        read_fail_cnt++;
        goto FINISH;
    }

    // bypass 로만 넘어온다.
    // sync 
    if (strstr(evt_data->evt_ext, "$MESYNC") != NULL)    
    {
        evt_data->evt_code = eADAS_EVT__NONE;
    }

    switch(evt_data->evt_code)
    {
        case eADAS_EVT__BYPASS:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
            printf("p_bmsg_proc(eADAS_EVT__BYPASS) :: evt_data->evt_ext >> ext data [%s]\r\n", evt_data->evt_ext);

            memset(option_str, 0x00, sizeof(option_str));
            option_str_len = 0;
            
            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_2);

            ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, evt_data->evt_ext);
            // ktth_adas_mgr_sendpkt(CL_ADAS_FPW_HMW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        // ----------------------------
        case eADAS_EVT__NONE:
        {
            evt_none_cnt++;

            // 한번에 여러 이벤트 처리를 위한 루틴으로 인해서
            // evt none 은 항상 1회씩 불린다.
            // 실제 none 이벤트는 여러번이 연속으로 와야 의미가 있다.

            printf("p_bmsg_proc(eADAS_EVT__NONE) :: evt_none_cnt [%d]\r\n", evt_none_cnt);

            if ( evt_none_cnt > ADAS_DEV_NON_EVT_CLR_CNT )
            {
                // LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__NONE) :: evt clear [%d]\r\n", evt_none_cnt);

                err_code_last = -1;

                read_fail_cnt = 0;

                adas_evt_stat = 1;
            }

            break;
        }
        case eADAS_EVT__INVALID:
        {
            read_fail_cnt++;

            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_code [%d] / [%d]\r\n", evt_data->evt_code, read_fail_cnt);
/*
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
*/
            break;
        }
        default:
        {
            break;
        }
    }

FINISH:
    memset(option_str, 0x00, sizeof(option_str));
    option_str_len = 0;

    if ( read_fail_cnt > 0 )
    {
        printf("read_fail_cnt is [%d]\r\n", read_fail_cnt);
    }

    if ( read_fail_cnt > ADAS_DEV_FAIL_CHK_CNT)
    {
        if (( adas_stat_send_evt == 0 ) || ( adas_stat_send_evt == -1 ) )
        {
            if ( ( nettool_get_state() == DEFINES_MDS_OK ) )
            {
                devel_webdm_send_log("[ADAS] DEV DISCONN : CANNOT COMM");
                option_str_len += sprintf(option_str + option_str_len, "%s" , "NC");

                //if ( g_stat_key_on == 1 )
                //    ktth_adas_mgr_sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);

                ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, "$MEMDSCHK,0*00");

                adas_stat_send_evt = 1;
                adas_evt_stat = 0; 
            }
        }
    }

    if ( ((adas_stat_send_evt==1) && (adas_evt_stat == 1) ) || ((adas_stat_send_evt==-1) && (adas_evt_stat==1)))
    {
        if ( ( nettool_get_state() == DEFINES_MDS_OK ) )
        {
            devel_webdm_send_log("[ADAS] DEV CONN : SUCCESS");
            //ktth_adas_mgr_sendpkt(CL_ADAS_PWR_ON, 0, NULL);
            ktth_adas_mgr_sendpkt(eCYCLE_REPORT_EVC, "$MEMDSCHK,1*00");

            adas_stat_send_evt = 0;

        }
    }

    return 0;
}

int ktth_adas_mgr__init()
{
    mobileye_adas__mgr_init(ADAS_DEV_PATH, ADAS_DEV_BAUDRATE, p_adas_bmsg_proc);

    return 0;

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}
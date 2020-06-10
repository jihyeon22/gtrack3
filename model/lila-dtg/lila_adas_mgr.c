#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <base/sender.h>
#include <base/devel.h>
#include <base/gpstool.h>

#include "logd/logd_rpc.h"

#include <include/defines.h>
#include "netcom.h"
#include "callback.h"


// #define USE_MOVON_ADAS // temp code : for code highligh

#include <mobileye_adas.h>
#include "lila_adas_mgr.h"
#include "data-list.h"

#define LOG_TARGET eSVC_MODEL

#define ADAS_DEV_FAIL_CHK_CNT     90
#define ADAS_DEV_NON_EVT_CLR_CNT    1

#define ADAS_DEV_PATH       "/dev/ttyHSL2"
#define ADAS_DEV_BAUDRATE   9600


static int lila_adas_mgr__set_evt(adas_evt_code_t evt_code, int ext_data)
{
    LILA_ADAS__DATA_T*      p_lila_adas_data;
    gpsData_t cur_dtg_gps_data;
    p_lila_adas_data = malloc(sizeof(LILA_ADAS__DATA_T));

    taco_gtrack_tool__get_cur_gps_data(&cur_dtg_gps_data);

    // p_lila_adas_data->time_sec = get_modem_time_utc_sec();
    p_lila_adas_data->time_sec = cur_dtg_gps_data.utc_sec;
    p_lila_adas_data->evt_code = evt_code;
    p_lila_adas_data->ext_data = ext_data;

    if(list_add(&lila_adas_data_buffer_list, p_lila_adas_data) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
		free(p_lila_adas_data);
	}

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

    switch(evt_data->evt_code)
    {
        case eADAS_EVT__FCW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);

            lila_adas_mgr__set_evt(eADAS_CODE__FCW, 0);
            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__UFCW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__UFCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);

            lila_adas_mgr__set_evt(eADAS_CODE__FCW, 0);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__LDW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            if ( evt_data->evt_data_4 == 0 )
                lila_adas_mgr__set_evt(eADAS_CODE__LDW_L, 0);
            else
                lila_adas_mgr__set_evt(eADAS_CODE__LDW_R, 0);

            err_code_last = -1;
            
            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__PCW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            lila_adas_mgr__set_evt(eADAS_CODE__PCW, 0);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__SLI:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            // do nothing..

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__TSR:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__TSR) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            // do nothing..

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__HMW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            lila_adas_mgr__set_evt(eADAS_CODE__HMW, evt_data->evt_data_2);

           // lila_adas_mgr__sendpkt(CL_ADAS_HMW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__FPW:
        {
            LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);

            // do nothing..

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__ERR:
        {
            //printf("p_bmsg_proc(eADAS_EVT__ERR) :: err_code_last [%d] / evt_data->evt_data_4 [%d]\r\n",err_code_last, evt_data->evt_data_4);
            if ( ( err_code_last != evt_data->evt_data_4 ) )
            {
                LOGT(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_code [%d] / [%d]\r\n", evt_data->evt_code, evt_data->evt_data_4);

                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);


                //option_str_len += sprintf(option_str + option_str_len, "%s" , evt_data->evt_ext );

                //lila_adas_mgr__sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);

                err_code_last = evt_data->evt_data_4;

            }
            else
            {
                //printf("p_bmsg_proc(eADAS_EVT__ERR) :: case 00000\r\n");
            }
            
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

            // printf("p_bmsg_proc(eADAS_EVT__NONE) :: evt_none_cnt [%d]\r\n", evt_none_cnt);

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

            // LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__INVALID) :: evt_data->evt_code [%d] / [%d]\r\n", evt_data->evt_code, read_fail_cnt);
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
        // printf("read_fail_cnt is [%d]\r\n", read_fail_cnt);
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
                //    lila_adas_mgr__sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);
                
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
            // lila_adas_mgr__sendpkt(CL_ADAS_PWR_ON, 0, NULL);

            adas_stat_send_evt = 0;

        }
    }

    return 0;
}

int lila_adas_mgr__init()
{
    mobileye_adas__mgr_init(ADAS_DEV_PATH, ADAS_DEV_BAUDRATE, p_adas_bmsg_proc);

    return 0;

}
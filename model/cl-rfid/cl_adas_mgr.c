#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/sender.h>
#include <base/devel.h>

#include "logd/logd_rpc.h"

#include "cl_mdt_pkt.h" 
#include "netcom.h"


// #define USE_MOVON_ADAS // temp code : for code highligh

#ifdef USE_MOVON_ADAS
#include <movon_adas.h>
#define LOG_TARGET eSVC_MODEL

#define ADAS_DEV_FAIL_CHK_CNT       10
#define ADAS_DEV_NON_EVT_CLR_CNT    30
#endif


#ifdef USE_MOBILEYE_ADAS
#include <mobileye_adas.h>
#define LOG_TARGET eSVC_MODEL

#define ADAS_DEV_FAIL_CHK_CNT     60
#define ADAS_DEV_NON_EVT_CLR_CNT    1
#endif

#if defined (SERVER_ABBR_CLRA0) || defined (SERVER_ABBR_CLRA1) || defined (SERVER_ABBR_CLRA9)
#define ADAS_DEV_PATH       "/dev/ttyHSL2"
#define ADAS_DEV_BAUDRATE   9600
#endif

#if defined (SERVER_ABBR_CLRB0) || defined (SERVER_ABBR_CLRB1) || defined (SERVER_ABBR_CLRB9)
#define ADAS_DEV_PATH       "/dev/ttyHSL2"
#define ADAS_DEV_BAUDRATE   9600
#endif


static int cl_adas_mgr_sendpkt(int evt_code, int speed, char* opt_data)
{
//    int pkt_type = -1;
    clAdasData_t adas_data;

    memset(&adas_data, 0x00, sizeof(adas_data));

    adas_data.event_code = evt_code;
    adas_data.adas_speed = speed;
    sprintf(adas_data.adas_opt_str, "%.6s", opt_data);

	sender_add_data_to_buffer(PACKET_TYPE_ADAS_EVENT, &adas_data, ePIPE_1);
    return 0;
}


#ifdef USE_MOVON_ADAS

int cl_adas_ttc_sendpkt(int evt_code)
{
    MOVON_DATA_FRAME_T cur_movon_data;
    char option_str[32] = {0,};

    memset(&cur_movon_data, 0x00, sizeof(MOVON_DATA_FRAME_T));
    movon_adas__get_cur_data(&cur_movon_data);

    sprintf(option_str, "%d" , cur_movon_data.ttc_sec);

    LOGI(LOG_TARGET, "[MOVON ADAS]  ttc_sec [%s]\r\n", option_str);

    cl_adas_mgr_sendpkt(evt_code, cur_movon_data.speed, option_str );

    return 0;
}
#endif

int p_adas_bmsg_proc(ADAS_EVT_DATA_T* evt_data)
{
    static int err_code_last = -1;

    static int read_fail_cnt = 0;
    static int adas_stat_send_evt = 0;

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
            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__FCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);

            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_2);
            cl_adas_mgr_sendpkt(CL_ADAS_FCW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__PCW:
        {
            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__PCW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_2);
            cl_adas_mgr_sendpkt(CL_ADAS_PCW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__LDW:
        {
            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__LDW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            option_str_len += sprintf(option_str + option_str_len, "%d," , evt_data->evt_data_2);
            if ( evt_data->evt_data_4 == 0 )
                option_str_len += sprintf(option_str + option_str_len, "L");
            else
                option_str_len += sprintf(option_str + option_str_len, "R");
            cl_adas_mgr_sendpkt(CL_ADAS_LDW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;
            
            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__HMW:
        {
            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__HMW) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_2);
            cl_adas_mgr_sendpkt(CL_ADAS_HMW_EVENT_CODE, evt_data->evt_data_1, option_str);

            err_code_last = -1;

            read_fail_cnt = 0;
            evt_none_cnt = 0;

            adas_evt_stat = 1;
            break;
        }
        case eADAS_EVT__SLI:
        {
            LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);

            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
            printf("p_bmsg_proc(eADAS_EVT__SLI) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);
        
            option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_4);
            cl_adas_mgr_sendpkt(CL_ADAS_SLI_EVENT_CODE, evt_data->evt_data_1, option_str);

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
                LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_code [%d] / [%d]\r\n", evt_data->evt_code, evt_data->evt_data_4);

                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_code [%d]\r\n", evt_data->evt_code);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_1 >> speed [%d]\r\n", evt_data->evt_data_1);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_2 >> break sig [%d]\r\n", evt_data->evt_data_2);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_3 >> ttc sec [%d]\r\n", evt_data->evt_data_3);
                printf("p_bmsg_proc(eADAS_EVT__ERR) :: evt_data->evt_data_4 >> ext data [%d]\r\n", evt_data->evt_data_4);

            #ifdef USE_MOVON_ADAS
                if ( evt_data->evt_data_4 == 0x01 )
                    option_str_len += sprintf(option_str + option_str_len, "%d" , 61);
                else if ( evt_data->evt_data_4 == 0x02 )
                    option_str_len += sprintf(option_str + option_str_len, "%d" , 62);
                else
                    option_str_len += sprintf(option_str + option_str_len, "%d" , 89);
            #endif

            #ifdef USE_MOBILEYE_ADAS
                option_str_len += sprintf(option_str + option_str_len, "%d" , evt_data->evt_data_4 );
            #endif

                cl_adas_mgr_sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);

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
                LOGI(LOG_TARGET, "p_bmsg_proc(eADAS_EVT__NONE) :: evt clear [%d]\r\n", evt_none_cnt);

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
    if ( read_fail_cnt > 0 )
    {
        printf("read_fail_cnt is [%d]\r\n", read_fail_cnt);
    }

    if ( read_fail_cnt > ADAS_DEV_FAIL_CHK_CNT)
    {
        if (( adas_stat_send_evt == 0 ) || ( adas_stat_send_evt == -1 ) )
        {
            devel_webdm_send_log("[ADAS] DEV DISCONN : CANNOT COMM");
            option_str_len += sprintf(option_str + option_str_len, "%d" , 89);
        #ifdef USE_MOVON_ADAS
            if ( g_stat_key_on == 1 )
                cl_adas_mgr_sendpkt(CL_ADAS_ERR_EVENT_CODE, evt_data->evt_data_1, option_str);
        #endif
            adas_stat_send_evt = 1;
            adas_evt_stat = 0; 
        }
    }

    if ( ((adas_stat_send_evt==1) && (adas_evt_stat == 1) ) || ((adas_stat_send_evt==-1) && (adas_evt_stat==1)))
    {
        devel_webdm_send_log("[ADAS] DEV CONN : SUCCESS");
        adas_stat_send_evt = 0;
    }

    return 0;
}


int cl_adas_mgr__init()
{
#if defined (USE_MOVON_ADAS)
    movon_adas__mgr_init(ADAS_DEV_PATH, ADAS_DEV_BAUDRATE, p_adas_bmsg_proc);
#endif

#if defined (USE_MOBILEYE_ADAS)
    mobileye_adas__mgr_init(ADAS_DEV_PATH, ADAS_DEV_BAUDRATE, p_adas_bmsg_proc);
#endif

    return 0;

}
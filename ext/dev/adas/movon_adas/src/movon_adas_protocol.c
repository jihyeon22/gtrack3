#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "movon_adas.h"
#include "movon_adas_mgr.h"
#include "movon_adas_protocol.h"
#include "movon_adas_tool.h"
#include "movon_adas_uart_util.h"

/*
int get_movon_data(char* input_data, MOVON_DATA_FRAME_T* movon_data)
{
    memcpy(movon_data, input_data, sizeof(MOVON_DATA_FRAME_T));
    movon_adas__set_cur_data(movon_data);
    //dbg_print_movon_data(movon_data);
    // TODO: data vaild check..
    return ADAS_EVT_RET_SUCCESS;
}
*/

int dbg_print_movon_data(MOVON_DATA_FRAME_T* movon_data)
{
    printf("movon data --------------------------------------\r\n");
	printf("  >> movon_data->stx -> [0x%x]\r\n",movon_data->stx);
	printf("  >> movon_data->type1 -> [0x%x]\r\n",movon_data->type1);
    printf("  >> movon_data->type2 -> [0x%x]\r\n",movon_data->type2);
    printf("  >> movon_data->data_len -> [0x%x]\r\n",movon_data->data_len);
    printf("  >> movon_data->speed -> [0x%x]\r\n",movon_data->speed);
    printf("  >> movon_data->left_turn_signal -> [0x%x]\r\n",movon_data->left_turn_signal);
    printf("  >> movon_data->right_turn_signal -> [0x%x]\r\n",movon_data->right_turn_signal);
    printf("  >> movon_data->break_signal -> [0x%x]\r\n",movon_data->break_signal);
    printf("  >> movon_data->char -> [0x%x]\r\n",movon_data->rpm);
    printf("  >> movon_data->LDW_left -> [0x%x]\r\n",movon_data->LDW_left);
    printf("  >> movon_data->LDW_right -> [0x%x]\r\n",movon_data->LDW_right);
    printf("  >> movon_data->left_distance -> [0x%x]\r\n",movon_data->left_distance);
    printf("  >> movon_data->right_distance -> [0x%x]\r\n",movon_data->right_distance);
    printf("  >> movon_data->ttc_sec -> [0x%x]\r\n",movon_data->ttc_sec);
    printf("  >> movon_data->sda -> [0x%x]\r\n",movon_data->sda);
    printf("  >> movon_data->fvsa -> [0x%x]\r\n",movon_data->fvsa);
    printf("  >> movon_data->fpw -> [0x%x]\r\n",movon_data->fpw);
    printf("  >> movon_data->fcw -> [0x%x]\r\n",movon_data->fcw);
    printf("  >> movon_data->pcw -> [0x%x]\r\n",movon_data->pcw);
    printf("  >> movon_data->recode -> [0x%x]\r\n",movon_data->recode);
    printf("  >> movon_data->errcode -> [0x%x]\r\n",movon_data->errcode);
    printf("  >> movon_data->chksum -> [0x%x]\r\n",movon_data->chksum);    
    printf("  >> movon_data->etx -> [0x%x]\r\n",movon_data->etx);
    printf("-------------------------------------------------------\r\n");
}  

int movon_get_evt_data(ADAS_EVT_DATA_T* evt_data, MOVON_DATA_FRAME_T* movon_data)
{
    int found_evt = 0;
//    eADAS_EVT__FCW, // FCW COMMAND : 전방추돌경보
//    eADAS_EVT__PCW, // PCW COMMAND : 보행자 충돌 경보
//    eADAS_EVT__LDW, // LDW COMMAND : 차선이탈
//    eADAS_EVT__HMW, // HMW COMMAND : 차간거리 경보
//    eADAS_EVT__SLI, // SLI COMMAND : 속도제한
//    eADAS_EVT__ERR, // ERR COMMAND : ERR

    evt_data->evt_data_1 = movon_data->speed;
    evt_data->evt_data_2 = movon_data->break_signal;
    evt_data->evt_data_3 = movon_data->ttc_sec;
    
    //dbg_print_movon_data(movon_data);
    
    if ( movon_data->fcw == 0x02 ) 
    {
        if ( movon_data->speed < 30)
            evt_data->evt_code = eADAS_EVT__UFCW;
        else
            evt_data->evt_code = eADAS_EVT__FCW;

        //dbg_print_movon_data(movon_data);
        movon_data->fcw = 0;
    }
    else if ( movon_data->LDW_left == 0x02 )
    {
        evt_data->evt_code = eADAS_EVT__LDW;
        evt_data->evt_data_4 = 0;   // left 
        //dbg_print_movon_data(movon_data);
        movon_data->LDW_left = 0;
    }
    else if ( movon_data->LDW_right == 0x02 )
    {
        evt_data->evt_code = eADAS_EVT__LDW;
        evt_data->evt_data_4 = 1;   // right
        //dbg_print_movon_data(movon_data);
        movon_data->LDW_right = 0;
     }
    else if ( movon_data->pcw == 0x02 ) 
    {
        evt_data->evt_code = eADAS_EVT__PCW;
        //dbg_print_movon_data(movon_data);
        movon_data->pcw = 0;
    }
    else if ( movon_data->fpw == 0x02 ) 
    {
        evt_data->evt_code = eADAS_EVT__FPW;
        //dbg_print_movon_data(movon_data);
        movon_data->fpw = 0;
    }
    else if ( movon_data->errcode != 0x00 ) 
    {
        evt_data->evt_code = eADAS_EVT__ERR;
        evt_data->evt_data_4 = movon_data->errcode;
        dbg_print_movon_data(movon_data);
        movon_data->errcode = 0;
    }
    else
    {
        evt_data->evt_code = eADAS_EVT__NONE;
    }


    return ADAS_EVT_RET_SUCCESS;
}

int movon_adas__cmd_broadcast_msg_start()
{
    return MOVON_ADAS_RET_SUCCESS;
}
int movon_adas__cmd_broadcast_msg_stop()
{
    return MOVON_ADAS_RET_SUCCESS;
}


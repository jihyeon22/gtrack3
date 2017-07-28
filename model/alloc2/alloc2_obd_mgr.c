#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <base/devel.h>
#include <base/sender.h>
#include <base/sender.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <logd_rpc.h>

#include <netcom.h>
#include "alloc2_pkt.h"

#include "alloc2_senario.h"
#include "seco_obd_1.h"
#include "alloc2_obd_mgr.h"

#define OBD_CMD_RETRY_CNT   2

static SECO_OBD_INFO_T g_seco_obd_info = {0,};
static SECO_OBD_DATA_T g_cur_seco_obd_data = {0,};
static SECO_OBD_RUN_CMD_MGR_T g_obd_run_mgr;

static pthread_mutex_t obd_data_mutex = PTHREAD_MUTEX_INITIALIZER;


int alloc2_obd_mgr__init()
{
    int i = 0;
    char tmp_buff[512] = {0,};
    //int ret = 0;
    // 먼저 시리얼들을 갖고온다.

    memset(&g_obd_run_mgr, 0x00, sizeof(g_obd_run_mgr));

    for ( i = 0 ; i < OBD_CMD_RETRY_CNT ; i++ )
    {
        memset(&tmp_buff, 0x00, 512);
        if ( get_seco_obd_1_serial(tmp_buff) == OBD_RET_SUCCESS )
        {
            printf("[ALLOC2 SENARIO] get obd sn : [%s]\r\n", tmp_buff);
            strcpy(g_seco_obd_info.obd_sn, tmp_buff);
            break;
        }
        printf("[ALLOC2 SENARIO] get obd sn fail [%d]\r\n",i);
        sleep(1);
    }

    if ( i >= OBD_CMD_RETRY_CNT )
        return -1;

    for ( i = 0 ; i < OBD_CMD_RETRY_CNT ; i++ )
    {
        memset(&tmp_buff, 0x00, 512);
        if ( get_seco_obd_1_ver(tmp_buff) == OBD_RET_SUCCESS )
        {
            printf("[ALLOC2 SENARIO] get obd ver : [%s]\r\n", tmp_buff);
            strcpy(g_seco_obd_info.obd_swver, tmp_buff);
            break;
        }
        printf("[ALLOC2 SENARIO] get obd ver fail [%d]\r\n",i);
        sleep(1);
    }

    if ( i >= OBD_CMD_RETRY_CNT )
        return -1;

    for ( i = 0 ; i < OBD_CMD_RETRY_CNT ; i++ )
    {
        memset(&tmp_buff, 0x00, 512);
        if ( get_seco_obd_1_fueltype(tmp_buff) == OBD_RET_SUCCESS )
        {
            printf("[ALLOC2 SENARIO] get obd fueltype : [%s]\r\n", tmp_buff);
            strcpy(g_seco_obd_info.fuel_type, tmp_buff);
            break;
        }
        printf("[ALLOC2 SENARIO] get obd fueltype fail [%d]\r\n",i);
        sleep(1);
    }

    if ( i >= OBD_CMD_RETRY_CNT )
        return -1;

    g_seco_obd_info.obd_stat = 1;

    alloc2_obd_mgr__obd_broadcast_start();
    
    for ( i = 0 ; i < SECO_OBD_CMD_TOTAL_CNT ; i ++ )
		alloc2_obd_mgr__clr_cmd_proc(i);

    return 0;
}

int alloc2_obd_mgr__get_obd_dev_info(SECO_OBD_INFO_T* obd_info)
{
    memcpy(obd_info, &g_seco_obd_info, sizeof(SECO_OBD_INFO_T));
    return 0;
}
int alloc2_obd_mgr__obd_broadcast_start()
{
	start_seco_obd_1_broadcast_msg(1, "FLI,RPM,COT,BAT,SPD,TDD,FBK,ODD");
    return 0;
}

int alloc2_obd_mgr__obd_broadcast_stop()
{
    stop_seco_obd_1_broadcast_msg();
    return 0;
}



int alloc2_obd_mgr__get_cur_obd_data(SECO_OBD_DATA_T* cur_obd_info)
{
    pthread_mutex_lock(&obd_data_mutex);
    memcpy(cur_obd_info, &g_cur_seco_obd_data, sizeof(SECO_OBD_DATA_T));
    pthread_mutex_unlock(&obd_data_mutex);
    return 0;
}

int alloc2_obd_mgr__chk_fail_proc()
{
    devel_webdm_send_log("[OBD MGR] chk fail\r\n");
    ALLOC_PKT_SEND__OBD_STAT_ARG obd_stat_arg;
    memset(&obd_stat_arg, 0x00, sizeof(ALLOC_PKT_SEND__OBD_STAT_ARG));

    obd_stat_arg.obd_stat_flag = 1;
    obd_stat_arg.obd_stat = 0;
    obd_stat_arg.obd_remain_fuel_stat = 0;
    obd_stat_arg.obd_evt_code = 0;;
    obd_stat_arg.obd_fuel_type = 0;;
    obd_stat_arg.obd_remain_fuel = 0;;

    pthread_mutex_lock(&obd_data_mutex);
    memset(&g_cur_seco_obd_data, 0x00, sizeof(SECO_OBD_DATA_T));
    pthread_mutex_unlock(&obd_data_mutex);

    sender_add_data_to_buffer(e_obd_stat, &obd_stat_arg, get_pkt_pipe_type(e_obd_stat,0));

    init_seco_obd_mgr("/dev/ttyHSL1", 115200, alloc2_obd_mgr__obd_broadcast_proc);
    alloc2_obd_mgr__init();

    return 0;
}

int alloc2_obd_mgr__obd_broadcast_proc(const int argc, const char* argv[])
{
    static int fail_cnt = 0;

    SECO_OBD_DATA_T tmp_cur_seco_obd_data = {0,};
    static unsigned int valid_fuel_remain = 0;

    if ( ( argc == 0) || ( argc != 8 ))
    {
        LOGE(eSVC_MODEL, "[OBD MGR] chk fail [%d]/[%d]\r\n", fail_cnt, MAX_FAIL_CNT_CHK);
        fail_cnt++;
    }
    else
    {
        fail_cnt=0;
    }


    if ( fail_cnt > MAX_FAIL_CNT_CHK )
    {
        fail_cnt = 0;
        alloc2_obd_mgr__chk_fail_proc();
    }

//    int i = 0;
    memset(&tmp_cur_seco_obd_data, 0x00, sizeof(SECO_OBD_DATA_T));
    //printf("alloc2_obd_mgr__obd_broadcast_proc start ======================================\r\n");

/*
broadcast value : [0]/[7] => [50]
broadcast value : [1]/[7] => [10214]
broadcast value : [2]/[7] => [14.1]
broadcast value : [3]/[7] => [125]
broadcast value : [4]/[7] => [3364292]
broadcast value : [5]/[7] => [X]
broadcast value : [6]/[7] => [X]
*/
/*
    for( i = 0 ; i < argc ; i++)
    {   
        printf("broadcast value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
*/
    if ( argc != 8 )
    {
        printf("broadcast value return fail : invalid argument [%d]\r\n",argc);
        return -1;
    }

    // 0 : FLI : 연료잔량
    {
        int cur_fuel_remain = atoi(argv[0]);

        if ( cur_fuel_remain > 0 )
            valid_fuel_remain = cur_fuel_remain;
            
        tmp_cur_seco_obd_data.obd_data_fuel_remain = valid_fuel_remain;


        //printf("tmp_cur_seco_obd_data.obd_data_fuel_remain is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_fuel_remain);
    }
    // 1 : RPM : RPM
    {
        tmp_cur_seco_obd_data.obd_data_rpm = atoi(argv[1]);
        //printf("tmp_cur_seco_obd_data.obd_data_rpm is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_rpm);
    }
    // 2 : COT : 냉각수온도 xx.x 
    {
        float tmp_val = atof(argv[2]);
        tmp_val = tmp_val * 10;
        tmp_cur_seco_obd_data.obd_data_cot = (unsigned int)tmp_val;
        //printf("tmp_cur_seco_obd_data.obd_data_cot is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_cot);
    }
    // 3 : BAT : 배터리전압 
    {
        tmp_cur_seco_obd_data.obd_data_car_volt = (int)(atof(argv[3])*10);
        //printf("tmp_cur_seco_obd_data.obd_data_car_volt is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_car_volt);
    }
    // 4 : SPD : 현재속도
    {
        tmp_cur_seco_obd_data.obd_data_car_speed = atoi(argv[4]);
        //printf("tmp_cur_seco_obd_data.obd_data_car_speed is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_car_speed);
    }
    // 5 : TDD : 총누적거리
    {
        float tmp_val = atof(argv[5]);
        tmp_cur_seco_obd_data.obd_data_total_distance =  (unsigned int)tmp_val;
        //printf("tmp_cur_seco_obd_data.obd_data_total_distance is [%s] / [%d]\r\n",argv[5], tmp_cur_seco_obd_data.obd_data_total_distance);
    }
    // 6 : FBK : 브레이크상태
    {
        tmp_cur_seco_obd_data.obd_data_break_signal = atoi(argv[6]);
        //printf("tmp_cur_seco_obd_data.obd_data_break_signal is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_break_signal);
    }
    // 7 : ODD : 계기판누적거리
    {
        tmp_cur_seco_obd_data.obd_data_panel_distance = atoi(argv[7]);
        //printf("tmp_cur_seco_obd_data.obd_data_panel_distance is [%d]\r\n",tmp_cur_seco_obd_data.obd_data_panel_distance);
    }

    pthread_mutex_lock(&obd_data_mutex);
    memcpy(&g_cur_seco_obd_data, &tmp_cur_seco_obd_data, sizeof(SECO_OBD_DATA_T));
    pthread_mutex_unlock(&obd_data_mutex);
    

    //printf("alloc2_obd_mgr__obd_broadcast_proc end ======================================\r\n");
    return 0;
}



int alloc2_obd_mgr__set_cmd_proc(int cmd_id, char* cmd_arg)
{
    g_obd_run_mgr.cmd_type[cmd_id] = SECO_OBD_CMD_RUN;
    strcpy( g_obd_run_mgr.cmd_arg[cmd_id], cmd_arg);

    return 0;
}

int alloc2_obd_mgr__clr_cmd_proc(int cmd_id)
{
    g_obd_run_mgr.cmd_type[cmd_id] = SECO_OBD_CMD_NOT_RUN;
    memset(g_obd_run_mgr.cmd_arg[cmd_id], 0x00, SECO_OBD_CMD_ARG_LEN);
    g_obd_run_mgr.cmd_result[cmd_id] = SECO_OBD_CMD_RET__NONE;

    return 0;
}


int alloc2_obd_mgr__get_cmd_proc_result(int cmd_id)
{
    int ret = g_obd_run_mgr.cmd_result[cmd_id];
    //alloc2_obd_mgr__clr_cmd_proc(cmd_id);
    return ret;
}

int alloc2_obd_mgr__run_cmd_proc()
{
    int i = 0;
    
    for ( i = 0 ; i < SECO_OBD_CMD_TOTAL_CNT ; i ++ )
    {
        if ( g_obd_run_mgr.cmd_type[i] == SECO_OBD_CMD_RUN )
            break;
    }

    switch(i)
    {
        case SECO_OBD_CMD_TYPE__SET_DISTANCE:
        {
  			

            alloc2_obd_mgr__obd_broadcast_stop();
            if ( set_seco_obd_1_total_distance(atoi(g_obd_run_mgr.cmd_arg[i])) == OBD_RET_SUCCESS )
            {
                g_obd_run_mgr.cmd_result[i] = SECO_OBD_CMD_RET__SUCCESS;
                LOGI(eSVC_MODEL, "[ALLOC2 CMD PROC] SECO_OBD_CMD_TYPE__SET_DISTANCE - DISTANCE [%d] - SUCCESS \r\n", atoi(g_obd_run_mgr.cmd_arg[i]));
            }
            else
            {
                g_obd_run_mgr.cmd_result[i] = SECO_OBD_CMD_RET__FAIL;
                LOGI(eSVC_MODEL, "[ALLOC2 CMD PROC] SECO_OBD_CMD_TYPE__SET_DISTANCE - DISTANCE [%d] - FAIL \r\n", atoi(g_obd_run_mgr.cmd_arg[i]));
            }

            alloc2_obd_mgr__obd_broadcast_start();

            g_obd_run_mgr.cmd_type[i] = SECO_OBD_CMD_NOT_RUN; // 처리했으니 클리어
            
            break;
        }
        default :
            break;

    }

    return 0;
}


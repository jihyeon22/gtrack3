
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <base/sender.h>
#include <base/watchdog.h>
#include <board/uart.h>

#include "seco_obd_1.h"
#include <pthread.h>

#include "seco_obd_mgr.h"
#include "katech-data-calc.h"

static pthread_mutex_t katech_obd_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static SECO_CMD_DATA_SRR_TA1_T _g_obd_ta1 = {0,};
static SECO_CMD_DATA_SRR_TA2_T _g_obd_ta2 = {0,};

static int _g_run_katech_obd_thread = 0;
static int _g_get_ta1_obd_info_ret = 0;
static int _g_get_ta2_obd_info_ret = 0;

int katech_obd_mgr__get_ta1_obd_info(SECO_CMD_DATA_SRR_TA1_T* ta1_buff)
{   
    pthread_mutex_lock(&katech_obd_data_mutex);
	memcpy ( ta1_buff, &_g_obd_ta1, sizeof(_g_obd_ta1) );
    pthread_mutex_unlock(&katech_obd_data_mutex);
    return _g_get_ta1_obd_info_ret;
}

int katech_obd_mgr__set_ta1_obd_info(SECO_CMD_DATA_SRR_TA1_T ta1_buff)
{
    // filter invalid data.
    if( ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_MAP].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_RPM].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_SPD].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_BS1].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_BAV].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_EFR].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_AED].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_COT].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_EGR].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_EGE].data == 0 ) &&
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_BRO].data == 0 ) )
    {
        printf("[OBD MGR] ERR not valid ta1 data\r\n");
        return 0;
    }

    // FIX : 180418 filer add
    if( ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_RPM].data >= 16383 ) ||
        ( ta1_buff.obd_data[eOBD_CMD_SRR_TA1_SPD].data >= 255 ))
    {
        printf("[OBD MGR] ERR not valid ta1 data\r\n");
        return 0;
    }

    pthread_mutex_lock(&katech_obd_data_mutex);
    memcpy ( &_g_obd_ta1, &ta1_buff, sizeof(_g_obd_ta1) );
    _g_get_ta1_obd_info_ret = 1;
    pthread_mutex_unlock(&katech_obd_data_mutex);
    return 0;
}

int katech_obd_mgr__get_ta2_obd_info(SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    pthread_mutex_lock(&katech_obd_data_mutex);
    memcpy ( ta2_buff, &_g_obd_ta2, sizeof(_g_obd_ta2) );
    pthread_mutex_unlock(&katech_obd_data_mutex);
    return _g_get_ta2_obd_info_ret;
}

int katech_obd_mgr__set_ta2_obd_info(SECO_CMD_DATA_SRR_TA2_T ta2_buff)
{
    pthread_mutex_lock(&katech_obd_data_mutex);
    memcpy ( &_g_obd_ta2, &ta2_buff, sizeof(_g_obd_ta2) );
    _g_get_ta2_obd_info_ret = 1;
    pthread_mutex_unlock(&katech_obd_data_mutex);
    return 0;
}

static int _g_ta1_interval = KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC;
int katech_obd_mgr__set_ta1_interval_sec(int sec)
{
    _g_ta1_interval = sec;
    return _g_ta1_interval;
}

static int _g_ta2_interval = KATECH_OBD_TA2_INTERVAL_DEFAULT_SEC;
int katech_obd_mgr__set_ta2_interval_sec(int sec)
{
    _g_ta2_interval = sec;
    return _g_ta2_interval;
}

void thread_katech_obd(void)
{
    SECO_CMD_DATA_SRR_TA1_T ta1_buff_cur;
    SECO_CMD_DATA_SRR_TA2_T ta2_buff_cur;

    int time_cnt = 0;

    int get_ta1_cnt = KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC+1;
    int get_ta2_cnt = KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC+1;

    _g_ta1_interval = KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC;
    _g_ta2_interval = KATECH_OBD_TA2_INTERVAL_DEFAULT_SEC;

    stop_seco_obd_1_broadcast_msg();
    _g_run_katech_obd_thread = 1;

    while(_g_run_katech_obd_thread)
    {
        memset(&ta1_buff_cur, 0x00, sizeof(SECO_CMD_DATA_SRR_TA1_T));
        memset(&ta2_buff_cur, 0x00, sizeof(SECO_CMD_DATA_SRR_TA2_T));

        // -------------------------------------------------------
        // GET TA1 DATA 
        // -------------------------------------------------------
        if ( get_ta1_cnt > _g_ta1_interval )
        {
            if ( get_seco_obd_cmd_ta1(&ta1_buff_cur) == OBD_RET_SUCCESS )
            {
                katech_obd_mgr__set_ta1_obd_info(ta1_buff_cur);
                get_ta1_cnt = 0; // 성공일때만 다음 interval.., fail 나면 매초시도
            }
        }


        if ( get_ta2_cnt > _g_ta2_interval )
        {
            if ( get_seco_obd_cmd_ta2(&ta2_buff_cur) == OBD_RET_SUCCESS)
            {
                katech_obd_mgr__set_ta2_obd_info(ta2_buff_cur);
                get_ta2_cnt = 0; // 성공일때만 다음 interval.., fail 나면 매초시도
            }
            
        }

        get_ta1_cnt++;
        get_ta2_cnt++;

        time_cnt++;
        sleep(1);
    }

}

void exit_thread_katech_obd(void)
{
	_g_run_katech_obd_thread = 0;
}



int katech_obd_mgr__timeserise_calc_init()
{
    timeserise_calc__init();
    return 0;
}
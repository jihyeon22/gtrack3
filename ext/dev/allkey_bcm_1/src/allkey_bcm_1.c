<<<<<<< HEAD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>


#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "allkey_bcm_1.h"
#include "allkey_bcm_1_internal.h"

pthread_t tid_allkey_bcm_1_thread;


static int (*g_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list) = NULL;


static pthread_mutex_t allkey_bcm_1_mutex = PTHREAD_MUTEX_INITIALIZER;
int _g_run_allkey_bcm_1_thread_run;

static int _bcm_fd = ALLKEY_BCM_INVAILD_FD;

static int _allkey_bcm_devinit()
{
    int max_chk_cnt = ALLKEY_BCM_MAX_CHK_DEV_CNT;
    int ret_val = ALLKEY_BCM_RET_FAIL;

    while(max_chk_cnt--)
    {
        if ( _bcm_fd == ALLKEY_BCM_INVAILD_FD )
            _bcm_fd = mds_api_init_uart(ALLKEY_BCM_UART_DEVNAME, ALLKEY_BCM_UART_BAUDRATE);
        
        if ( _bcm_fd > 0 ) 
        {
            ret_val = ALLKEY_BCM_RET_SUCCESS;
            break;
        }
        else
        {
            ret_val = ALLKEY_BCM_RET_FAIL;
            _bcm_fd = ALLKEY_BCM_INVAILD_FD;
        }
    }
    return ret_val;
}

static int _allkey_bcm_cmd(int auto_lock, unsigned char cmd_type, unsigned char cmd_data, unsigned char recv_buff[8])
{
    int uart_ret = 0;
    int ret_val = 0;

    unsigned char allkey_bcm_send_cmd[4] = {0,};
    unsigned char allkey_bcm_recv_data[8] = {0,};
    
    int read_len = 0;
    int to_read = sizeof(allkey_bcm_recv_data);
    int read_retry_cnt = ALLKEY_BCM_UART_READ_RETRY_CNT;
    
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex lock\r\n");
        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);
    }

    if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    // make command.
    allkey_bcm_send_cmd[0] = 0x02;
    allkey_bcm_send_cmd[1] = cmd_type;
    allkey_bcm_send_cmd[2] = cmd_data;
    allkey_bcm_send_cmd[3] = 0xff;

    // do not ret chk
    mds_api_uart_write(_bcm_fd, allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
	
    while(read_retry_cnt--)
    {
	    uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data + read_len,  sizeof(allkey_bcm_recv_data) - read_len, ALLKEY_BCM_UART_READ_TIMEOUT);
        if ( uart_ret <= 0 )
            continue;

        read_len += uart_ret;

        if ( read_len >= to_read )
            break;
    }


    if (read_len != to_read)
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    if ( allkey_bcm_recv_data[0] != 0x02 )
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[1] != cmd_type)
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[2] != 0x41 ) // ack : 'A'
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

//    allkey_bcm_recv_data[3]; // data len??

//    allkey_bcm_recv_data[4]; // stat1
//    allkey_bcm_recv_data[5]; // stat2
//    allkey_bcm_recv_data[6]; // error list
//    allkey_bcm_recv_data[7]; // endof frame

    memcpy(recv_buff, allkey_bcm_recv_data, sizeof(allkey_bcm_recv_data));
    ret_val = ALLKEY_BCM_RET_SUCCESS;

FINISH:
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
    }

    return ret_val;
}

static int _allkey_bcm_cmd2(int auto_lock, unsigned char cmd_type, unsigned char cmd_data, unsigned char ext_data[4], unsigned char recv_buff[9])
{
    int uart_ret = 0;
    int ret_val = 0;

    unsigned char allkey_bcm_send_cmd[9] = {0,};
    //unsigned char allkey_bcm_recv_data[9] = {0,};
    
    int read_len = 0;
    //int to_read = sizeof(allkey_bcm_recv_data);
    int read_retry_cnt = 0;
    
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex lock\r\n");
        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);
    }

    if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    // make command.
    allkey_bcm_send_cmd[0] = 0x02;
    allkey_bcm_send_cmd[1] = cmd_type; // CMD List 참조    
    allkey_bcm_send_cmd[2] = cmd_data; // DATA List 참조
    allkey_bcm_send_cmd[3] = 0x04; // DATA Length (0x04)
    allkey_bcm_send_cmd[4] = ext_data[0]; // data1 
    allkey_bcm_send_cmd[5] = ext_data[1]; // data2
    allkey_bcm_send_cmd[6] = ext_data[2]; // data3
    allkey_bcm_send_cmd[7] = ext_data[3]; // data4
    allkey_bcm_send_cmd[8] = 0xff;

    //printf("allkey bcm cmd 2 api write ------------------------------ ++ [%d] \r\n", sizeof(allkey_bcm_send_cmd));
    //mds_api_debug_hexdump_buff(allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
    //printf("allkey bcm cmd 2 api write ------------------------------ -- [%d] \r\n", sizeof(allkey_bcm_send_cmd));

    // do not ret chk
    uart_ret = mds_api_uart_write(_bcm_fd, allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
    
    /*
    while(read_retry_cnt--)
    {
        uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data + read_len,  sizeof(allkey_bcm_recv_data) - read_len, ALLKEY_BCM_UART_READ_TIMEOUT);

        if ( uart_ret <= 0 )
            continue;

        read_len += uart_ret;

        if ( read_len >= to_read )
            break;
    }

    printf("allkey bcm cmd 2 api read ------------------------------ ++ [%d] \r\n", read_len);
    mds_api_debug_hexdump_buff(allkey_bcm_recv_data, read_len);
    printf("allkey bcm cmd 2 api read ------------------------------ -- [%d] \r\n", read_len);


    if (read_len != to_read)
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[0] != 0x02 )
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[1] != cmd_type)
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[2] != 0x41 ) // ack : 'A'
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

//    allkey_bcm_recv_data[3]; // data len??

//    allkey_bcm_recv_data[4]; // stat1
//    allkey_bcm_recv_data[5]; // stat2
//    allkey_bcm_recv_data[6]; // error list
//    allkey_bcm_recv_data[7]; // endof frame

    memcpy(recv_buff, allkey_bcm_recv_data, sizeof(allkey_bcm_recv_data));
    
*/
    if ( uart_ret > 0 )
        ret_val = ALLKEY_BCM_RET_SUCCESS;
    else
        ret_val = ALLKEY_BCM_RET_FAIL;
FINISH:
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
    }

    return ret_val;
}


int allkey_bcm_cmd__get_stat(ALLKEY_BCM_1_DEV_T* dev_stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_req_stat;
    send_data = 'O';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    // printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        printf("bcm recv : ");
        for ( i = 0 ; i < 8 ; i ++ )
            printf("[0x%02x]", recv_buff[i]);
        printf("\r\n");
    }

    // bcm recv : [0x02][0x53][0x41][0x03][0x02][0x20][0x00][0xff]
    if ( ( recv_buff[0] == 0x02 ) && ( recv_buff[1] == 0x53) )
    {
        dev_stat->door_open_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x01); 
        dev_stat->door_lock_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x02); 
        dev_stat->engine_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x04); 
        dev_stat->remote_start_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x08); 
        dev_stat->hood_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x10); 
        dev_stat->turbo_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x20); 
        dev_stat->timer_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x40); 
        dev_stat->mute_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x80); 

        dev_stat->shock_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x01);
        dev_stat->acc_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x02);
        dev_stat->trunk_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x04);
        dev_stat->always_evt_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x08);
        dev_stat->ig_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x10);
        dev_stat->theif_sensor_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x20);
        dev_stat->valet_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x40);
        dev_stat->alert_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x80);
/*
        printf("dev_stat->door_open_stat is [%d]\r\n", dev_stat->door_open_stat);
        printf("dev_stat->door_lock_stat is [%d]\r\n", dev_stat->door_lock_stat);
        printf("dev_stat->engine_stat is [%d]\r\n", dev_stat->engine_stat);
        printf("dev_stat->remote_start_stat is [%d]\r\n", dev_stat->remote_start_stat);
        printf("dev_stat->hood_stat is [%d]\r\n", dev_stat->hood_stat);
        printf("dev_stat->turbo_stat is [%d]\r\n", dev_stat->turbo_stat);
        printf("dev_stat->timer_stat is [%d]\r\n", dev_stat->timer_stat);
        printf("dev_stat->mute_stat is [%d]\r\n", dev_stat->mute_stat);

        printf("dev_stat->shock_stat is [%d]\r\n", dev_stat->shock_stat);
        printf("dev_stat->acc_stat is [%d]\r\n", dev_stat->acc_stat);
        printf("dev_stat->trunk_stat is [%d]\r\n", dev_stat->trunk_stat);
        printf("dev_stat->always_evt_stat is [%d]\r\n", dev_stat->always_evt_stat);
        printf("dev_stat->ig_stat is [%d]\r\n", dev_stat->ig_stat);
        printf("dev_stat->theif_sensor_stat is [%d]\r\n", dev_stat->theif_sensor_stat);
        printf("dev_stat->valet_stat is [%d]\r\n", dev_stat->valet_stat);
        printf("dev_stat->alert_stat is [%d]\r\n", dev_stat->alert_stat);
*/
    }


    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__door_lock(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_door_ctr;
    if ( stat )
        send_data = 'L';
    else
        send_data = 'U';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    if (0)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__door_evt_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_door_evt_setting;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__horn_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_horn_ctr;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;

}


int allkey_bcm_ctr__light_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_twingkle_light_ctr;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__theft_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_twingkle_light_ctr;
    if ( stat )
        send_data = 'F';
    else
        send_data = 'O';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


static int _allkey_bcm_ctr__knocksensor_set_id(int use_mutex_lock, unsigned short id)
{
    // unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];

    unsigned short arg_tmp = id;

    unsigned short arg_tmp_1 = 0;
    unsigned short arg_tmp_2 = 0;

    unsigned char tmp_digit[4] = {0,};
    int i = 0;
    
    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_id;

    for( i = 0 ; i < 4 ; i ++)
    {
        tmp_digit[3-i] = arg_tmp % 10 ;
        arg_tmp = arg_tmp / 10;
    }

    arg_tmp = 0;
    
    arg_tmp_1 += tmp_digit[0] * 0x10;
    arg_tmp_1 += tmp_digit[1] * 0x1;
    arg_tmp_2 += tmp_digit[2] * 0x10;
    arg_tmp_2 += tmp_digit[3] * 0x1;

    printf("arg_tmp_1 [0x%x], arg_tmp_2 [0x%x]\r\n", arg_tmp_1, arg_tmp_2);
    /*
    for( i = 0 ; i < 4 ; i ++)
    {
        arg_tmp += tmp_digit[i] * (16^(3-i));
        printf("arg_tmp [0x%x] / tmp_digit[%d]=[0x%x] / [0x%x]\r\n", arg_tmp, i, tmp_digit[i], (16^(3-i)));
    }
    */

    ext_data[0] = arg_tmp_1;
    ext_data[1] = arg_tmp_2;
    ext_data[2] = 0;
    ext_data[3] = 0;

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, id, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }
    
    printf("%s() success\r\n", __func__);


    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__knocksensor_set_id(unsigned short id)
{
    return _allkey_bcm_ctr__knocksensor_set_id(USE_MUTEX_LOCK, id);
}

int allkey_bcm_ctr__knocksensor_set_id_evt_proc(unsigned short id)
{
    return _allkey_bcm_ctr__knocksensor_set_id(NOT_USE_MUTEX_LOCK, id);
}


int _allkey_bcm_ctr__knocksensor_set_passwd(int use_mutex_lock, unsigned short passwd)
{
   //unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];

    unsigned short arg_tmp = passwd;
    
    unsigned short arg_tmp_1 = 0;
    unsigned short arg_tmp_2 = 0;

    unsigned char tmp_digit[4] = {0,};
    int i = 0;

    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_passwd;

    for( i = 0 ; i < 4 ; i ++)
    {
        tmp_digit[3-i] = arg_tmp % 10 ;
        arg_tmp = arg_tmp / 10;
    }

    arg_tmp = 0;
      
    arg_tmp_1 += tmp_digit[0] * 0x10;
    arg_tmp_1 += tmp_digit[1] * 0x1;
    arg_tmp_2 += tmp_digit[2] * 0x10;
    arg_tmp_2 += tmp_digit[3] * 0x1;

    printf("arg_tmp_1 [0x%x], arg_tmp_2 [0x%x]\r\n", arg_tmp_1, arg_tmp_2);
    /*
    for( i = 0 ; i < 4 ; i ++)
    {
        arg_tmp += tmp_digit[i] * (16^(3-i));
        printf("arg_tmp [0x%x] / tmp_digit[%d]=[0x%x] / [0x%x]\r\n", arg_tmp, i, tmp_digit[i], (16^(3-i)));
    }
    */

    ext_data[0] = arg_tmp_1;
    ext_data[1] = arg_tmp_2;
    ext_data[2] = 0;
    ext_data[3] = 0;

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, passwd, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }

    printf("%s() success\r\n", __func__);

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__knocksensor_set_passwd(unsigned short passwd)
{
    _allkey_bcm_ctr__knocksensor_set_passwd(USE_MUTEX_LOCK, passwd);
}

int allkey_bcm_ctr__knocksensor_set_passwd_evt_proc(unsigned short passwd)
{
    _allkey_bcm_ctr__knocksensor_set_passwd(NOT_USE_MUTEX_LOCK, passwd);
}


/*
Unix time을 4byte 로 보내주시면 됩니다.
현재 유닉스 타임이 1234567890 이라면 4996 02D2 이고 위 표에 timestamp1 = 49, timestamp2=96, timestamp3=02, timestamp4=D2 가 됩니다.
02 52 54 04 49 96 02 D2 FF
*/

static int _allkey_bcm_ctr__knocksensor_set_modemtime(int use_mutex_lock)
{
    // unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];
    unsigned char ext_data_tmp[4];

    int modemtime_utc =  get_modem_time_utc_sec(); // 1234567890;

    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_timedate;
    
    memcpy(&ext_data_tmp, &modemtime_utc, 4);

    ext_data[3] = ext_data_tmp[0];
    ext_data[2] = ext_data_tmp[1];
    ext_data[1] = ext_data_tmp[2];
    ext_data[0] = ext_data_tmp[3];

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, modemtime_utc, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }

    printf("%s() success\r\n", __func__);

    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__knocksensor_set_modemtime_evt_proc()
{
    // EVENT PROC 에서는 이미 MUTEX LOCK 상태에서 들어온다.
    // 때문에 해당 커맨드에서 다시 MUTEXT LOCK 하면 HANG 에 빠짐, MUTEX LOCK 하지 말고 사용
    return _allkey_bcm_ctr__knocksensor_set_modemtime(NOT_USE_MUTEX_LOCK);
}

int allkey_bcm_ctr__knocksensor_set_modemtime()
{
    return _allkey_bcm_ctr__knocksensor_set_modemtime(USE_MUTEX_LOCK);
}



// ...


int allkey_evt_proc(const unsigned char buff[8])
{
    int ret_val = ALLKEY_BCM_RET_FAIL;
    int evt_code = e_bcm_evt_monitor_none;

    static int last_event = e_bcm_evt_monitor_none;
    int chk_double_evt = 0;


    if (( buff[0] != 0x02 ) || ( buff[7] != 0xff ) )
        return ALLKEY_BCM_RET_FAIL;

    switch (buff[1])
    {
        case 0x63: 
        {
            if ( buff[2] == 0x6F) // 차주호출
                evt_code = e_bcm_evt_driver_call;
            break;
        }
        case 0x64:
        {
            if ( buff[2] == 0x6c ) // 경계 해제시 도어잠김
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt recv : door close", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                evt_code = e_bcm_evt_monitor_off_door_open;
                chk_double_evt = 1;
            }
            else if ( buff[2] == 0x75 ) // 경계 해제시 도어열림
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt recv : door open", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                evt_code = e_bcm_evt_monitor_off_door_close;
                chk_double_evt = 1;
            }
            break;
        }
        case 0x6b:
        {
            if ( buff[2] == 0x77 ) // 약한 충격 감지
                evt_code = e_bcm_evt_small_shock;
            else if ( buff[2] == 0x73 ) // 강한 충격 감지
                evt_code = e_bcm_evt_big_shock;
            break;
        }
        case 0x74:
        {
            if ( buff[2] == 0x6c ) // 경계 해제 시 트렁크 닫힘
            {
                evt_code = e_bcm_evt_monitor_off_trunk_open;
                chk_double_evt = 1;
            }
            else if ( buff[2] == 0x75 ) // 경계해제 시 트렁크 열림
            {
                evt_code = e_bcm_evt_monitor_off_trunk_close;
                chk_double_evt = 1;
            }
            break;
        }
        case 0x76:
        {
            if ( buff[2] == 0x64 ) // 침입감지(도어)
                evt_code = e_bcm_evt_monitor_on_door_stat;
            else if ( buff[2] == 0x74 ) // 침입감지(트렁크)
                evt_code = e_bcm_evt_monitor_on_trunk_stat;
            else if ( buff[2] == 0x62 ) // 침입감지(후드)
                evt_code = e_bcm_evt_monitor_on_hood_stat;
            break;
        }
        case 0x72:
        {
            if ( buff[2] == 0x69 ) //  노크센서 ID 요청
                evt_code = e_bcm_evt_knocksensor_set_id_req;
            else if ( buff[2] == 0x74 ) //  노크센서  TimeStamp 요청	
                evt_code = e_bcm_evt_knocksensor_set_timestamp_req;
            break;
        }
        default:
            evt_code = e_bcm_evt_monitor_none;
            break;
    }
//     buff[3]; // data len : 의미없음?
    if ( evt_code == e_bcm_evt_monitor_none )
        return ALLKEY_BCM_RET_FAIL;

//   연속적인 이벤트 처리건
    if ( chk_double_evt == 1 ) 
    {
        if ( evt_code == last_event )
            return ALLKEY_BCM_RET_SUCCESS;

        last_event = evt_code;
    }

    if ( g_mdm_evt_proc != NULL )
        return g_mdm_evt_proc(evt_code, buff[4], buff[5], buff[6]);
    else // 만약에 proc 가 없으면 그냥 성공이라고 하자. 성공이라고 리턴하면 echo 보낸다.
        return ALLKEY_BCM_RET_SUCCESS;

    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__get_info(ALLKEY_BCM_1_INFO_T* allkey_bcm_info)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = 'O';
    send_data = 0x00;

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    if ( allkey_bcm_info != NULL )
    {
        allkey_bcm_info->init_stat = 1;
        allkey_bcm_info->horn_cnt = recv_buff[4] >> 4;
        allkey_bcm_info->light_cnt = recv_buff[4] & 0x0f;
        allkey_bcm_info->bcm_swver = recv_buff[5];
    }

    // debug..
    if(1)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__set_horn_light(int horn_cnt, int light_cnt)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    unsigned char conv_horn_cnt = ( horn_cnt << 4 ) & 0xf0;
    unsigned char conv_light_cnt = light_cnt & 0x0f ;

    send_cmd = 'O';

    send_data = conv_horn_cnt + conv_light_cnt;
    printf( " >>> horn cnt conv [0x%x]\r\n", conv_horn_cnt);
    printf( " >>> light cnt conv [0x%x]\r\n", conv_light_cnt);
    printf( " >>> send data conv [0x%x]\r\n", send_data);
    

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);
/*
    if ( allkey_bcm_info != NULL )
    {
        allkey_bcm_info->init_stat = 1;
        allkey_bcm_info->horn_cnt = recv_buff[4] >> 4;
        allkey_bcm_info->light_cnt = recv_buff[4] & 0x0f;
        allkey_bcm_info->bcm_swver = recv_buff[5];
    }
*/
    // debug..
    if(1)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}




void allkey_bcm_1_read_thread(void)
{
    unsigned char allkey_bcm_recv_data[128] = {0,};
    unsigned char cmd_recv_buff[16] = {0,};

    int uart_ret = 0;
    int usleep_time = 0;

    while(_g_run_allkey_bcm_1_thread_run)
    {
        if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);

        // 기존 버퍼에 쌓여 있는것들은 모두 제거한다. 어차피 못받으면 또 보낸다고하니 .. 
        //mds_api_uart_flush(_bcm_fd, 0, 300);
        mds_api_uart_read2(_bcm_fd, allkey_bcm_recv_data,  sizeof(allkey_bcm_recv_data), 100000);

        memset(allkey_bcm_recv_data, 0x00, 128);
        uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data,  sizeof(allkey_bcm_recv_data), ALLKEY_BCM_READ_THREAD_TIMEOUT);
        
        if ( uart_ret == 8 )
        {
            printf("----------------- read allkey bcm 1 evt [%d]----------------------\r\n", uart_ret);
            mds_api_debug_hexdump_buff(allkey_bcm_recv_data, uart_ret);

            memset(cmd_recv_buff, 0x00, 16);
            if ( allkey_evt_proc(allkey_bcm_recv_data) == ALLKEY_BCM_RET_SUCCESS )
            {
                _allkey_bcm_cmd(NOT_USE_MUTEX_LOCK, e_cmd_evt_recv, 'O', cmd_recv_buff);
                printf(" >>> recv proc success :: echo send\r\n");
                //usleep_time = 200;
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt clear", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
            }
            else
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt fail", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                //usleep_time = 0;
            }
        }
        else
        {
            //printf("[ALLKEY_BCM_1] bcm cannot read anything...\r\n");
            ;
        }

        //printf(" >> allkey bcm 1 thread  mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
        usleep(200); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
    }
}



int start_allkey_bcm_1_thread()
{
    _g_run_allkey_bcm_1_thread_run = 1;
    pthread_create(&tid_allkey_bcm_1_thread, NULL, allkey_bcm_1_read_thread, NULL);
    return ALLKEY_BCM_RET_SUCCESS;
}

int stop_allkey_bcm_1_thread()
{
    _g_run_allkey_bcm_1_thread_run = 0;
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_1_init(int (*p_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list))
{
    g_mdm_evt_proc = p_mdm_evt_proc;
    start_allkey_bcm_1_thread();
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_1_chk_stat(const unsigned int evt, const unsigned char stat)
{
    if ( stat & evt )
        return 1;
    else 
        return 0;

    return 0;
}





=======
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>


#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "allkey_bcm_1.h"
#include "allkey_bcm_1_internal.h"

pthread_t tid_allkey_bcm_1_thread;


static int (*g_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list) = NULL;


static pthread_mutex_t allkey_bcm_1_mutex = PTHREAD_MUTEX_INITIALIZER;
int _g_run_allkey_bcm_1_thread_run;

static int _bcm_fd = ALLKEY_BCM_INVAILD_FD;

static int _allkey_bcm_devinit()
{
    int max_chk_cnt = ALLKEY_BCM_MAX_CHK_DEV_CNT;
    int ret_val = ALLKEY_BCM_RET_FAIL;

    while(max_chk_cnt--)
    {
        if ( _bcm_fd == ALLKEY_BCM_INVAILD_FD )
            _bcm_fd = mds_api_init_uart(ALLKEY_BCM_UART_DEVNAME, ALLKEY_BCM_UART_BAUDRATE);
        
        if ( _bcm_fd > 0 ) 
        {
            ret_val = ALLKEY_BCM_RET_SUCCESS;
            break;
        }
        else
        {
            ret_val = ALLKEY_BCM_RET_FAIL;
            _bcm_fd = ALLKEY_BCM_INVAILD_FD;
        }
    }
    return ret_val;
}

static int _allkey_bcm_cmd(int auto_lock, unsigned char cmd_type, unsigned char cmd_data, unsigned char recv_buff[8])
{
    int uart_ret = 0;
    int ret_val = 0;

    unsigned char allkey_bcm_send_cmd[4] = {0,};
    unsigned char allkey_bcm_recv_data[8] = {0,};
    
    int read_len = 0;
    int to_read = sizeof(allkey_bcm_recv_data);
    int read_retry_cnt = ALLKEY_BCM_UART_READ_RETRY_CNT;
    
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex lock\r\n");
        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);
    }

    if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    // make command.
    allkey_bcm_send_cmd[0] = 0x02;
    allkey_bcm_send_cmd[1] = cmd_type;
    allkey_bcm_send_cmd[2] = cmd_data;
    allkey_bcm_send_cmd[3] = 0xff;

    // do not ret chk
    mds_api_uart_write(_bcm_fd, allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
	
    while(read_retry_cnt--)
    {
	    uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data + read_len,  sizeof(allkey_bcm_recv_data) - read_len, ALLKEY_BCM_UART_READ_TIMEOUT);
        if ( uart_ret <= 0 )
            continue;

        read_len += uart_ret;

        if ( read_len >= to_read )
            break;
    }


    if (read_len != to_read)
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    if ( allkey_bcm_recv_data[0] != 0x02 )
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[1] != cmd_type)
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[2] != 0x41 ) // ack : 'A'
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

//    allkey_bcm_recv_data[3]; // data len??

//    allkey_bcm_recv_data[4]; // stat1
//    allkey_bcm_recv_data[5]; // stat2
//    allkey_bcm_recv_data[6]; // error list
//    allkey_bcm_recv_data[7]; // endof frame

    memcpy(recv_buff, allkey_bcm_recv_data, sizeof(allkey_bcm_recv_data));
    ret_val = ALLKEY_BCM_RET_SUCCESS;

FINISH:
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
    }

    return ret_val;
}

static int _allkey_bcm_cmd2(int auto_lock, unsigned char cmd_type, unsigned char cmd_data, unsigned char ext_data[4], unsigned char recv_buff[9])
{
    int uart_ret = 0;
    int ret_val = 0;

    unsigned char allkey_bcm_send_cmd[9] = {0,};
    //unsigned char allkey_bcm_recv_data[9] = {0,};
    
    int read_len = 0;
    //int to_read = sizeof(allkey_bcm_recv_data);
    int read_retry_cnt = 0;
    
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex lock\r\n");
        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);
    }

    if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }
    
    // make command.
    allkey_bcm_send_cmd[0] = 0x02;
    allkey_bcm_send_cmd[1] = cmd_type; // CMD List 참조    
    allkey_bcm_send_cmd[2] = cmd_data; // DATA List 참조
    allkey_bcm_send_cmd[3] = 0x04; // DATA Length (0x04)
    allkey_bcm_send_cmd[4] = ext_data[0]; // data1 
    allkey_bcm_send_cmd[5] = ext_data[1]; // data2
    allkey_bcm_send_cmd[6] = ext_data[2]; // data3
    allkey_bcm_send_cmd[7] = ext_data[3]; // data4
    allkey_bcm_send_cmd[8] = 0xff;

    //printf("allkey bcm cmd 2 api write ------------------------------ ++ [%d] \r\n", sizeof(allkey_bcm_send_cmd));
    //mds_api_debug_hexdump_buff(allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
    //printf("allkey bcm cmd 2 api write ------------------------------ -- [%d] \r\n", sizeof(allkey_bcm_send_cmd));

    // do not ret chk
    uart_ret = mds_api_uart_write(_bcm_fd, allkey_bcm_send_cmd, sizeof(allkey_bcm_send_cmd));
    
    /*
    while(read_retry_cnt--)
    {
        uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data + read_len,  sizeof(allkey_bcm_recv_data) - read_len, ALLKEY_BCM_UART_READ_TIMEOUT);

        if ( uart_ret <= 0 )
            continue;

        read_len += uart_ret;

        if ( read_len >= to_read )
            break;
    }

    printf("allkey bcm cmd 2 api read ------------------------------ ++ [%d] \r\n", read_len);
    mds_api_debug_hexdump_buff(allkey_bcm_recv_data, read_len);
    printf("allkey bcm cmd 2 api read ------------------------------ -- [%d] \r\n", read_len);


    if (read_len != to_read)
    {
        ret_val = ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[0] != 0x02 )
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[1] != cmd_type)
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

    if ( allkey_bcm_recv_data[2] != 0x41 ) // ack : 'A'
    {
        ret_val =  ALLKEY_BCM_RET_FAIL;
        goto FINISH;
    }

//    allkey_bcm_recv_data[3]; // data len??

//    allkey_bcm_recv_data[4]; // stat1
//    allkey_bcm_recv_data[5]; // stat2
//    allkey_bcm_recv_data[6]; // error list
//    allkey_bcm_recv_data[7]; // endof frame

    memcpy(recv_buff, allkey_bcm_recv_data, sizeof(allkey_bcm_recv_data));
    
*/
    if ( uart_ret > 0 )
        ret_val = ALLKEY_BCM_RET_SUCCESS;
    else
        ret_val = ALLKEY_BCM_RET_FAIL;
FINISH:
    if ( auto_lock )
    {
        //printf(" >> allkey bcm 1 cmd mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
    }

    return ret_val;
}


int allkey_bcm_cmd__get_stat(ALLKEY_BCM_1_DEV_T* dev_stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_req_stat;
    send_data = 'O';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    // printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        printf("bcm recv : ");
        for ( i = 0 ; i < 8 ; i ++ )
            printf("[0x%02x]", recv_buff[i]);
        printf("\r\n");
    }

    // bcm recv : [0x02][0x53][0x41][0x03][0x02][0x20][0x00][0xff]
    if ( ( recv_buff[0] == 0x02 ) && ( recv_buff[1] == 0x53) )
    {
        dev_stat->door_open_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x01); 
        dev_stat->door_lock_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x02); 
        dev_stat->engine_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x04); 
        dev_stat->remote_start_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x08); 
        dev_stat->hood_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x10); 
        dev_stat->turbo_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x20); 
        dev_stat->timer_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x40); 
        dev_stat->mute_stat = allkey_bcm_1_chk_stat(recv_buff[4], 0x80); 

        dev_stat->shock_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x01);
        dev_stat->acc_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x02);
        dev_stat->trunk_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x04);
        dev_stat->always_evt_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x08);
        dev_stat->ig_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x10);
        dev_stat->theif_sensor_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x20);
        dev_stat->valet_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x40);
        dev_stat->alert_stat = allkey_bcm_1_chk_stat(recv_buff[5], 0x80);
/*
        printf("dev_stat->door_open_stat is [%d]\r\n", dev_stat->door_open_stat);
        printf("dev_stat->door_lock_stat is [%d]\r\n", dev_stat->door_lock_stat);
        printf("dev_stat->engine_stat is [%d]\r\n", dev_stat->engine_stat);
        printf("dev_stat->remote_start_stat is [%d]\r\n", dev_stat->remote_start_stat);
        printf("dev_stat->hood_stat is [%d]\r\n", dev_stat->hood_stat);
        printf("dev_stat->turbo_stat is [%d]\r\n", dev_stat->turbo_stat);
        printf("dev_stat->timer_stat is [%d]\r\n", dev_stat->timer_stat);
        printf("dev_stat->mute_stat is [%d]\r\n", dev_stat->mute_stat);

        printf("dev_stat->shock_stat is [%d]\r\n", dev_stat->shock_stat);
        printf("dev_stat->acc_stat is [%d]\r\n", dev_stat->acc_stat);
        printf("dev_stat->trunk_stat is [%d]\r\n", dev_stat->trunk_stat);
        printf("dev_stat->always_evt_stat is [%d]\r\n", dev_stat->always_evt_stat);
        printf("dev_stat->ig_stat is [%d]\r\n", dev_stat->ig_stat);
        printf("dev_stat->theif_sensor_stat is [%d]\r\n", dev_stat->theif_sensor_stat);
        printf("dev_stat->valet_stat is [%d]\r\n", dev_stat->valet_stat);
        printf("dev_stat->alert_stat is [%d]\r\n", dev_stat->alert_stat);
*/
    }


    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__door_lock(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_door_ctr;
    if ( stat )
        send_data = 'L';
    else
        send_data = 'U';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    if (0)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__door_evt_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_door_evt_setting;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__horn_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_horn_ctr;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;

}


int allkey_bcm_ctr__light_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_twingkle_light_ctr;
    if ( stat )
        send_data = 'O';
    else
        send_data = 'F';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__theft_on(int stat)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = e_cmd_twingkle_light_ctr;
    if ( stat )
        send_data = 'F';
    else
        send_data = 'O';

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    // debug..
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


static int _allkey_bcm_ctr__knocksensor_set_id(int use_mutex_lock, unsigned short id)
{
    // unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];

    unsigned short arg_tmp = id;

    unsigned short arg_tmp_1 = 0;
    unsigned short arg_tmp_2 = 0;

    unsigned char tmp_digit[4] = {0,};
    int i = 0;
    
    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_id;

    for( i = 0 ; i < 4 ; i ++)
    {
        tmp_digit[3-i] = arg_tmp % 10 ;
        arg_tmp = arg_tmp / 10;
    }

    arg_tmp = 0;
    
    arg_tmp_1 += tmp_digit[0] * 0x10;
    arg_tmp_1 += tmp_digit[1] * 0x1;
    arg_tmp_2 += tmp_digit[2] * 0x10;
    arg_tmp_2 += tmp_digit[3] * 0x1;

    printf("arg_tmp_1 [0x%x], arg_tmp_2 [0x%x]\r\n", arg_tmp_1, arg_tmp_2);
    /*
    for( i = 0 ; i < 4 ; i ++)
    {
        arg_tmp += tmp_digit[i] * (16^(3-i));
        printf("arg_tmp [0x%x] / tmp_digit[%d]=[0x%x] / [0x%x]\r\n", arg_tmp, i, tmp_digit[i], (16^(3-i)));
    }
    */

    ext_data[0] = arg_tmp_1;
    ext_data[1] = arg_tmp_2;
    ext_data[2] = 0;
    ext_data[3] = 0;

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, id, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }
    
    printf("%s() success\r\n", __func__);


    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__knocksensor_set_id(unsigned short id)
{
    return _allkey_bcm_ctr__knocksensor_set_id(USE_MUTEX_LOCK, id);
}

int allkey_bcm_ctr__knocksensor_set_id_evt_proc(unsigned short id)
{
    return _allkey_bcm_ctr__knocksensor_set_id(NOT_USE_MUTEX_LOCK, id);
}


int _allkey_bcm_ctr__knocksensor_set_passwd(int use_mutex_lock, unsigned short passwd)
{
   //unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];

    unsigned short arg_tmp = passwd;
    
    unsigned short arg_tmp_1 = 0;
    unsigned short arg_tmp_2 = 0;

    unsigned char tmp_digit[4] = {0,};
    int i = 0;

    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_passwd;

    for( i = 0 ; i < 4 ; i ++)
    {
        tmp_digit[3-i] = arg_tmp % 10 ;
        arg_tmp = arg_tmp / 10;
    }

    arg_tmp = 0;
      
    arg_tmp_1 += tmp_digit[0] * 0x10;
    arg_tmp_1 += tmp_digit[1] * 0x1;
    arg_tmp_2 += tmp_digit[2] * 0x10;
    arg_tmp_2 += tmp_digit[3] * 0x1;

    printf("arg_tmp_1 [0x%x], arg_tmp_2 [0x%x]\r\n", arg_tmp_1, arg_tmp_2);
    /*
    for( i = 0 ; i < 4 ; i ++)
    {
        arg_tmp += tmp_digit[i] * (16^(3-i));
        printf("arg_tmp [0x%x] / tmp_digit[%d]=[0x%x] / [0x%x]\r\n", arg_tmp, i, tmp_digit[i], (16^(3-i)));
    }
    */

    ext_data[0] = arg_tmp_1;
    ext_data[1] = arg_tmp_2;
    ext_data[2] = 0;
    ext_data[3] = 0;

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, passwd, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }

    printf("%s() success\r\n", __func__);

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__knocksensor_set_passwd(unsigned short passwd)
{
    _allkey_bcm_ctr__knocksensor_set_passwd(USE_MUTEX_LOCK, passwd);
}

int allkey_bcm_ctr__knocksensor_set_passwd_evt_proc(unsigned short passwd)
{
    _allkey_bcm_ctr__knocksensor_set_passwd(NOT_USE_MUTEX_LOCK, passwd);
}


/*
Unix time을 4byte 로 보내주시면 됩니다.
현재 유닉스 타임이 1234567890 이라면 4996 02D2 이고 위 표에 timestamp1 = 49, timestamp2=96, timestamp3=02, timestamp4=D2 가 됩니다.
02 52 54 04 49 96 02 D2 FF
*/

static int _allkey_bcm_ctr__knocksensor_set_modemtime(int use_mutex_lock)
{
    // unsigned char recv_buff[9];

    unsigned char send_cmd = 0;
    unsigned char send_data = 0;
    unsigned char ext_data[4];
    unsigned char ext_data_tmp[4];

    int modemtime_utc =  get_modem_time_utc_sec(); // 1234567890;

    send_cmd = e_cmd_knocksensor;
    send_data = e_knock_cmd_timedate;
    
    memcpy(&ext_data_tmp, &modemtime_utc, 4);

    ext_data[3] = ext_data_tmp[0];
    ext_data[2] = ext_data_tmp[1];
    ext_data[1] = ext_data_tmp[2];
    ext_data[0] = ext_data_tmp[3];

    printf("%s() input val [%d] => [0x%x], [0x%x], [0x%x], [0x%x]\r\n", __func__, modemtime_utc, ext_data[0], ext_data[1], ext_data[2], ext_data[3]);

    if ( _allkey_bcm_cmd2(use_mutex_lock, send_cmd, send_data, ext_data, NULL) == ALLKEY_BCM_RET_FAIL )
    {
        printf("%s() fail\r\n", __func__);
        return ALLKEY_BCM_RET_FAIL;
    }

    printf("%s() success\r\n", __func__);

    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__knocksensor_set_modemtime_evt_proc()
{
    // EVENT PROC 에서는 이미 MUTEX LOCK 상태에서 들어온다.
    // 때문에 해당 커맨드에서 다시 MUTEXT LOCK 하면 HANG 에 빠짐, MUTEX LOCK 하지 말고 사용
    return _allkey_bcm_ctr__knocksensor_set_modemtime(NOT_USE_MUTEX_LOCK);
}

int allkey_bcm_ctr__knocksensor_set_modemtime()
{
    return _allkey_bcm_ctr__knocksensor_set_modemtime(USE_MUTEX_LOCK);
}



// ...


int allkey_evt_proc(const unsigned char buff[8])
{
    int ret_val = ALLKEY_BCM_RET_FAIL;
    int evt_code = e_bcm_evt_monitor_none;

    static int last_event = e_bcm_evt_monitor_none;
    int chk_double_evt = 0;


    if (( buff[0] != 0x02 ) || ( buff[7] != 0xff ) )
        return ALLKEY_BCM_RET_FAIL;

    switch (buff[1])
    {
        case 0x63: 
        {
            if ( buff[2] == 0x6F) // 차주호출
                evt_code = e_bcm_evt_driver_call;
            break;
        }
        case 0x64:
        {
            if ( buff[2] == 0x6c ) // 경계 해제시 도어잠김
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt recv : door close", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                evt_code = e_bcm_evt_monitor_off_door_open;
                chk_double_evt = 1;
            }
            else if ( buff[2] == 0x75 ) // 경계 해제시 도어열림
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt recv : door open", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                evt_code = e_bcm_evt_monitor_off_door_close;
                chk_double_evt = 1;
            }
            break;
        }
        case 0x6b:
        {
            if ( buff[2] == 0x77 ) // 약한 충격 감지
                evt_code = e_bcm_evt_small_shock;
            else if ( buff[2] == 0x73 ) // 강한 충격 감지
                evt_code = e_bcm_evt_big_shock;
            break;
        }
        case 0x74:
        {
            if ( buff[2] == 0x6c ) // 경계 해제 시 트렁크 닫힘
            {
                evt_code = e_bcm_evt_monitor_off_trunk_open;
                chk_double_evt = 1;
            }
            else if ( buff[2] == 0x75 ) // 경계해제 시 트렁크 열림
            {
                evt_code = e_bcm_evt_monitor_off_trunk_close;
                chk_double_evt = 1;
            }
            break;
        }
        case 0x76:
        {
            if ( buff[2] == 0x64 ) // 침입감지(도어)
                evt_code = e_bcm_evt_monitor_on_door_stat;
            else if ( buff[2] == 0x74 ) // 침입감지(트렁크)
                evt_code = e_bcm_evt_monitor_on_trunk_stat;
            else if ( buff[2] == 0x62 ) // 침입감지(후드)
                evt_code = e_bcm_evt_monitor_on_hood_stat;
            break;
        }
        case 0x72:
        {
            if ( buff[2] == 0x69 ) //  노크센서 ID 요청
                evt_code = e_bcm_evt_knocksensor_set_id_req;
            else if ( buff[2] == 0x74 ) //  노크센서  TimeStamp 요청	
                evt_code = e_bcm_evt_knocksensor_set_timestamp_req;
            break;
        }
        default:
            evt_code = e_bcm_evt_monitor_none;
            break;
    }
//     buff[3]; // data len : 의미없음?
    if ( evt_code == e_bcm_evt_monitor_none )
        return ALLKEY_BCM_RET_FAIL;

//   연속적인 이벤트 처리건
    if ( chk_double_evt == 1 ) 
    {
        if ( evt_code == last_event )
            return ALLKEY_BCM_RET_SUCCESS;

        last_event = evt_code;
    }

    if ( g_mdm_evt_proc != NULL )
        return g_mdm_evt_proc(evt_code, buff[4], buff[5], buff[6]);
    else // 만약에 proc 가 없으면 그냥 성공이라고 하자. 성공이라고 리턴하면 echo 보낸다.
        return ALLKEY_BCM_RET_SUCCESS;

    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_ctr__get_info(ALLKEY_BCM_1_INFO_T* allkey_bcm_info)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    send_cmd = 'O';
    send_data = 0x00;

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);

    if ( allkey_bcm_info != NULL )
    {
        allkey_bcm_info->init_stat = 1;
        allkey_bcm_info->horn_cnt = recv_buff[4] >> 4;
        allkey_bcm_info->light_cnt = recv_buff[4] & 0x0f;
        allkey_bcm_info->bcm_swver = recv_buff[5];
    }

    // debug..
    if(1)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}


int allkey_bcm_ctr__set_horn_light(int horn_cnt, int light_cnt)
{
    unsigned char recv_buff[8];
    unsigned char send_cmd = 0;
    unsigned char send_data = 0;

    unsigned char conv_horn_cnt = ( horn_cnt << 4 ) & 0xf0;
    unsigned char conv_light_cnt = light_cnt & 0x0f ;

    send_cmd = 'O';

    send_data = conv_horn_cnt + conv_light_cnt;
    printf( " >>> horn cnt conv [0x%x]\r\n", conv_horn_cnt);
    printf( " >>> light cnt conv [0x%x]\r\n", conv_light_cnt);
    printf( " >>> send data conv [0x%x]\r\n", send_data);
    

    if ( _allkey_bcm_cmd(USE_MUTEX_LOCK, send_cmd, send_data, recv_buff) == ALLKEY_BCM_RET_FAIL )
        return ALLKEY_BCM_RET_FAIL;

    printf("%s() success\r\n", __func__);
/*
    if ( allkey_bcm_info != NULL )
    {
        allkey_bcm_info->init_stat = 1;
        allkey_bcm_info->horn_cnt = recv_buff[4] >> 4;
        allkey_bcm_info->light_cnt = recv_buff[4] & 0x0f;
        allkey_bcm_info->bcm_swver = recv_buff[5];
    }
*/
    // debug..
    if(1)
    {
        int i = 0;
        for ( i = 0 ; i < 8 ; i ++ )
            printf("recv buf [%d] => [0x%x]\r\n", i, recv_buff[i]);
    }

    return ALLKEY_BCM_RET_SUCCESS;
}




void allkey_bcm_1_read_thread(void)
{
    unsigned char allkey_bcm_recv_data[128] = {0,};
    unsigned char cmd_recv_buff[16] = {0,};

    int uart_ret = 0;
    int usleep_time = 0;

    while(_g_run_allkey_bcm_1_thread_run)
    {
        if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        //printf("%s():%d : mutex lock\r\n", __func__, __LINE__);
        pthread_mutex_lock(&allkey_bcm_1_mutex);

        // 기존 버퍼에 쌓여 있는것들은 모두 제거한다. 어차피 못받으면 또 보낸다고하니 .. 
        //mds_api_uart_flush(_bcm_fd, 0, 300);
        mds_api_uart_read2(_bcm_fd, allkey_bcm_recv_data,  sizeof(allkey_bcm_recv_data), 100000);

        memset(allkey_bcm_recv_data, 0x00, 128);
        uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data,  sizeof(allkey_bcm_recv_data), ALLKEY_BCM_READ_THREAD_TIMEOUT);
        
        if ( uart_ret == 8 )
        {
            printf("----------------- read allkey bcm 1 evt [%d]----------------------\r\n", uart_ret);
            mds_api_debug_hexdump_buff(allkey_bcm_recv_data, uart_ret);

            memset(cmd_recv_buff, 0x00, 16);
            if ( allkey_evt_proc(allkey_bcm_recv_data) == ALLKEY_BCM_RET_SUCCESS )
            {
                _allkey_bcm_cmd(NOT_USE_MUTEX_LOCK, e_cmd_evt_recv, 'O', cmd_recv_buff);
                printf(" >>> recv proc success :: echo send\r\n");
                //usleep_time = 200;
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt clear", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
            }
            else
            {
            //#ifdef BCM_EVT_DEGUG_LOG
            //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt fail", BCM_EVT_DBG_LOG_MAX_SIZE);
            //#endif
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                printf(" >>> recv proc fail :: do nothing\r\n");
                //usleep_time = 0;
            }
        }
        else
        {
            //printf("[ALLKEY_BCM_1] bcm cannot read anything...\r\n");
            ;
        }

        //printf(" >> allkey bcm 1 thread  mutex unlock\r\n");
        //printf("%s():%d : mutex un lock\r\n", __func__, __LINE__);
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
        usleep(200); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
    }
}



int start_allkey_bcm_1_thread()
{
    _g_run_allkey_bcm_1_thread_run = 1;
    pthread_create(&tid_allkey_bcm_1_thread, NULL, allkey_bcm_1_read_thread, NULL);
    return ALLKEY_BCM_RET_SUCCESS;
}

int stop_allkey_bcm_1_thread()
{
    _g_run_allkey_bcm_1_thread_run = 0;
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_1_init(int (*p_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list))
{
    g_mdm_evt_proc = p_mdm_evt_proc;
    start_allkey_bcm_1_thread();
    return ALLKEY_BCM_RET_SUCCESS;
}

int allkey_bcm_1_chk_stat(const unsigned int evt, const unsigned char stat)
{
    if ( stat & evt )
        return 1;
    else 
        return 0;

    return 0;
}





>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

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
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
    }

    return ret_val;
}


int allkey_bcm_cmd__get_stat()
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




int allkey_evt_proc(const unsigned char buff[8])
{
    int ret_val = ALLKEY_BCM_RET_FAIL;
    int evt_code = e_bcm_evt_monitor_none;

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
                evt_code = e_bcm_evt_monitor_off_door_open;
            else if ( buff[2] == 0x75 ) // 경계 해제시 도어열림
                evt_code = e_bcm_evt_monitor_off_door_close;
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
        default:
            evt_code = e_bcm_evt_monitor_none;
    }
//     buff[3]; // data len : 의미없음?
    if ( evt_code == e_bcm_evt_monitor_none )
        return ALLKEY_BCM_RET_FAIL;

    if ( g_mdm_evt_proc != NULL )
        return g_mdm_evt_proc(evt_code, buff[4], buff[5], buff[6]);
    else // 만약에 proc 가 없으면 그냥 성공이라고 하자. 성공이라고 리턴하면 echo 보낸다.
        return ALLKEY_BCM_RET_SUCCESS;
    
    return ret_val;
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

    while(_g_run_allkey_bcm_1_thread_run)
    {
        if ( _allkey_bcm_devinit() == ALLKEY_BCM_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        //printf(" >> allkey bcm 1 thread mutex lock\r\n");
        pthread_mutex_lock(&allkey_bcm_1_mutex);

        memset(allkey_bcm_recv_data, 0x00, 128);
        uart_ret =  mds_api_uart_read(_bcm_fd, allkey_bcm_recv_data,  sizeof(allkey_bcm_recv_data), ALLKEY_BCM_READ_THREAD_TIMEOUT);
        if ( uart_ret == 8 )
        {
            printf("----------------- read allkey bcm 1 evt [%d]----------------------\r\n", uart_ret);
            mds_api_debug_hexdump_buff(allkey_bcm_recv_data, uart_ret);

            memset(cmd_recv_buff, 0x00, 16);
            if ( allkey_evt_proc(allkey_bcm_recv_data) == ALLKEY_BCM_RET_SUCCESS )
            {
                printf(" >>> recv proc success :: echo send\r\n");
                _allkey_bcm_cmd(NOT_USE_MUTEX_LOCK, e_cmd_evt_recv, 'O', cmd_recv_buff);
            }
            else
            {
                printf(" >>> recv proc fail :: do nothing\r\n");
            }
        }
        else
        {
            //printf("[ALLKEY_BCM_1] bcm cannot read anything...\r\n");
            ;
        }

        //printf(" >> allkey bcm 1 thread  mutex unlock\r\n");
        pthread_mutex_unlock(&allkey_bcm_1_mutex);
        usleep(500); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
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






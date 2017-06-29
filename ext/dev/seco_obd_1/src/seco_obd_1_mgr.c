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

#include "seco_obd_1.h"
#include "seco_obd_1_mgr.h"
#include "seco_obd_1_protocol.h"
#include "seco_obd_1_util.h"


pthread_t tid_seco_obd_1_thread = NULL;

static int _g_run_seco_obd_1_thread_run = 0;
static pthread_mutex_t seco_obd_1_mutex = PTHREAD_MUTEX_INITIALIZER;

#define BROAD_CAST_CHK_MSG "OBD+SBR+MES="

static int (*g_bmsg_proc)(int argc, char* argv[]) = NULL;

void seco_obd_1_mutex_lock()
{
    pthread_mutex_lock(&seco_obd_1_mutex);
}

void seco_obd_1_mutex_unlock()
{
    pthread_mutex_unlock(&seco_obd_1_mutex);
}



// 브로드케스트 명령어획득을 위한 쓰래드
void seco_obd_1_read_thread(void)
{
    int obd_fd = -1;
    int uart_ret = 0;
    char seco_obd_1_recv_data[512] ={0,};
    char tmp_recv_data[512] ={0,};
    int broad_cast_chk_msg_len;

    while(_g_run_seco_obd_1_thread_run)
    {
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        // printf("[obd read thread] run..\r\n");
        if ( _seco_obd_uart_chk() != OBD_RET_SUCCESS )
        {
            printf("[obd read thread] obd init fail..\r\n");
            sleep(1);
            continue;
        }

        obd_fd = seco_obd_get_fd();

        if ( obd_fd == SECO_OBD_INVAILD_FD )
        {
            printf("[obd read thread] get obdfd fail\r\n");
            sleep(1);
            continue;
        }

        memset(seco_obd_1_recv_data, 0x00, sizeof(seco_obd_1_recv_data));

        seco_obd_1_mutex_lock();
        uart_ret =  mds_api_uart_read(obd_fd, seco_obd_1_recv_data,  sizeof(seco_obd_1_recv_data), SECO_OBD_1_READ_THREAD_TIMEOUT);
        seco_obd_1_mutex_unlock();

        if ( uart_ret <= 0 )
        {
            if ( g_bmsg_proc != NULL )
                g_bmsg_proc(0, NULL);

            printf("[obd read thread] read fail case 1 do anything...\r\n");
            continue;
        }

        // cr, lr 을 제거한다.
        uart_ret = mds_api_remove_cr(seco_obd_1_recv_data, tmp_recv_data, 512);

        if ( uart_ret <= 0 )
        {
            if ( g_bmsg_proc != NULL )
                g_bmsg_proc(0, NULL);

            printf("[obd read thread] read fail case 2 do anything...\r\n");
            continue;
        }

        strcpy(seco_obd_1_recv_data, tmp_recv_data);
        memset(tmp_recv_data, 0x00, 512);

        printf("[obd read thread] read sucess [%s] \r\n", seco_obd_1_recv_data);
        broad_cast_chk_msg_len = strlen(BROAD_CAST_CHK_MSG);
        
        if ( strncmp(seco_obd_1_recv_data, BROAD_CAST_CHK_MSG, broad_cast_chk_msg_len) == 0 )
        {
            char* tmp_argv[20] = {0,};
            int tmp_argc = 0;
            int tmp_recv_data_len = 0;
            
            tmp_recv_data_len = sprintf(tmp_recv_data,"%s",seco_obd_1_recv_data + broad_cast_chk_msg_len);
            //printf("tmp_recv_data is [%s] / [%d] \r\n", tmp_recv_data, broad_cast_chk_msg_len);

            tmp_argc = _seco_obd_1_device_argument(tmp_recv_data, tmp_recv_data_len, tmp_argv);
            if ( g_bmsg_proc != NULL )
                g_bmsg_proc(tmp_argc, tmp_argv);
        }
        else
        {
            if ( g_bmsg_proc != NULL )
                g_bmsg_proc(0, NULL);
            
            printf("[obd read thread] read fail case 3 do anything...\r\n");

            continue;
        }
        
        //printf(" >> allkey bcm 1 thread  mutex unlock\r\n");
        
        
    }

}

int start_seco_obd_1_thread()
{
    _g_run_seco_obd_1_thread_run = 1;
    if ( tid_seco_obd_1_thread == NULL )
        pthread_create(&tid_seco_obd_1_thread, NULL, seco_obd_1_read_thread, NULL);

    return 0;
}

int stop_seco_obd_1_thread()
{
    _g_run_seco_obd_1_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int init_seco_obd_mgr(char* dev_name, int baud_rate, int (*p_bmsg_proc)(int argc, char* argv[]))
{
    seco_obd_1_deinit();

    strcpy(g_obd_dev_path_str, dev_name);
    g_obd_dev_baudrate = baud_rate;
    g_bmsg_proc = p_bmsg_proc;

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, obd 는 이미 계속 메시지를 뿌리고있기 때문.
    stop_seco_obd_1_broadcast_msg();
    start_seco_obd_1_thread();

    return 0;
}



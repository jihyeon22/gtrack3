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

#include "adas_common.h"

#include "mobileye_adas.h"
#include "mobileye_adas_mgr.h"
#include "mobileye_adas_protocol.h"
#include "mobileye_adas_tool.h"
#include "mobileye_adas_uart_util.h"



#define MAX_READ_ADAS_UART_INVAILD_CHK 200

pthread_t tid_mobileye_adas_thread = NULL;

static int _g_run_mobileye_adas_thread_run = 0;
static pthread_mutex_t mobileye_adas_mutex = PTHREAD_MUTEX_INITIALIZER;

static int (*g_mobileye_adas_bmsg_proc)(ADAS_EVT_DATA_T* evt_data) = NULL;

void mobileye_adas__mgr_mutex_lock()
{
    pthread_mutex_lock(&mobileye_adas_mutex);
}

void mobileye_adas__mgr_mutex_unlock()
{
    pthread_mutex_unlock(&mobileye_adas_mutex);
}


// 브로드케스트 명령어획득을 위한 쓰래드
void mobileye_adas__mgr_read_thread(void)
{
    int  mobileye_adas_fd = -1;
    int  uart_ret = 0;
    char mobileye_adas_recv_data[MAX_MOBILEYE_ADAS_RET_BUFF_SIZE] ={0,};

    static int  to_line_read = 0;
    char* p_tmp_buff = NULL;

    int parse_success_flag = 0;

    static int read_fail_cnt = 0;
    mobileye_adas__cmd_broadcast_msg_start();

    while(_g_run_mobileye_adas_thread_run)
    {
        parse_success_flag = 0;
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_READ_ADAS_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[mobileye_adas read thread] run..\r\n");
        if ( mobileye_adas__uart_chk() != MOBILEYE_ADAS_RET_SUCCESS )
        {
            printf("[mobileye_adas read thread] mobileye_adas init fail..\r\n");
            sleep(1);
            continue;
        }

        mobileye_adas_fd = mobileye_adas__uart_get_fd();

        if ( mobileye_adas_fd == MOBILEYE_ADAS_INVAILD_FD )
        {
            printf("[mobileye_adas read thread] get mobileye_adas fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(mobileye_adas_recv_data, 0x00, sizeof(mobileye_adas_recv_data));

        mobileye_adas__mgr_mutex_lock();
        uart_ret =  mds_api_uart_read(mobileye_adas_fd, (void*)mobileye_adas_recv_data,  sizeof(mobileye_adas_recv_data), MOBILEYE_ADAS_UART_READ_THREAD_TIMEOUT);
        mobileye_adas__mgr_mutex_unlock();

        // cr, lr 을 제거한다.
        //uart_ret = mds_api_remove_cr(mobileye_adas_recv_data, tmp_recv_data, 512);

        if ( uart_ret <= 0 )
        {
            ADAS_EVT_DATA_T evt_data = {0,};
            evt_data.evt_code = eADAS_EVT__INVALID;

            if ( g_mobileye_adas_bmsg_proc != NULL )
                g_mobileye_adas_bmsg_proc(&evt_data);

            // printf("[mobileye_adas read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

 
        printf("---------------------------------------------------------------------\r\n");
        printf("[mobileye_adas read thread] read success [%s] [%d]\r\n", mobileye_adas_recv_data, uart_ret);
        // debug_hexdump_buff(mobileye_adas_recv_data, uart_ret);
        printf("---------------------------------------------------------------------\r\n");

        read_fail_cnt = 0;
        to_line_read = strlen(mobileye_adas_recv_data);
        p_tmp_buff = mobileye_adas_recv_data;

        while( to_line_read > 0 )
        {
            char read_line_buff[MAX_MOBILEYE_ADAS_RET_BUFF_SIZE] = {0,};
            int  read_line_len = 0;
            ADAS_EVT_DATA_T evt_data = {0,};
            evt_data.evt_code = eADAS_EVT__INVALID;

            // 모든데이터는 1 line 이 한개 데이터다.
            // 여러개 line 이 한번에 들어올때가 있어서 버퍼를 읽으면서 proc 에 던진다.
            if ( ( read_line_len = mds_api_read_line(p_tmp_buff, to_line_read, read_line_buff, sizeof(read_line_buff)) ) <= 0 )
                break;

            // debug msg
            /*
            if ( strlen(read_line_buff) > 0 )
                devel_webdm_send_log("ME EVT => %s",read_line_buff);
            */
           
            to_line_read -= read_line_len;
            p_tmp_buff += read_line_len;

            if ( strstr(read_line_buff, MOBILEYE_ADAS_UART_BROAD_CAST_CHK_MSG) != NULL)
            {
                parse_success_flag = 1;
            #ifdef USE_MOBILEYE_ADAS_BYPASS
                evt_data.evt_code = eADAS_EVT__BYPASS;
                strncpy(evt_data.evt_ext, read_line_buff, read_line_len);
            #else
                char tmp_argv[20][128];
                int tmp_argc = 0;
                memset(&tmp_argv, 0x00, sizeof(tmp_argv));
                
                tmp_argc = mobileye_adas__tool_device_argument(read_line_buff, strlen(read_line_buff), tmp_argv);

                if ( tmp_argc > 0 )
                {
                    mobileye_get_evt_data(tmp_argc, tmp_argv, &evt_data);
                }
            #endif
            }
            
            // invalid data (trash data) need to reopen uart.
            if ( parse_success_flag == 0 )
            {
                printf("invalid uart data .... reopen uart !!\r\n");
                printf("invalid uart data .... reopen uart !!\r\n");
                printf("invalid uart data .... reopen uart !!\r\n");
                printf("invalid uart data .... reopen uart !!\r\n");
                mobileye_adas__uart_deinit();
            }
        
            if ( g_mobileye_adas_bmsg_proc != NULL )
                g_mobileye_adas_bmsg_proc(&evt_data);
            
        }

    }

}

int mobileye_adas__mgr_thread_start()
{
    pthread_attr_t attr;
    _g_run_mobileye_adas_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_mobileye_adas_thread == NULL )
        pthread_create(&tid_mobileye_adas_thread, &attr, mobileye_adas__mgr_read_thread, NULL);

    return 0;
}

int mobileye_adas__mgr_thread_stop()
{
    _g_run_mobileye_adas_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int mobileye_adas__mgr_init(char* dev_name, int baud_rate, int (*p_bmsg_proc)(ADAS_EVT_DATA_T* evt_data))
{
    mobileye_adas__uart_deinit();

    mobileye_adas__uart_set_dev(dev_name);
    mobileye_adas__uart_set_baudrate(baud_rate);

    g_mobileye_adas_bmsg_proc = p_bmsg_proc;

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, mobileye_adas 는 이미 계속 메시지를 뿌리고있기 때문.
    mobileye_adas__cmd_broadcast_msg_stop();
    mobileye_adas__mgr_thread_start();

    return 0;
}



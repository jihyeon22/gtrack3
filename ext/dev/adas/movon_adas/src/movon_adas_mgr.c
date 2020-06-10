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

#include "movon_adas.h"
#include "movon_adas_mgr.h"
#include "movon_adas_protocol.h"
#include "movon_adas_tool.h"
#include "movon_adas_uart_util.h"


#include "movon_data_queue.h"

#define MAX_READ_ADAS_UART_INVAILD_CHK 200

pthread_t tid_movon_adas_thread = NULL;

static int _g_run_movon_adas_thread_run = 0;
static pthread_mutex_t movon_adas_mutex = PTHREAD_MUTEX_INITIALIZER;

static int (*g_movon_adas_bmsg_proc)(ADAS_EVT_DATA_T* evt_data) = NULL;

static MOVON_DATA_FRAME_T g_movon_cur_data = {0,};

void movon_adas__mgr_mutex_lock()
{
    pthread_mutex_lock(&movon_adas_mutex);
}

void movon_adas__mgr_mutex_unlock()
{
    pthread_mutex_unlock(&movon_adas_mutex);
}

void movon_adas__set_cur_data(MOVON_DATA_FRAME_T* data)
{
    movon_adas__mgr_mutex_lock();
    memcpy(&g_movon_cur_data, data, sizeof(MOVON_DATA_FRAME_T));
    movon_adas__mgr_mutex_unlock();
}


void movon_adas__get_cur_data(MOVON_DATA_FRAME_T* data)
{
    movon_adas__mgr_mutex_lock();
    memcpy(data, &g_movon_cur_data, sizeof(MOVON_DATA_FRAME_T));
    movon_adas__mgr_mutex_unlock();
}

// 브로드케스트 명령어획득을 위한 쓰래드
void movon_adas__mgr_read_thread(void)
{
    int  movon_adas_fd = -1;
    int  uart_ret = 0;
    char movon_adas_recv_data[MAX_MOVON_ADAS_RET_BUFF_SIZE] ={0,};

//   static int  to_line_read = 0;
//    char* p_tmp_buff = NULL;

    static int read_fail_cnt = 0;

    static int read_invaild_cnt = 0;

    MOVON_DATA_FRAME_T data_frame = {0,};

    movon_adas__cmd_broadcast_msg_start();

    while(_g_run_movon_adas_thread_run)
    {
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_READ_ADAS_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[movon_adas read thread] run..\r\n");
        if ( movon_adas__uart_chk() != MOVON_ADAS_RET_SUCCESS )
        {
            printf("[movon_adas read thread] movon_adas init fail..\r\n");
            sleep(1);
            continue;
        }

        movon_adas_fd = movon_adas__uart_get_fd();

        if ( movon_adas_fd == MOVON_ADAS_INVAILD_FD )
        {
            printf("[movon_adas read thread] get movon_adas fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(movon_adas_recv_data, 0x00, sizeof(movon_adas_recv_data));

        movon_adas__mgr_mutex_lock();
        uart_ret =  mds_api_uart_read(movon_adas_fd, (void*)movon_adas_recv_data,  sizeof(movon_adas_recv_data), MOVON_ADAS_UART_READ_THREAD_TIMEOUT);
        movon_adas__mgr_mutex_unlock();

        // printf(" -------------- uart get [%d]\r\n", uart_ret);
        if ( uart_ret > 0 )
        {
            //printf("---------------------------------------------------------------------\r\n");
            //debug_hexdump_buff(movon_adas_recv_data, uart_ret);
            //printf("---------------------------------------------------------------------\r\n");
            enQueue_size(movon_adas_recv_data, uart_ret);
        }

        while(1)
        {
            ADAS_EVT_DATA_T evt_data = {0,};
            memset(&data_frame, 0x00, sizeof(data_frame));

            if ( get_dataframe_from_Queue(&data_frame) == QUEUE_RET_FAIL )
            {
                read_invaild_cnt++;

                if ( read_invaild_cnt > 10)
                {
                    evt_data.evt_code = eADAS_EVT__INVALID;

                    if ( g_movon_adas_bmsg_proc != NULL )
                        g_movon_adas_bmsg_proc(&evt_data);

                    read_invaild_cnt = 0;

                }
                break;
            }

            movon_adas__set_cur_data(&data_frame);

            //dbg_print_movon_data(&data_frame);
            read_invaild_cnt = 0;

            evt_data.evt_code = eADAS_EVT__NONE;

            // adas data parsing : multi event support..
            while (1)
            {
                movon_get_evt_data(&evt_data, &data_frame);

                // event proc
                if ( g_movon_adas_bmsg_proc != NULL )
                    g_movon_adas_bmsg_proc(&evt_data);

                if ( evt_data.evt_code == eADAS_EVT__NONE)
                    break;
            }


            //printf("---------------------------------------------------------------------\r\n");
            //printf("[movon_adas read thread] read success [%d] \r\n", uart_ret);
            //debug_hexdump_buff(movon_adas_recv_data, uart_ret);
            //printf("---------------------------------------------------------------------\r\n");
            
        }
    


#if 0
        read_fail_cnt = 0;
        to_line_read = strlen(movon_adas_recv_data);
        p_tmp_buff = movon_adas_recv_data;

        while( to_line_read > 0 )
        {
            char read_line_buff[MAX_MOVON_ADAS_RET_BUFF_SIZE] = {0,};
            int  read_line_len = 0;

            // 모든데이터는 1 line 이 한개 데이터다.
            // 여러개 line 이 한번에 들어올때가 있어서 버퍼를 읽으면서 proc 에 던진다.
            if ( ( read_line_len = mds_api_read_line(p_tmp_buff, to_line_read, read_line_buff, sizeof(read_line_buff)) ) <= 0 )
                break;
                 
            to_line_read -= read_line_len;
            p_tmp_buff += read_line_len;

            if ( strstr(read_line_buff, MOVON_ADAS_UART_BROAD_CAST_CHK_MSG) != NULL)
            {
                char tmp_argv[20][128];
                int tmp_argc = 0;
                memset(&tmp_argv, 0x00, sizeof(tmp_argv));

                tmp_argc = movon_adas__tool_device_argument(read_line_buff, strlen(read_line_buff), tmp_argv);

                if ( tmp_argc > 0 )
                    g_movon_adas_bmsg_proc(tmp_argc, tmp_argv);
            }
            else
            {
                if ( g_movon_adas_bmsg_proc != NULL )
                    g_movon_adas_bmsg_proc(0, NULL);
                
                // printf("[movon_adas read thread] read fail case 3 do anything...\r\n");
            }
        }
#endif
    }

}

int movon_adas__mgr_thread_start()
{
    pthread_attr_t attr;
    _g_run_movon_adas_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_movon_adas_thread == NULL )
        pthread_create(&tid_movon_adas_thread, &attr, movon_adas__mgr_read_thread, NULL);

    return 0;
}

int movon_adas__mgr_thread_stop()
{
    _g_run_movon_adas_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int movon_adas__mgr_init(char *dev_name, int baud_rate, int (*p_bmsg_proc)(ADAS_EVT_DATA_T* evt_data))
{
    movon_adas__uart_deinit();

    movon_adas__uart_set_dev(dev_name);
    movon_adas__uart_set_baudrate(baud_rate);

    g_movon_adas_bmsg_proc = p_bmsg_proc;

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, movon_adas 는 이미 계속 메시지를 뿌리고있기 때문.
    movon_adas__cmd_broadcast_msg_stop();
    movon_adas__mgr_thread_start();

    return 0;
}


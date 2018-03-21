
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
#include <base/sender.h>

#include "netcom.h"
#include "sup_rfid_tools.h"
#include "sup_rfid_cmd.h"

#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL

pthread_t tid_sup_rfid_thread = NULL;

static int _g_run_sup_rfid_thread_run = 0;
static pthread_mutex_t sup_rfid_mutex = PTHREAD_MUTEX_INITIALIZER;

int sup_rfid_bmsg_proc(SUP_RFID_DATA_T* evt_data);


void sup_rfid__mgr_mutex_lock()
{
    pthread_mutex_lock(&sup_rfid_mutex);
}

void sup_rfid__mgr_mutex_unlock()
{
    pthread_mutex_unlock(&sup_rfid_mutex);
}


// 브로드케스트 명령어획득을 위한 쓰래드
void sup_rfid__mgr_read_thread(void)
{
    int  sup_rfid_fd = -1;
    int  uart_ret = 0;
    char sup_rfid_recv_data[MAX_SUP_RFID_RET_BUFF_SIZE] ={0,};

    static int read_fail_cnt = 0;
    static int read_invaild_cnt = 0;
    //sup_rfid__cmd_broadcast_msg_start();

    while(_g_run_sup_rfid_thread_run)
    {
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_READ_SUP_RFID_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[sup_rfid read thread] run..\r\n");
        if ( sup_rfid__uart_chk() != SUP_RFID_RET_SUCCESS )
        {
            printf("[sup_rfid read thread] sup_rfid init fail..\r\n");
            sleep(1);
            continue;
        }

        sup_rfid_fd = sup_rfid__uart_get_fd();

        if ( sup_rfid_fd == SUP_RFID_INVAILD_FD )
        {
            printf("[sup_rfid read thread] get sup_rfid fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(sup_rfid_recv_data, 0x00, sizeof(sup_rfid_recv_data));

        sup_rfid__mgr_mutex_lock();
        mds_api_uart_write(sup_rfid_fd, "at\r\n", strlen("at\r\n"));
        uart_ret =  mds_api_uart_read(sup_rfid_fd, (void*)sup_rfid_recv_data,  sizeof(sup_rfid_recv_data), SUP_RFID_UART_READ_THREAD_TIMEOUT);
        sup_rfid__mgr_mutex_unlock();

        // cr, lr 을 제거한다.
        //uart_ret = mds_api_remove_cr(sup_rfid_recv_data, tmp_recv_data, 512);

        if ( uart_ret <= 0 )
        {
            SUP_RFID_DATA_T evt_data = {0,};

            evt_data.evt_ret = SUP_RFID_RET__ERR_UART_TIMEOUT;
            sup_rfid_bmsg_proc(&evt_data);

            printf("[sup_rfid read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

        printf("---------------------------------------------------------------------\r\n");
        //printf("[sup_rfid read thread] read success [%s] \r\n", sup_rfid_recv_data);
        mds_api_debug_hexdump_buff(sup_rfid_recv_data, uart_ret);
        printf("---------------------------------------------------------------------\r\n");
        
        sup_rfid_q_enQueue_size(sup_rfid_recv_data, uart_ret);

        while(1)
        {
            SUP_RFID_DATA_FRAME_T data_frame = {0,};
            SUP_RFID_DATA_T evt_data = {0,};
            
            memset(&data_frame, 0x00, sizeof(data_frame));

            if ( get_sup_rfid_dataframe_from_Queue(&data_frame) == SUP_RFID_QUEUE_RET_FAIL )
            {
                read_invaild_cnt++;

                if ( read_invaild_cnt > 10)
                {
                    evt_data.evt_ret = SUP_RFID_RET__ERR_DATA_INVALID;
                    sup_rfid_bmsg_proc(&evt_data);

                    read_invaild_cnt = 0;

                }
                break;
            }
            
            
            read_invaild_cnt = 0;

            evt_data.evt_ret = SUP_RFID_RET__SUCCESS;
            strcpy(evt_data.rfid_str, data_frame.rfid_data);
            sup_rfid_bmsg_proc(&evt_data);
        }
        
    }

}

int sup_rfid__mgr_thread_start()
{
    pthread_attr_t attr;
    _g_run_sup_rfid_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_sup_rfid_thread == NULL )
        pthread_create(&tid_sup_rfid_thread, &attr, sup_rfid__mgr_read_thread, NULL);

    return 0;
}

int sup_rfid__mgr_thread_stop()
{
    _g_run_sup_rfid_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int sup_rfid__mgr_init(char* dev_name, int baud_rate)
{
    sup_rfid__uart_deinit();

    sup_rfid__uart_set_dev(dev_name);
    sup_rfid__uart_set_baudrate(baud_rate);

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, sup_rfid 는 이미 계속 메시지를 뿌리고있기 때문.
    //sup_rfid__cmd_broadcast_msg_stop();
    sup_rfid__mgr_thread_start();

    return 0;
}

// -----------------------------------------------------------------
// queue tools
// -----------------------------------------------------------------

#define FULL_QUEUE_DELETE_AND_INSERT

char g_sup_rfid_q_items[SUP_RFID_QUEUE_SIZE];

int sup_rfid_q_front = -1, sup_rfid_q_rear =-1;

static int _sup_rfid_q_isFull()
{
    if( (sup_rfid_q_front == sup_rfid_q_rear + 1) || (sup_rfid_q_front == 0 && sup_rfid_q_rear == SUP_RFID_QUEUE_SIZE-1) ) 
		return 1;
	
    return 0;
}

static int _sup_rfid_q_isEmpty()
{
    if( sup_rfid_q_front == -1 ) 
		return 1;
    return 0;
}

static int _sup_rfid_q_cntQueue()
{
	int cnt = 1;
	int i = 0;
	for( i = sup_rfid_q_front; i!=sup_rfid_q_rear; i=(i+1)%SUP_RFID_QUEUE_SIZE) 
		cnt++;
	
	return cnt;
}

int sup_rfid_q_deQueue(char* data)
{
    char element;
	
    if( _sup_rfid_q_isEmpty() ) 
	{
        //printf("\n Queue is empty !! \n");
        return SUP_RFID_QUEUE_RET_FAIL;
    }
		
	element = g_sup_rfid_q_items[sup_rfid_q_front];
	if (sup_rfid_q_front == sup_rfid_q_rear) {
		sup_rfid_q_front = -1;
		sup_rfid_q_rear = -1;
	} /* Q has only one element, so we reset the queue after dequeing it. ? */
	else {
		sup_rfid_q_front = (sup_rfid_q_front + 1) % SUP_RFID_QUEUE_SIZE;
		
	}
	//printf("\n Deleted element -> %d \n", element);
    *data = element;
    return SUP_RFID_QUEUE_RET_SUCCESS;
}


void sup_rfid_q_enQueue(char element)
{
    char tmp_element;
    if( _sup_rfid_q_isFull() ) 
	{
		//printf("\n Queue is full!! \n");
#ifdef FULL_QUEUE_DELETE_AND_INSERT
		sup_rfid_q_deQueue(&tmp_element); // fource delete
#else
		return -1;
#endif
	}
	
	// FORCE insert..
	if (sup_rfid_q_front == -1) 
		sup_rfid_q_front = 0;
	
	sup_rfid_q_rear = (sup_rfid_q_rear + 1) % SUP_RFID_QUEUE_SIZE;
	g_sup_rfid_q_items[sup_rfid_q_rear] = element;
	//printf("\n Inserted -> %d", element);
    
}

void sup_rfid_q_enQueue_size(char* element, int size)
{
    int i = 0;

    for ( i = 0 ; i < size ; i ++ )
        sup_rfid_q_enQueue(element[i]);
}


int get_sup_rfid_dataframe_from_Queue(SUP_RFID_DATA_FRAME_T* data)
{
    char tmp_buff[512] = {0,};
    
    char data_buff[512] = {0,};
    char data_one;
    int  data_idx = 0;

    int found_data_prefix = 0 ;
    int found_data_sufix = 0 ;

    // found prefix ...
    while(1)
    {
        if ( sup_rfid_q_deQueue(&data_one) == SUP_RFID_QUEUE_RET_FAIL )
            break;

        if ( data_one == SUP_RFID_FRAME__PREFIX )
        {
            found_data_prefix = 1;
            break;
        }
    }

    if ( found_data_prefix == 0 )
        return SUP_RFID_QUEUE_RET_FAIL;

    data_buff[data_idx++] = SUP_RFID_FRAME__PREFIX;
    
    // found prefix ...
    while(1)
    {
        if ( sup_rfid_q_deQueue(&data_one) == SUP_RFID_QUEUE_RET_FAIL )
            break;
        
        if ( data_one == SUP_RFID_FRAME__SUFIX )
        {
            found_data_sufix = 1;
            break;
        }
        
        data_buff[data_idx++] = data_one;
    }

    // found data..
    if ( ( found_data_sufix != 1 ) || ( found_data_sufix != 1 ) )
        return SUP_RFID_QUEUE_RET_FAIL;

    mds_api_remove_etc_char(data_buff, tmp_buff, sizeof(tmp_buff));

    sprintf(data->rfid_data,"%s", tmp_buff);


    return SUP_RFID_QUEUE_RET_SUCCESS;
    
}





int sup_rfid_bmsg_proc(SUP_RFID_DATA_T* evt_data)
{
 	time_t t;
    struct tm *lt;
    char   time_str[26];

    RFID_BOARDING_MGR_T boarding_list;
    int total_boarding_list = 0;
    memset(&boarding_list, 0x00, sizeof(boarding_list));
    
    // check err stat...
    if (evt_data->evt_ret != SUP_RFID_RET__SUCCESS )
        return 0;

    printf("[SUP RFID] rfid_str [%s]\r\n", evt_data->rfid_str);

    // get time
    t = time(NULL);
    lt = localtime(&t);

    //20170901130801
    sprintf(time_str, "%02d%02d%02d%02d%02d%02d", lt->tm_year+1900,
                    lt->tm_mon+1,
                    lt->tm_mday,
                    lt->tm_hour,
                    lt->tm_min,
                    lt->tm_sec);
    
    strcpy(boarding_list.boarding_info[0].rfid_uid, evt_data->rfid_str);
    boarding_list.boarding_info[0].boarding = 0;
    strcpy(boarding_list.boarding_info[0].date,time_str);
    boarding_list.boarding_info[0].chk_result = 1;
    
    LOGI(LOG_TARGET," >> boarding_list.boarding_info[0].rfid_uid [%s]\r\n", boarding_list.boarding_info[0].rfid_uid );
    LOGI(LOG_TARGET," >> boarding_list.boarding_info[0].boarding [%d]\r\n", boarding_list.boarding_info[0].boarding );
    LOGI(LOG_TARGET," >> boarding_list.boarding_info[0].date [%s]\r\n", boarding_list.boarding_info[0].date );
    LOGI(LOG_TARGET," >> boarding_list.boarding_info[0].chk_result [%d]\r\n", boarding_list.boarding_info[0].chk_result );

    boarding_list.rfid_boarding_idx = 1;
    sender_add_data_to_buffer(PACKET_TYPE_HTTP_SET_BOARDING_LIST, &boarding_list, ePIPE_2);

    sleep(20);
    return 0;
}



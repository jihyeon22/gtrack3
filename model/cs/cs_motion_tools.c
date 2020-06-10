<<<<<<< HEAD

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
#include "cs_motion_tools.h"
#include "cs_motion_cmd.h"

#include <board/board_system.h>
#include <board/power.h>

#include <mdt800/packet.h>


#define LOG_TARGET eSVC_MODEL

pthread_t tid_cs_motion_thread = NULL;

static int _g_run_cs_motion_thread_run = 0;
static pthread_mutex_t cs_motion_mutex = PTHREAD_MUTEX_INITIALIZER;

int cs_motion_bmsg_proc(CS_MOTION_DATA_T* evt_data);


void cs_motion__mgr_mutex_lock()
{
    pthread_mutex_lock(&cs_motion_mutex);
}

void cs_motion__mgr_mutex_unlock()
{
    pthread_mutex_unlock(&cs_motion_mutex);
}


// 브로드케스트 명령어획득을 위한 쓰래드
void cs_motion__mgr_read_thread(void)
{
    int  cs_motion_fd = -1;
    int  uart_ret = 0;
    char cs_motion_recv_data[MAX_CS_MOTION_RET_BUFF_SIZE] ={0,};

    static int read_fail_cnt = 0;
    static int read_invaild_cnt = 0;
    //cs_motion__cmd_broadcast_msg_start();

    while(_g_run_cs_motion_thread_run)
    {
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_READ_CS_MOTION_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[cs_motion read thread] run..\r\n");
        if ( cs_motion__uart_chk() != CS_MOTION_RET_SUCCESS )
        {
            printf("[cs_motion read thread] cs_motion init fail..\r\n");
            sleep(1);
            continue;
        }

        cs_motion_fd = cs_motion__uart_get_fd();

        if ( cs_motion_fd == CS_MOTION_INVAILD_FD )
        {
            printf("[cs_motion read thread] get cs_motion fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(cs_motion_recv_data, 0x00, sizeof(cs_motion_recv_data));

        cs_motion__mgr_mutex_lock();
        mds_api_uart_write(cs_motion_fd, "at\r\n", strlen("at\r\n"));
        uart_ret =  mds_api_uart_read(cs_motion_fd, (void*)cs_motion_recv_data,  sizeof(cs_motion_recv_data), CS_MOTION_UART_READ_THREAD_TIMEOUT);
        cs_motion__mgr_mutex_unlock();

        // cr, lr 을 제거한다.
        //uart_ret = mds_api_remove_cr(cs_motion_recv_data, tmp_recv_data, 512);

        if ( uart_ret <= 0 )
        {
            CS_MOTION_DATA_T evt_data = {0,};

            evt_data.evt_ret = CS_MOTION_RET__ERR_UART_TIMEOUT;
            cs_motion_bmsg_proc(&evt_data);

            printf("[cs_motion read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

        printf("---------------------------------------------------------------------\r\n");
        printf("[cs_motion read thread] read success [%s] \r\n", cs_motion_recv_data);
        mds_api_debug_hexdump_buff(cs_motion_recv_data, uart_ret);
        printf("---------------------------------------------------------------------\r\n");
        
        cs_motion_q_enQueue_size(cs_motion_recv_data, uart_ret);

        while(1)
        {
            CS_MOTION_DATA_FRAME_T data_frame;
            CS_MOTION_DATA_T evt_data;
            
            memset(&data_frame, 0x00, sizeof(data_frame));
            memset(&evt_data, 0x00, sizeof(evt_data));

            if ( get_cs_motion_dataframe_from_Queue(&data_frame) == CS_MOTION_QUEUE_RET_FAIL )
            {
                read_invaild_cnt++;

                if ( read_invaild_cnt > 10)
                {
                    evt_data.evt_ret = CS_MOTION_RET__ERR_DATA_INVALID;
                    cs_motion_bmsg_proc(&evt_data);

                    read_invaild_cnt = 0;

                }
                break;
            }
            
            
            read_invaild_cnt = 0;

            evt_data.evt_ret = CS_MOTION_RET__SUCCESS;
            strcpy(evt_data.sesnor_data, data_frame.sesnor_data);
            cs_motion_bmsg_proc(&evt_data);
        }
        
    }

}

int cs_motion__mgr_thread_start()
{
    pthread_attr_t attr;
    _g_run_cs_motion_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_cs_motion_thread == NULL )
        pthread_create(&tid_cs_motion_thread, &attr, cs_motion__mgr_read_thread, NULL);

    return 0;
}

int cs_motion__mgr_thread_stop()
{
    _g_run_cs_motion_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int cs_motion__mgr_init(char* dev_name, int baud_rate)
{
    cs_motion__uart_deinit();

    cs_motion__uart_set_dev(dev_name);
    cs_motion__uart_set_baudrate(baud_rate);

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, cs_motion 는 이미 계속 메시지를 뿌리고있기 때문.
    //cs_motion__cmd_broadcast_msg_stop();
    cs_motion__mgr_thread_start();

    return 0;
}

// -----------------------------------------------------------------
// queue tools
// -----------------------------------------------------------------

#define FULL_QUEUE_DELETE_AND_INSERT

char g_cs_motion_q_items[CS_MOTION_QUEUE_SIZE];

int cs_motion_q_front = -1, cs_motion_q_rear =-1;

static int _cs_motion_q_isFull()
{
    if( (cs_motion_q_front == cs_motion_q_rear + 1) || (cs_motion_q_front == 0 && cs_motion_q_rear == CS_MOTION_QUEUE_SIZE-1) ) 
		return 1;
	
    return 0;
}

static int _cs_motion_q_isEmpty()
{
    if( cs_motion_q_front == -1 ) 
		return 1;
    return 0;
}

static int _cs_motion_q_cntQueue()
{
	int cnt = 1;
	int i = 0;
	for( i = cs_motion_q_front; i!=cs_motion_q_rear; i=(i+1)%CS_MOTION_QUEUE_SIZE) 
		cnt++;
	
	return cnt;
}

int cs_motion_q_deQueue(char* data)
{
    char element;
	
    if( _cs_motion_q_isEmpty() ) 
	{
        //printf("\n Queue is empty !! \n");
        return CS_MOTION_QUEUE_RET_FAIL;
    }
		
	element = g_cs_motion_q_items[cs_motion_q_front];
	if (cs_motion_q_front == cs_motion_q_rear) {
		cs_motion_q_front = -1;
		cs_motion_q_rear = -1;
	} /* Q has only one element, so we reset the queue after dequeing it. ? */
	else {
		cs_motion_q_front = (cs_motion_q_front + 1) % CS_MOTION_QUEUE_SIZE;
		
	}
	//printf("\n Deleted element -> %d \n", element);
    *data = element;
    return CS_MOTION_QUEUE_RET_SUCCESS;
}


void cs_motion_q_enQueue(char element)
{
    char tmp_element;
    if( _cs_motion_q_isFull() ) 
	{
		//printf("\n Queue is full!! \n");
#ifdef FULL_QUEUE_DELETE_AND_INSERT
		cs_motion_q_deQueue(&tmp_element); // fource delete
#else
		return -1;
#endif
	}
	
	// FORCE insert..
	if (cs_motion_q_front == -1) 
		cs_motion_q_front = 0;
	
	cs_motion_q_rear = (cs_motion_q_rear + 1) % CS_MOTION_QUEUE_SIZE;
	g_cs_motion_q_items[cs_motion_q_rear] = element;
	//printf("\n Inserted -> %d", element);
    
}

void cs_motion_q_enQueue_size(char* element, int size)
{
    int i = 0;

    for ( i = 0 ; i < size ; i ++ )
        cs_motion_q_enQueue(element[i]);
}


int get_cs_motion_dataframe_from_Queue(CS_MOTION_DATA_FRAME_T* data)
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
        if ( cs_motion_q_deQueue(&data_one) == CS_MOTION_QUEUE_RET_FAIL )
            break;

        if ( data_one == CS_MOTION_FRAME__PREFIX )
        {
            found_data_prefix = 1;
            break;
        }
    }

    if ( found_data_prefix == 0 )
        return CS_MOTION_QUEUE_RET_FAIL;

    data_buff[data_idx++] = CS_MOTION_FRAME__PREFIX;
    
    // found prefix ...
    while(1)
    {
        if ( cs_motion_q_deQueue(&data_one) == CS_MOTION_QUEUE_RET_FAIL )
            break;
        
        if ( ( data_one == CS_MOTION_FRAME__SUFIX_1 ) || ( data_one == CS_MOTION_FRAME__SUFIX_2 ))
        {
            found_data_sufix = 1;
            data_buff[data_idx++] = data_one;
            break;
        }
        
        data_buff[data_idx++] = data_one;
    }

    // found data..
    if ( ( found_data_prefix != 1 ) || ( found_data_sufix != 1 ) )
        return CS_MOTION_QUEUE_RET_FAIL;

    mds_api_remove_etc_char(data_buff, tmp_buff, sizeof(tmp_buff));

    sprintf(data->sesnor_data,"%s", tmp_buff);

    return CS_MOTION_QUEUE_RET_SUCCESS;
    
}





int cs_motion_bmsg_proc(CS_MOTION_DATA_T* evt_data)
{
// 	time_t t;
//    struct tm *lt;
//    char   time_str[26];

    if (evt_data->evt_ret != CS_MOTION_RET__SUCCESS )
    {
         printf("[SENSOR MOTION] do nothing [%d]\r\n", evt_data->evt_ret);
        return 0;
    }

    LOGI(LOG_TARGET, "[SENSOR MOTION] GET DATA :: [%s]\r\n", evt_data->sesnor_data);

    if ( power_get_ignition_status() == POWER_IGNITION_ON )
    {
        LOGE(LOG_TARGET, "[SENSOR MOTION] KEY ON .. DO NOTHING..\r\n");
        return 0;
    }

    if (strcmp(evt_data->sesnor_data, "KEY:01") == 0 )
    {
        LOGT(LOG_TARGET, "[SENSOR MOTION] SEND KEY EVT [%d]\r\n", eBUTTON_NUM1_EVT);
        sender_add_data_to_buffer(eBUTTON_NUM1_EVT, NULL, ePIPE_1);
    }
    else if (strcmp(evt_data->sesnor_data, "KEY:02") == 0 )
    {
        LOGT(LOG_TARGET, "[SENSOR MOTION] SEND KEY EVT [%d]\r\n", eBUTTON_NUM2_EVT);
        sender_add_data_to_buffer(eBUTTON_NUM2_EVT, NULL, ePIPE_1);
    }
    

    return 0;
}


=======

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
#include "cs_motion_tools.h"
#include "cs_motion_cmd.h"

#include <board/board_system.h>
#include <board/power.h>

#include <mdt800/packet.h>


#define LOG_TARGET eSVC_MODEL

pthread_t tid_cs_motion_thread = NULL;

static int _g_run_cs_motion_thread_run = 0;
static pthread_mutex_t cs_motion_mutex = PTHREAD_MUTEX_INITIALIZER;

int cs_motion_bmsg_proc(CS_MOTION_DATA_T* evt_data);


void cs_motion__mgr_mutex_lock()
{
    pthread_mutex_lock(&cs_motion_mutex);
}

void cs_motion__mgr_mutex_unlock()
{
    pthread_mutex_unlock(&cs_motion_mutex);
}


// 브로드케스트 명령어획득을 위한 쓰래드
void cs_motion__mgr_read_thread(void)
{
    int  cs_motion_fd = -1;
    int  uart_ret = 0;
    char cs_motion_recv_data[MAX_CS_MOTION_RET_BUFF_SIZE] ={0,};

    static int read_fail_cnt = 0;
    static int read_invaild_cnt = 0;
    //cs_motion__cmd_broadcast_msg_start();

    while(_g_run_cs_motion_thread_run)
    {
        usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_READ_CS_MOTION_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[cs_motion read thread] run..\r\n");
        if ( cs_motion__uart_chk() != CS_MOTION_RET_SUCCESS )
        {
            printf("[cs_motion read thread] cs_motion init fail..\r\n");
            sleep(1);
            continue;
        }

        cs_motion_fd = cs_motion__uart_get_fd();

        if ( cs_motion_fd == CS_MOTION_INVAILD_FD )
        {
            printf("[cs_motion read thread] get cs_motion fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(cs_motion_recv_data, 0x00, sizeof(cs_motion_recv_data));

        cs_motion__mgr_mutex_lock();
        mds_api_uart_write(cs_motion_fd, "at\r\n", strlen("at\r\n"));
        uart_ret =  mds_api_uart_read(cs_motion_fd, (void*)cs_motion_recv_data,  sizeof(cs_motion_recv_data), CS_MOTION_UART_READ_THREAD_TIMEOUT);
        cs_motion__mgr_mutex_unlock();

        // cr, lr 을 제거한다.
        //uart_ret = mds_api_remove_cr(cs_motion_recv_data, tmp_recv_data, 512);

        if ( uart_ret <= 0 )
        {
            CS_MOTION_DATA_T evt_data = {0,};

            evt_data.evt_ret = CS_MOTION_RET__ERR_UART_TIMEOUT;
            cs_motion_bmsg_proc(&evt_data);

            printf("[cs_motion read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

        printf("---------------------------------------------------------------------\r\n");
        printf("[cs_motion read thread] read success [%s] \r\n", cs_motion_recv_data);
        mds_api_debug_hexdump_buff(cs_motion_recv_data, uart_ret);
        printf("---------------------------------------------------------------------\r\n");
        
        cs_motion_q_enQueue_size(cs_motion_recv_data, uart_ret);

        while(1)
        {
            CS_MOTION_DATA_FRAME_T data_frame;
            CS_MOTION_DATA_T evt_data;
            
            memset(&data_frame, 0x00, sizeof(data_frame));
            memset(&evt_data, 0x00, sizeof(evt_data));

            if ( get_cs_motion_dataframe_from_Queue(&data_frame) == CS_MOTION_QUEUE_RET_FAIL )
            {
                read_invaild_cnt++;

                if ( read_invaild_cnt > 10)
                {
                    evt_data.evt_ret = CS_MOTION_RET__ERR_DATA_INVALID;
                    cs_motion_bmsg_proc(&evt_data);

                    read_invaild_cnt = 0;

                }
                break;
            }
            
            
            read_invaild_cnt = 0;

            evt_data.evt_ret = CS_MOTION_RET__SUCCESS;
            strcpy(evt_data.sesnor_data, data_frame.sesnor_data);
            cs_motion_bmsg_proc(&evt_data);
        }
        
    }

}

int cs_motion__mgr_thread_start()
{
    pthread_attr_t attr;
    _g_run_cs_motion_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_cs_motion_thread == NULL )
        pthread_create(&tid_cs_motion_thread, &attr, cs_motion__mgr_read_thread, NULL);

    return 0;
}

int cs_motion__mgr_thread_stop()
{
    _g_run_cs_motion_thread_run = 0;
    return 0;
}

// int (*p_bmsg_proc)(const int argc, const char* argv[])
int cs_motion__mgr_init(char* dev_name, int baud_rate)
{
    cs_motion__uart_deinit();

    cs_motion__uart_set_dev(dev_name);
    cs_motion__uart_set_baudrate(baud_rate);

    // 제일먼저 브로드캐스트메시지를 중지시킨다.
    // 만약, 비정상적으로 리셋이 되던가하면, cs_motion 는 이미 계속 메시지를 뿌리고있기 때문.
    //cs_motion__cmd_broadcast_msg_stop();
    cs_motion__mgr_thread_start();

    return 0;
}

// -----------------------------------------------------------------
// queue tools
// -----------------------------------------------------------------

#define FULL_QUEUE_DELETE_AND_INSERT

char g_cs_motion_q_items[CS_MOTION_QUEUE_SIZE];

int cs_motion_q_front = -1, cs_motion_q_rear =-1;

static int _cs_motion_q_isFull()
{
    if( (cs_motion_q_front == cs_motion_q_rear + 1) || (cs_motion_q_front == 0 && cs_motion_q_rear == CS_MOTION_QUEUE_SIZE-1) ) 
		return 1;
	
    return 0;
}

static int _cs_motion_q_isEmpty()
{
    if( cs_motion_q_front == -1 ) 
		return 1;
    return 0;
}

static int _cs_motion_q_cntQueue()
{
	int cnt = 1;
	int i = 0;
	for( i = cs_motion_q_front; i!=cs_motion_q_rear; i=(i+1)%CS_MOTION_QUEUE_SIZE) 
		cnt++;
	
	return cnt;
}

int cs_motion_q_deQueue(char* data)
{
    char element;
	
    if( _cs_motion_q_isEmpty() ) 
	{
        //printf("\n Queue is empty !! \n");
        return CS_MOTION_QUEUE_RET_FAIL;
    }
		
	element = g_cs_motion_q_items[cs_motion_q_front];
	if (cs_motion_q_front == cs_motion_q_rear) {
		cs_motion_q_front = -1;
		cs_motion_q_rear = -1;
	} /* Q has only one element, so we reset the queue after dequeing it. ? */
	else {
		cs_motion_q_front = (cs_motion_q_front + 1) % CS_MOTION_QUEUE_SIZE;
		
	}
	//printf("\n Deleted element -> %d \n", element);
    *data = element;
    return CS_MOTION_QUEUE_RET_SUCCESS;
}


void cs_motion_q_enQueue(char element)
{
    char tmp_element;
    if( _cs_motion_q_isFull() ) 
	{
		//printf("\n Queue is full!! \n");
#ifdef FULL_QUEUE_DELETE_AND_INSERT
		cs_motion_q_deQueue(&tmp_element); // fource delete
#else
		return -1;
#endif
	}
	
	// FORCE insert..
	if (cs_motion_q_front == -1) 
		cs_motion_q_front = 0;
	
	cs_motion_q_rear = (cs_motion_q_rear + 1) % CS_MOTION_QUEUE_SIZE;
	g_cs_motion_q_items[cs_motion_q_rear] = element;
	//printf("\n Inserted -> %d", element);
    
}

void cs_motion_q_enQueue_size(char* element, int size)
{
    int i = 0;

    for ( i = 0 ; i < size ; i ++ )
        cs_motion_q_enQueue(element[i]);
}


int get_cs_motion_dataframe_from_Queue(CS_MOTION_DATA_FRAME_T* data)
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
        if ( cs_motion_q_deQueue(&data_one) == CS_MOTION_QUEUE_RET_FAIL )
            break;

        if ( data_one == CS_MOTION_FRAME__PREFIX )
        {
            found_data_prefix = 1;
            break;
        }
    }

    if ( found_data_prefix == 0 )
        return CS_MOTION_QUEUE_RET_FAIL;

    data_buff[data_idx++] = CS_MOTION_FRAME__PREFIX;
    
    // found prefix ...
    while(1)
    {
        if ( cs_motion_q_deQueue(&data_one) == CS_MOTION_QUEUE_RET_FAIL )
            break;
        
        if ( ( data_one == CS_MOTION_FRAME__SUFIX_1 ) || ( data_one == CS_MOTION_FRAME__SUFIX_2 ))
        {
            found_data_sufix = 1;
            data_buff[data_idx++] = data_one;
            break;
        }
        
        data_buff[data_idx++] = data_one;
    }

    // found data..
    if ( ( found_data_prefix != 1 ) || ( found_data_sufix != 1 ) )
        return CS_MOTION_QUEUE_RET_FAIL;

    mds_api_remove_etc_char(data_buff, tmp_buff, sizeof(tmp_buff));

    sprintf(data->sesnor_data,"%s", tmp_buff);

    return CS_MOTION_QUEUE_RET_SUCCESS;
    
}





int cs_motion_bmsg_proc(CS_MOTION_DATA_T* evt_data)
{
// 	time_t t;
//    struct tm *lt;
//    char   time_str[26];

    if (evt_data->evt_ret != CS_MOTION_RET__SUCCESS )
    {
         printf("[SENSOR MOTION] do nothing [%d]\r\n", evt_data->evt_ret);
        return 0;
    }

    LOGI(LOG_TARGET, "[SENSOR MOTION] GET DATA :: [%s]\r\n", evt_data->sesnor_data);

    if ( power_get_ignition_status() == POWER_IGNITION_ON )
    {
        LOGE(LOG_TARGET, "[SENSOR MOTION] KEY ON .. DO NOTHING..\r\n");
        return 0;
    }

    if (strcmp(evt_data->sesnor_data, "KEY:01") == 0 )
    {
        LOGT(LOG_TARGET, "[SENSOR MOTION] SEND KEY EVT [%d]\r\n", eBUTTON_NUM1_EVT);
        sender_add_data_to_buffer(eBUTTON_NUM1_EVT, NULL, ePIPE_1);
    }
    else if (strcmp(evt_data->sesnor_data, "KEY:02") == 0 )
    {
        LOGT(LOG_TARGET, "[SENSOR MOTION] SEND KEY EVT [%d]\r\n", eBUTTON_NUM2_EVT);
        sender_add_data_to_buffer(eBUTTON_NUM2_EVT, NULL, ePIPE_1);
    }
    

    return 0;
}


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

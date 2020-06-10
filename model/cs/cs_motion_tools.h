<<<<<<< HEAD
#ifndef __CS_MOTION_TOOLS_H__
#define __CS_MOTION_TOOLS_H__


void cs_motion__mgr_mutex_lock();
void cs_motion__mgr_mutex_unlock();

#define CS_MOTION_INVAILD_FD              -44
#define CS_MOTION_UART_BUFF_SIZE          1024
#define CS_MOTION_UART_READ_TIMEOUT_SEC   3
#define CS_MOTION_UART_WRITE_CMD_SIZE     512
#define CS_MOTION_UART_INIT_TRY_CNT       3

#define CS_MOTION_UART_READ_THREAD_TIMEOUT 1

typedef struct
{
    int evt_ret;
    char sesnor_data[32];
    // int break_val; // remove spec
}CS_MOTION_DATA_T;


typedef struct
{
    char sesnor_data[64];
    // int break_val; // remove spec
}CS_MOTION_DATA_FRAME_T;



#define CS_MOTION_FRAME__PREFIX   'K'
#define CS_MOTION_FRAME__SUFIX_1    '1'
#define CS_MOTION_FRAME__SUFIX_2    '2'

#define CS_MOTION_QUEUE_RET_SUCCESS  1
#define CS_MOTION_QUEUE_RET_FAIL     0

#define CS_MOTION_QUEUE_SIZE 2048

#define CS_MOTION_RET__ERR_UART_TIMEOUT  -1
#define CS_MOTION_RET__ERR_DATA_INVALID  -2
#define CS_MOTION_RET__SUCCESS           1


int get_cs_motion_dataframe_from_Queue(CS_MOTION_DATA_FRAME_T* data);
void cs_motion_q_enQueue_size(char* element, int size);

#endif // __CS_MOTION_TOOLS_H__
=======
#ifndef __CS_MOTION_TOOLS_H__
#define __CS_MOTION_TOOLS_H__


void cs_motion__mgr_mutex_lock();
void cs_motion__mgr_mutex_unlock();

#define CS_MOTION_INVAILD_FD              -44
#define CS_MOTION_UART_BUFF_SIZE          1024
#define CS_MOTION_UART_READ_TIMEOUT_SEC   3
#define CS_MOTION_UART_WRITE_CMD_SIZE     512
#define CS_MOTION_UART_INIT_TRY_CNT       3

#define CS_MOTION_UART_READ_THREAD_TIMEOUT 1

typedef struct
{
    int evt_ret;
    char sesnor_data[32];
    // int break_val; // remove spec
}CS_MOTION_DATA_T;


typedef struct
{
    char sesnor_data[64];
    // int break_val; // remove spec
}CS_MOTION_DATA_FRAME_T;



#define CS_MOTION_FRAME__PREFIX   'K'
#define CS_MOTION_FRAME__SUFIX_1    '1'
#define CS_MOTION_FRAME__SUFIX_2    '2'

#define CS_MOTION_QUEUE_RET_SUCCESS  1
#define CS_MOTION_QUEUE_RET_FAIL     0

#define CS_MOTION_QUEUE_SIZE 2048

#define CS_MOTION_RET__ERR_UART_TIMEOUT  -1
#define CS_MOTION_RET__ERR_DATA_INVALID  -2
#define CS_MOTION_RET__SUCCESS           1


int get_cs_motion_dataframe_from_Queue(CS_MOTION_DATA_FRAME_T* data);
void cs_motion_q_enQueue_size(char* element, int size);

#endif // __CS_MOTION_TOOLS_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

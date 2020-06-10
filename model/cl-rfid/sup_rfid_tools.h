#ifndef __SUP_RFID_TOOLS_H__
#define __SUP_RFID_TOOLS_H__


void sup_rfid__mgr_mutex_lock();
void sup_rfid__mgr_mutex_unlock();

#define SUP_RFID_INVAILD_FD              -44
#define SUP_RFID_UART_BUFF_SIZE          1024
#define SUP_RFID_UART_READ_TIMEOUT_SEC   3
#define SUP_RFID_UART_WRITE_CMD_SIZE     512
#define SUP_RFID_UART_INIT_TRY_CNT       3

#define SUP_RFID_UART_READ_THREAD_TIMEOUT 1

typedef struct
{
    int evt_ret;
    char rfid_str[32];
    // int break_val; // remove spec
}SUP_RFID_DATA_T;


typedef struct
{
    char rfid_data[64];
    // int break_val; // remove spec
}SUP_RFID_DATA_FRAME_T;



#define SUP_RFID_FRAME__PREFIX   0x02
#define SUP_RFID_FRAME__SUFIX    0x03

#define SUP_RFID_QUEUE_RET_SUCCESS  1
#define SUP_RFID_QUEUE_RET_FAIL     0

#define SUP_RFID_QUEUE_SIZE 2048

#define SUP_RFID_RET__ERR_UART_TIMEOUT  -1
#define SUP_RFID_RET__ERR_DATA_INVALID  -2
#define SUP_RFID_RET__SUCCESS           1


int get_sup_rfid_dataframe_from_Queue(SUP_RFID_DATA_FRAME_T* data);
void sup_rfid_q_enQueue_size(char* element, int size);

#endif // __SUP_RFID_TOOLS_H__

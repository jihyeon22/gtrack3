#ifndef __KJTEC_RFID_PROTOCOL_H__
#define __KJTEC_RFID_PROTOCOL_H__

#define KJTEC_RFID_MAX_CHK_DEV_CNT  5

#define KJTEC_RFID_INVAILD_FD   0xAABBCC

#define KJTEC_RFID_RET_FAIL     -1
#define KJTEC_RFID_RET_SUCCESS  1

#define KJTEC_RFID_CMD_RESP_WAIT_TIME   3

#define KJTEC_RFID_UART_DEVNAME         "/dev/ttyHSL1"
#define KJTEC_RFID_UART_BAUDRATE        115200
#define KJTEC_RFID_UART_READ_TIMEOUT    1
#define KJTEC_RFID_UART_READ_RETRY_CNT  2


/*
static RFID_DEV_INFO_T                  g_rfid_dev_info;
static RIFD_PASSENGER_DATA_INFO_T       g_rfid_passenger_data_info;
static RIFD_CHK_READY_T                 g_rfid_chk_ready;
static RIFD_DATA_ALL_CLR_T              g_rfid_all_clr;
static RFID_SAVE_PASSENGER_DATA_T       g_rfid_save_passenger_data;

*/

typedef struct rfid_dev_info
{
    int cmd_result;
    char model_no[32];
    int total_passenger_cnt;
    char saved_timestamp[12+1]; // char??
}RFID_DEV_INFO_T;
#define RFID_CMD_ID_REQ__INIT_WAKEUP     0x20
#define RFID_CMD_ID_RESP___INIT_WAKEUP   0x21 // [KJ3400+,01222605817,2,]

typedef struct rfid_passenger_data_info
{
    int cmd_result;
    char saved_timestamp[12+1]; // char??
}RIFD_PASSENGER_DATA_INFO_T;
#define RFID_CMD_ID_RESP__PASSENGER_DATA_INFO           0x30 // [ 01222605817,2017/06/30.10:00:09,]

//#define RFID_CMD_RET__READY       KJTEC_RFID_RET_SUCCESS
//#define RFID_CMD_RET__NOT_READY   KJTEC_RFID_RET_FAIL
typedef struct rfid_chk_ready
{
    int cmd_result;
    int data_result; // char??
}RIFD_CHK_READY_T;
#define RFID_CMD_ID_REQ__CHK_READY       0x22
#define RFID_CMD_ID_RESP__CHK_READY      0x23 // [Ready]

typedef struct rfid_data_all_clr
{
    int cmd_result;
    int data_result; // char??
}RIFD_DATA_ALL_CLR_T;
#define RFID_CMD_ID_REQ__DATA_ALL_CLR    0x24
#define RFID_CMD_ID_RESP__DATA_ALL_CLR   0x25 // [Success]

typedef struct rfid_save_passenger_data
{
    int cmd_result;
    int data_result; // char??
}RFID_SAVE_PASSENGER_DATA_T;
#define RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA    0x31
#define RFID_CMD_ID_RESP__SAVE_PASSENGER_DATA   0x32 // [Data Result,1,]
#define RFID_CMD_PASSENGER_STR_MAX_LEN          253 // (256 -3)
#define RFID_CMD_PASSENGER_STR_PADDING          3 // (256 -3)
#define RFID_USER_INFO_FRAME__START             0x01
#define RFID_USER_INFO_FRAME__BODY              0x02
#define RFID_USER_INFO_FRAME__END               0x03
#define RFID_USER_INFO_FRAME__ONLY_ONE_PKT      0x04


#define RFID_CMD_ID_REQ__GET_PASSENGER_DATA             0x40
#define RFID_CMD_ID_RESP__GET_PASSENGER_DATA            0x41 
// +[0:[K0010W0000412001,1,20170704 085918,0],]
// +[4:[K0010W0000412001,1,20170705112223,0],]
#define GET_PASSENGER_DATA_PREFIX_STR   ":["
#define GET_PASSENGER_DATA_SUFFIX_STR   "],"
#define GET_PASSENGER_DATA_ARGUMENT_SPLIT_CHAR   ','
#define GET_PASSENGER_DATA_ARGUMENT_CNT   4
#define RFID_CMD_ID_REQ__GET_PASSENGER_DATA_SUCCESS     0x42 // ACK


// -------------------------------------------------------

int kjtec_rfid__dev_wakeup(RFID_DEV_INFO_T* result);
int kjtec_rfid__dev_ready_chk(RIFD_CHK_READY_T* result);
int kjtec_rfid__dev_rfid_all_clear(RIFD_DATA_ALL_CLR_T* result);
int kjtec_rfid__dev_write_rfid_data(int flag, char* rfid_user_str);
int kjtec_rfid__dev_rfid_req();

void init_kjtec_rfid();


#endif // __KJTEC_RFID_PROTOCOL_H__

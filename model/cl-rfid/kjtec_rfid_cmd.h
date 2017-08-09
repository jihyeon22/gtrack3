#ifndef __KJTEC_RFID_PROTOCOL_H__
#define __KJTEC_RFID_PROTOCOL_H__

#define KJTEC_RFID_MAX_CHK_DEV_CNT  5

#define KJTEC_RFID_INVAILD_FD   0xAABBCC

#define KJTEC_RFID_RET_FAIL     -1
#define KJTEC_RFID_RET_SUCCESS  1

#define RIFD_READ_BUFF_SIZE     1024


#define KJTEC_RFID_UART_DEVNAME         "/dev/ttyHSL1"
#define KJTEC_RFID_UART_BAUDRATE        115200
#define KJTEC_RFID_UART_READ_TIMEOUT    1
#define KJTEC_RFID_UART_READ_TIMEOUT2_USEC    500000
#define KJTEC_RFID_UART_READ_RETRY_CNT  2

#define KJTEC_RFID_CMD_TYPE__NORMAL_STR     1
#define KJTEC_RFID_CMD_TYPE__ONLY_BINARY    2
#define KJTEC_RFID_CMD__SLEEP_INTERAL_MIL   500


#define KJTEC_RFID_CMD_RESP_WAIT_TIME       4 * 10 // _SEC
#define KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL   100000 // usleep

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

typedef struct rfid_firmware_ver
{
    int cmd_result;
    char data_result[64]; // char??
}RFID_FIRMWARE_VER_T; // g_rfid_firmware_ver
#define RFID_CMD_ID_REQ__FIRMWARE_VER_INFO    0x60
#define RFID_CMD_ID_RESP__FIRMWARE_VER_INFO   0x61 // [Data Result,1,]

typedef struct rfid_db_info
{
    int cmd_result;
    int db_cnt;
    char db_date[128]; // char??
}RFID_DB_INFO_T; // g_rfid_db_info
#define RFID_CMD_ID_REQ__RFID_DB_INFO    0x62
#define RFID_CMD_ID_RESP__RFID_DB_INFO   0x63 // [Data Result,1,]


typedef struct rfid_firmware_down_pkt
{
    int cmd_result;
    int data_result; // char??
}RFID_FIRMWARE_DOWN_PKT_T; // g_rfid_firm_down_pkt
#define RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE        254
#define RFID_CMD_FIRMWARE_ONE_PKT_MAX_RETRY        1        // 여러번 시도해봤자 헛수고;;
#define RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_START    0x50
#define RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_ONE_PKT  0x99
#define RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_WRITE_RET   0x51 // [Data Result,1,]




#define RFID_CMD_ID_REQ__GET_PASSENGER_DATA             0x40
#define RFID_CMD_ID_RESP__GET_PASSENGER_DATA            0x41 
// +[0:[K0010W0000412001,1,20170704 085918,0],]
// +[4:[K0010W0000412001,1,20170705112223,0],]
#define GET_PASSENGER_DATA_PREFIX_STR   ":["
#define GET_PASSENGER_DATA_SUFFIX_STR   "],"
#define GET_PASSENGER_DATA_ARGUMENT_SPLIT_CHAR   ','
#define GET_PASSENGER_DATA_ARGUMENT_CNT   4
#define RFID_CMD_ID_REQ__GET_PASSENGER_DATA_SUCCESS     0x42 // ACK
#define RIFD_MAX_READ_USER_INFO_TRY_FAIL_CNT 10

// -------------------------------------------------------

int kjtec_rfid__dev_wakeup(RFID_DEV_INFO_T* result);
int kjtec_rfid__dev_ready_chk(RIFD_CHK_READY_T* result);
int kjtec_rfid__dev_rfid_all_clear(RIFD_DATA_ALL_CLR_T* result);
int kjtec_rfid__dev_write_rfid_data(int flag, char* rfid_user_str);
int kjtec_rfid__dev_rfid_req();
int kjtec_rfid__firmware_ver_info(RFID_FIRMWARE_VER_T* result);
int kjtec_rfid__rfid_db_info(RFID_DB_INFO_T* result);
int kjtec_rfid__firmware_write_start(int size);
int kjtec_rfid__firmware_write_one_pkt(char* buff, int buff_len);


void init_kjtec_rfid();


#endif // __KJTEC_RFID_PROTOCOL_H__

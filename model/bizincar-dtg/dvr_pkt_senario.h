#ifndef __DVR_PKT_SENARIO_H__
#define __DVR_PKT_SENARIO_H__

// dvr pkt evt code
#define DVR_EVT_CODE	eREPORT_CYCLE_SETUP_EVT

struct bizincar_dvr__dev_data {
	char buff[1024];
    int buff_len;
}__attribute__((packed));
typedef struct bizincar_dvr__dev_data bizincar_dvr__dev_data_t;

struct bizincar_dvr__server_resp {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_dvr__server_resp bizincar_dvr__server_resp_t;

// -------------------------------------------
// api..
// -------------------------------------------
int bizincar_dvr__send_pkt(char* dvr_str, int str_len);

int bizincar_dvr__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, char* recode, int recode_len );
int bizincar_dvr__parse_resp(bizincar_dvr__server_resp_t* resp);

int bizincar_dvr__mgr_init();
// -------------------------------------------
// uart setting..
// -------------------------------------------

#define DVR_DEV_DEFAULT_PATH          "/dev/ttyHSL1"
#define DVR_DEV_DEFAULT_BAUDRATE      9600

#define DVR_RET_SUCCESS                0
#define DVR_RET_FAIL                   -1
#define DVR_CMD_RET_ERROR              -2
#define DVR_CMD_UART_INIT_FAIL         -3
#define DVR_CMD_RET_CHECK_SUM_FAIL     -4
#define DVR_CMD_RET_TIMEOUT            -5
#define DVR_CMD_RET_INVALID_COND       -999

#define MAX_DVR_RET_BUFF_SIZE     1024

#define MAX_ADAS_CHK_ERR 120

#define DVR_INVAILD_FD              -44
#define DVR_UART_BUFF_SIZE          1024
#define DVR_UART_READ_TIMEOUT_SEC   3
#define DVR_UART_WRITE_CMD_SIZE     512
#define DVR_UART_INIT_TRY_CNT       3

#define DVR_UART_READ_THREAD_TIMEOUT 1

#define MAX_DRV_UART_INVAILD_CHK	20
#endif // __DVR_PKT_SENARIO_H__

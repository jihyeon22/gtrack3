#ifndef __MOBILEYE_ADAS_1_H__
#define __MOBILEYE_ADAS_1_H__

#include <adas_common.h>

#define MOBILEYE_ADAS_DEV_DEFAULT_PATH          "/dev/ttyHSL1"
#define MOBILEYE_ADAS_DEV_DEFAULT_BAUDRATE      9600

#define MOBILEYE_ADAS_RET_SUCCESS                0
#define MOBILEYE_ADAS_RET_FAIL                   -1
#define MOBILEYE_ADAS_CMD_RET_ERROR              -2
#define MOBILEYE_ADAS_CMD_UART_INIT_FAIL         -3
#define MOBILEYE_ADAS_CMD_RET_CHECK_SUM_FAIL     -4
#define MOBILEYE_ADAS_CMD_RET_TIMEOUT            -5
#define MOBILEYE_ADAS_CMD_RET_INVALID_COND       -999

#define MAX_MOBILEYE_ADAS_RET_BUFF_SIZE     1024

#define MAX_ADAS_CHK_ERR 120


int mobileye_adas__mgr_init(char *dev_name, int baud_rate, int (*p_bmsg_proc)(ADAS_EVT_DATA_T* evt_data));

int mobileye_adas__cmd_broadcast_msg_start();
int mobileye_adas__cmd_broadcast_msg_stop();

// ----------------------------------------------


typedef enum
{
    eMOBILEYE_CMD_RET__SUCCESS, 
    eMOBILEYE_CMD_RET__FAIL
}MOBILEYE_CMD_RET;


#endif // __MOBILEYE_ADAS_1_H__

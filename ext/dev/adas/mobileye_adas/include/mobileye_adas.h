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

// --------------------------------------------------
// FCW COMMAND : 전방추돌경보
// --------------------------------------------------
#define MOBILEYE_CMD_FCW__ARG_CNT   3
#define MOBILEYE_CMD_FCW__COMMAND   "$MEFCW"

typedef enum
{
    eMOBILEYE_CMD_FCW__BREAK_OFF = 0, 
    eMOBILEYE_CMD_FCW__BREAK_ON = 1, 
}MOBILEYE_CMD_FCW__BREAK;

typedef struct
{
    int speed_kmh;
    // int break_val; // remove spec
}MOBILEYE_CMD_FCW_VAL_T;

int mobileye_evt_cmd_parse__fcw(int argc, char* argv[], MOBILEYE_CMD_FCW_VAL_T* retval);


// --------------------------------------------------
// PCW COMMAND : 보행자 충돌 경보
// --------------------------------------------------
#define MOBILEYE_CMD_PCW__ARG_CNT   3
#define MOBILEYE_CMD_PCW__COMMAND   "$MEPCW"

typedef enum
{
    eMOBILEYE_CMD_PCW__BREAK_OFF = 0, 
    eMOBILEYE_CMD_PCW__BREAK_ON = 1, 
}MOBILEYE_CMD_PCW__BREAK;

typedef struct
{
    int speed_kmh;
    // int break_val; // remove spec
}MOBILEYE_CMD_PCW_VAL_T;

int mobileye_evt_cmd_parse__pcw(int argc, char* argv[], MOBILEYE_CMD_PCW_VAL_T* retval);

// --------------------------------------------------
// LDW COMMAND : 차선이탈
// --------------------------------------------------
#define MOBILEYE_CMD_LDW__ARG_CNT   4
#define MOBILEYE_CMD_LDW__COMMAND   "$MELDW"

typedef enum
{
    eMOBILEYE_CMD_LDW__BREAK_OFF = 0, 
    eMOBILEYE_CMD_LDW__BREAK_ON = 1, 
}MOBILEYE_CMD_LDW__BREAK;

typedef enum
{
    eMOBILEYE_CMD_LDW__LANE_LEFT = 'L', 
    eMOBILEYE_CMD_LDW__LANE_RIGHT = 'R', 
}MOBILEYE_CMD_LDW__LANE;

typedef struct
{
    int speed_kmh;
    // int break_val; // remove spec
    char lane_val;
}MOBILEYE_CMD_LDW_VAL_T;

int mobileye_evt_cmd_parse__ldw(int argc, char* argv[], MOBILEYE_CMD_LDW_VAL_T* retval);

// --------------------------------------------------
// HMW COMMAND : 차간거리 경보
// --------------------------------------------------
#define MOBILEYE_CMD_HMW__ARG_CNT   3
#define MOBILEYE_CMD_HMW__COMMAND   "$MEHMW"

typedef enum
{
    eMOBILEYE_CMD_HMW__BREAK_OFF = 0, 
    eMOBILEYE_CMD_HMW__BREAK_ON = 1, 
}MOBILEYE_CMD_HMW__BREAK;

typedef struct
{
    int speed_kmh;
    int break_val;
}MOBILEYE_CMD_HMW_VAL_T;

int mobileye_evt_cmd_parse__hmw(int argc, char* argv[], MOBILEYE_CMD_HMW_VAL_T* retval);

// --------------------------------------------------
// SLI COMMAND : 속도제한
// --------------------------------------------------
#define MOBILEYE_CMD_SLI__ARG_CNT   3
#define MOBILEYE_CMD_SLI__COMMAND   "$MESLI"

typedef struct
{
    int warnning_level;
    int speed_kmh;
}MOBILEYE_CMD_SLI_VAL_T;

int mobileye_evt_cmd_parse__sli(int argc, char* argv[], MOBILEYE_CMD_SLI_VAL_T* retval);

// --------------------------------------------------
// ERR COMMAND : ERR
// --------------------------------------------------
#define MOBILEYE_CMD_ERR__ARG_CNT   3
#define MOBILEYE_CMD_ERR__COMMAND   "$MEERR"

typedef struct
{
    int err_code;
}MOBILEYE_CMD_ERR_VAL_T;

// ERR COMMAND : ERR CODE
int mobileye_evt_cmd_parse__err(int argc, char* argv[], MOBILEYE_CMD_ERR_VAL_T* retval);

// --------------------------------------------------
// ALIVE COMMAND : ERR
// --------------------------------------------------
// $MESYNC,00*23
#define MOBILEYE_CMD_SYNC__ARG_CNT   3
#define MOBILEYE_CMD_SYNC__COMMAND   "$MESYNC"

typedef struct
{
    int sync_code;
}MOBILEYE_CMD_SYNC_VAL_T;

// ERR COMMAND : ERR CODE
int mobileye_evt_cmd_parse__sync(int argc, char* argv[], MOBILEYE_CMD_SYNC_VAL_T* retval);



#endif // __MOBILEYE_ADAS_1_H__

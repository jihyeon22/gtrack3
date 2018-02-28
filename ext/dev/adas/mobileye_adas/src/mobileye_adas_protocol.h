#ifndef __MOBILEYE_ADAS_PROTOCOL_H__
#define __MOBILEYE_ADAS_PROTOCOL_H__

#include <adas_common.h>

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



int mobileye_adas__uart_chk();
int mobileye_get_evt_data(int argc, char* argv[], ADAS_EVT_DATA_T* evt_data);

#endif




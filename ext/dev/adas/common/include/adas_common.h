#ifndef __ADAS_COMMON_H__
#define __ADAS_COMMON_H__


typedef enum
{
    eADAS_EVT__START = 0,
    eADAS_EVT__NONE = eADAS_EVT__START,
    eADAS_EVT__INVALID,
    eADAS_EVT__FCW,  // FCW COMMAND : 전방추돌경보
    eADAS_EVT__UFCW, // UFCW COMMAND : 서행 충돌 경보
    eADAS_EVT__LDW,  // LDW COMMAND : 차선이탈
    eADAS_EVT__PCW,  // PCW COMMAND : 보행자 충돌 경보
    eADAS_EVT__SLI, // SLI COMMAND : 속도제한
    eADAS_EVT__TSR, // TSR COMMAND : 교통표지판 인식
    eADAS_EVT__HMW, // HMW COMMAND : 차간거리 경보
    eADAS_EVT__ERR, // ERR COMMAND : ERR
    eADAS_EVT__FPW,  // FCW COMMAND : 전방추돌경보
    eADAS_EVT__END,
}eADAS_EVT_CODE;

typedef struct
{
    eADAS_EVT_CODE evt_code;
    int evt_data_1;  // movon : speed 
    int evt_data_2;  // movon : break sig
    int evt_data_3;  // movon : ttc sec
    int evt_data_4;  // movon : ext data (ldw => 0:left / 1:right)
    char evt_ext[32];
    // int break_val; // remove spec
}ADAS_EVT_DATA_T;

#define ADAS_EVT_RET_SUCCESS    0
#define ADAS_EVT_RET_FAIL       -1

#endif // __ADAS_COMMON_H__


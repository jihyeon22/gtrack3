#ifndef __MOVON_ADAS_1_H__
#define __MOVON_ADAS_1_H__

#include <adas_common.h>

#define MOVON_ADAS_DEV_DEFAULT_PATH          "/dev/ttyHSL2"
#define MOVON_ADAS_DEV_DEFAULT_BAUDRATE      9600
// movon setting : rs232 -> convert

#define MOVON_ADAS_RET_SUCCESS                0
#define MOVON_ADAS_RET_FAIL                   -1
#define MOVON_ADAS_CMD_RET_ERROR              -2
#define MOVON_ADAS_CMD_UART_INIT_FAIL         -3
#define MOVON_ADAS_CMD_RET_CHECK_SUM_FAIL     -4
#define MOVON_ADAS_CMD_RET_TIMEOUT            -5
#define MOVON_ADAS_CMD_RET_INVALID_COND       -999

#define MAX_MOVON_ADAS_RET_BUFF_SIZE     2048

int movon_adas__mgr_init(char *dev_name, int baud_rate, int (*p_bmsg_proc)(ADAS_EVT_DATA_T* evt_data));

int movon_adas__cmd_broadcast_msg_start();
int movon_adas__cmd_broadcast_msg_stop();

// ----------------------------------------------
typedef enum
{
    eMOVON_CMD_RET__SUCCESS, 
    eMOVON_CMD_RET__FAIL
}MOVON_CMD_RET;


typedef struct
{
	char stx;           // Transmission Start   // [
	char type1;         // Header 1     // I
    char type2;         // Header 2     // B
    char data_len;      // The number of Characters in data part // 0x30 + 20
    char speed;         // Speed ( 0 ~ 255 /  0x00 – 0xFF)
    char left_turn_signal;      // 0x00 – None , 0x01 – Left turn signal On
    char right_turn_signal;     // 0x00 – None, 0x01 – Right turn signal On
    char break_signal;          // 0x00 – None, 0x01 – Brake On
    unsigned short rpm;    // 0 ~ 65535 // RPM[0] MSB / RPM[1] LSB
    char LDW_left;              // 0x00 – None  , 0x01 – Recognized, 0x02 – Left LDW event , 0x03 – Function disabled 
    char LDW_right;             // 0x00 – None , 0x01 – Recognized, 0x02 – Right LDW event, 0x03 – Function disabled
    unsigned short left_distance;       // Length of left lane (cm) // Left[0] MSB / Left[1] LSB
    unsigned short right_distance;      // Length of right lane (cm) // Right[0] MSB / Right[1] LSB
    char ttc_sec;           // TTC time // 0 ~ 25.5 sec // Ex) 125 => 12.5sec
    char sda;       // 0x00 – None , 0x01 – Recognized (ahead vhicle), 0x02 – SDA event, 0x03 – Function disabled
    char fvsa;      // (Front Vehicle Start Alarm)  // 0x00 – None , 0x02 – FVSA event, 0x03 – Function disabled
    char fpw;       // (Forward Proximity Warning) // 0x00 – None , 0x02 – FPW event, 0x03 – Function disabled
    char fcw;       // (Forward Collision Warning ) // 0x00 – None , 0x02 - FCW event, 0x03 – Function disabled
    char pcw;       // 0x00 – None, 0x01 – Recognized, 0x02 – PCW event, 0x03 – Function disabled
    char recode;        // 0x00 – None , 0x01 – Recording (Mic off), 0x02 – Recording (Mic On)
    char errcode;       // 0x00 – None, 0x01 – Low visibility, 0x02 – Camera blocked 
    char chksum;
    char etx;     // Transmission End
}MOVON_DATA_FRAME_T;

void movon_adas__set_cur_data(MOVON_DATA_FRAME_T* data);
void movon_adas__get_cur_data(MOVON_DATA_FRAME_T* data);



#endif // __MOVON_ADAS_1_H__

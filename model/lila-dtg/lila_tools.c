#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <include/defines.h>
#include <base/config.h>
#include <base/gpstool.h>
#include <base/mileage.h>
#include <base/error.h>

#include <at/at_util.h>

#include <board/power.h>
#include <board/gpio.h>
#include <board/modem-time.h>
#include <board/board_system.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_version.h>

#include "dtg_gtrack_tool.h"

#include "data-list.h"

#include "lila_adas_mgr.h"
#include "lila_tools.h"

#define CONV_BUFF_SIZE 64
char conv_str_buff[CONV_BUFF_SIZE] = {0,};

static const unsigned short crc16tab[256]= {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

unsigned short lila_tools__crc16_ccitt(const void *buf, int len)
{
    register int counter;
    register unsigned short crc = 0;
    for( counter = 0; counter < len; counter++)
    crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(char *)buf++)&0x00FF];
    return crc;
}


// ------------------------------------------------------------------------
// pkt tools
// ------------------------------------------------------------------------
int lila_tools__get_phonenum_int_type()
{
    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    static unsigned int phonenum_int = 0;

    if ( phonenum_int > 0 )
        return phonenum_int;

    at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);
    phonenum_int = atoi(phonenum);
    // test code
    //phonenum_int = 1223687989;

    return phonenum_int;
}


char* lila_tools__conv_buff_to_str(char* src, int src_size)
{
    memset(&conv_str_buff, 0x00, sizeof(conv_str_buff));
    memcpy(&conv_str_buff, src, src_size);

    return conv_str_buff;
}

char* lila_tools__conv_buff_to_str_reverse(char* src, int src_size)
{
    char tmp_buff[CONV_BUFF_SIZE] = {0,};
    int i = 0;

    memset(&conv_str_buff, 0x00, sizeof(conv_str_buff));
    memcpy(&tmp_buff, src, src_size);
    
    for (i = 0; i < src_size ; i ++)
    {
        conv_str_buff[i] = src[src_size - i - 1];
    }

    return conv_str_buff;
}


// putty : ISO-8859-1:1998 (Latin-1, West Europe)
// text editor : ansi encoding
static tacom_std_hdr_t g_current_std_hdr = {0,};
static tacom_std_data_t g_current_std_data = {0,};

void lila_dtg__set_current_dtg_data(tacom_std_hdr_t* p_current_std_hdr, tacom_std_data_t* p_current_std_data)
{
	memcpy(&g_current_std_hdr, p_current_std_hdr, sizeof(tacom_std_hdr_t));
printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ : set_current_dtg_data ++++\n");
	memcpy(&g_current_std_data, p_current_std_data, sizeof(tacom_std_data_t));
#if defined(DEVICE_MODEL_INNOCAR)
printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ : set_current_dtg_data-----\n");
printf("%d, %d, %d, %d\n", g_current_std_data.k_factor, g_current_std_data.rpm_factor, g_current_std_data.weight1, g_current_std_data.weight2);
	set_factor_value(g_current_std_data.k_factor, g_current_std_data.rpm_factor, g_current_std_data.weight1, g_current_std_data.weight2, g_current_std_hdr.dtg_fw_ver);
#endif
	//lila_dtg_debug__print_tacom_std_hdr(p_current_std_hdr);
}

void lila_dtg__get_current_dtg_data(tacom_std_hdr_t* p_current_std_hdr, tacom_std_data_t* p_current_std_data)
{
    if ( p_current_std_hdr != NULL)
	    memcpy(p_current_std_hdr, &g_current_std_hdr, sizeof(tacom_std_hdr_t));
    
    if ( p_current_std_data != NULL)
        memcpy(p_current_std_data, &g_current_std_data, sizeof(tacom_std_data_t));

    //lila_dtg_debug__print_tacom_std_hdr(&g_current_std_hdr);
	//lila_dtg_debug__print_tacom_std_hdr(p_current_std_data);
}


/*
char* lila_tools__swap_str(char* src, int size)
{
	char tmp_buff[512] = {0,};
	int i = 0;
	memcpy(&tmp_buff, src, size);

	for( i = 0 ; i < size ; i ++ )
	{
		src[0] = tmp_buff[size - i - 1];
	}
    return size;
}
*/


unsigned char lila_dtg__convert_angle(int bearing)
{
	if(bearing == 0) {
		return 0;
	} else if((bearing > 0) && (bearing < 90)) {
		return 1;
	} else if(bearing == 90) {
		return 2;
	} else if((bearing > 90) && (bearing < 180)) {
		return 3;
	} else if(bearing == 180) {
		return 4;
	} else if((bearing > 180) && (bearing < 270)) {
		return 5;
	} else if(bearing == 270) {
		return 6;
	} else if((bearing > 270) && (bearing <= 360)) {
		return 7;
	}

	return 0xff;
}

unsigned char lila_tools__get_rssi(unsigned int cur_time)
{
    int calc_val;
    static int last_time = 0;
    static int last_ret_val = 0;

    if ( ( ( cur_time - last_time ) < LILA_TOOLS__CALC_INTERVAL ) || (last_ret_val != 0) )
    {
        //printf("%s() -> return last val [%d]\r\n", __func__, last_ret_val);
        return last_ret_val;
    }

    last_time = cur_time;

    //printf("%s() -> calc start!\r\n", __func__);

    if ( at_get_rssi(&calc_val) == AT_RET_SUCCESS) 
        last_ret_val = calc_val;

    printf("%s() -> ret val [%d]\r\n", __func__, last_ret_val);

    return last_ret_val;
}

unsigned short lila_tools__get_car_batt(unsigned int cur_time)
{
    int calc_val;
    static int last_time = 0;
    static int last_ret_val = 0;

    if ( ( ( cur_time - last_time ) < LILA_TOOLS__CALC_INTERVAL ) || (last_ret_val != 0) )
    {
        //printf("%s() -> return last val [%d]\r\n", __func__, last_ret_val);
        return last_ret_val;
    }

    last_time = cur_time;

    //printf("%s() -> calc start!\r\n", __func__);

    if ( at_get_adc_main_pwr(&calc_val) == AT_RET_SUCCESS) 
        last_ret_val = calc_val;
    
    printf("%s() -> ret val [%d]\r\n", __func__, last_ret_val);

    return last_ret_val;
}

char lila_tools__get_gps_stat(int gps_lat, int gps_lon)
{
    if ( ( gps_lat == 0 ) || ( gps_lon == 0 ))
        return 'N';
    else
        return 'A';
}

unsigned short lila_tools__get_car_stat(char* buff)
{
    int stat_num = atoi(buff);
    unsigned short ret_val = 0;
    int bit_set_val = 0;

    switch (stat_num)
    {
        //case 00: //	운행기록장치정상
        //break;
        case 11: //	위치추적장치(GPS 수신기)이상
            bit_set_val = 0;
        break;
        case 12: //	속도센서이상
            bit_set_val = 1;
        break;
        case 13: //	RPM 센서이상
            bit_set_val = 2;
        break;
        case 14: //	브레이크 신호감지 센서 이상
            bit_set_val = 3;
        break;
        case 21: //	센서 입력부 장치 이상
            bit_set_val = 4;
        break;
        case 22: //	센서 출력부 장치 이상
            bit_set_val = 5;
        break;
        case 31: //	데이터 출력부 장치 이상
            bit_set_val = 6;
        break;
        case 32: //	통신 장치 이상
            bit_set_val = 7;
        break;
        case 41: //	운행거리 산정 이상
            bit_set_val = 8;
        break;
        case 99: //	전원공급 이상
            bit_set_val = 9;
        break;
    }

    ret_val |= 1UL << bit_set_val;

    return ret_val;
}
unsigned char lila_tools__get_adas_data(unsigned int cur_time, unsigned char* aebs_data, unsigned char* ldw_data)
{
    LILA_ADAS__DATA_T*      p_lila_adas_data;
    unsigned char  ret_val_aebs = 0;
    unsigned char  ret_val_ldw = 0;

    int adas_data_cnt = 0;

    adas_data_cnt = get_lila_adas_data_count();

    //printf("%s()[%d] : adas data cnt [%d]\r\n",__func__, __LINE__, adas_data_cnt);
    while(list_pop(&lila_adas_data_buffer_list, (void *)&p_lila_adas_data) >= 0)
    {
        switch(p_lila_adas_data->evt_code)
        {
            case eADAS_CODE__FCW: // 4
                ret_val_aebs |= 1UL << (4);
                break;  
            case eADAS_CODE__PCW: // 5
                ret_val_aebs |= 1UL << (5);
                break;  
            case eADAS_CODE__HMW: // 6~7
                // ret_val_aebs
                break; 
            case eADAS_CODE__LDW_R: // 4
                ret_val_ldw |= 1UL << (4);
                break;
            case eADAS_CODE__LDW_L: // 6
                ret_val_ldw |= 1UL << (6);
                break;
            default:
                break;
        }

        //printf("dtg sec [%d] / adas sec [%d]\r\n", cur_time, p_lila_adas_data->time_sec);
        if ( p_lila_adas_data->time_sec >= cur_time )
        {
            free(p_lila_adas_data);
            break;
        }
        free(p_lila_adas_data);
    }


    //printf("%s()[%d] : adas data cnt [%d]\r\n",__func__, __LINE__, adas_data_cnt);

   // if ( ( ret_val_aebs != 0 ) || (ret_val_ldw != 0) )
    //    printf("ret_val_aebs [0x%x] / ret_val_ldw [0x%x] \r\n", ret_val_aebs, ret_val_ldw);

     *aebs_data = ret_val_aebs;
     *ldw_data = ret_val_ldw ;
/*
    *aebs_data = 0xff;
    *ldw_data = 0xff ;
*/
    return 0;
}


unsigned short lila_tools__get_car_signal()
{
    int key_stat = 0; 
    int ret_val = 0;


    key_stat = lila_dtg__key_stat();

    if ( key_stat == 1 )
        ret_val |= 1UL << 3;
    
    return ret_val;
}


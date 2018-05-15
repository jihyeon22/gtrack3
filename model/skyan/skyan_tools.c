
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logd_rpc.h>

#include <board/modem-time.h>

#include <at/at_util.h>
#include <mdsapi/mds_api.h>
#include <util/storage.h>

#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"

// ---------------------------------------------------
// batt tools
// ---------------------------------------------------
static int g_car_batt_level = 0;
static int g_low_batt_level = 0;
int skyan_tools__get_car_batt_level()
{
    return g_car_batt_level;
}

// 1초당 한번씩 불린다.
int skyan_tools__chk_car_batt_level(int low_batt, int chk_flag)
{
    static int batt_chk_interval = 0;
    static int evt_send_flag = 0;

    int car_voltage = 0;

    batt_chk_interval ++;

    //printf(" ----------> batt level is [%d] / [%d]\r\n", g_car_batt_level, batt_chk_interval);

    // batt 값을 강제로 업데이트 할 경우다.
    if ( chk_flag == SKYAN_TOOLS__CHK_IMMEDIATELY )
        batt_chk_interval = BATT_CHK_INTERVAL_SEC;

    // 최초 실행시 유효값 얻는다.
    if ( ( g_car_batt_level <= 0 ) || ( g_car_batt_level > 420 ) )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        LOGT(eSVC_MODEL, "[SKYAN BATT] 1 -> low batt [%d] / cur batt [%d] \r\n",  low_batt, g_car_batt_level);
        // check batt valid range..
        if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
            return 0;
        else
             g_car_batt_level = car_voltage;
    }

    if ( batt_chk_interval++ >= BATT_CHK_INTERVAL_SEC )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        LOGT(eSVC_MODEL, "[SKYAN BATT] 2 -> low batt [%d] / cur batt [%d] \r\n",  low_batt, g_car_batt_level);
        batt_chk_interval = 0;
    }

    
    // check batt valid range..
    if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
        return 0;
    else
         g_car_batt_level = car_voltage;

    LOGT(eSVC_MODEL, "[SKYAN BATT] 3 -> low batt [%d] / cur batt [%d] \r\n",  low_batt, g_car_batt_level);

    if ( (low_batt > 0 ) && ( low_batt > g_car_batt_level ) )
    {
        skyan_tools__set_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_LOW);
        if ( evt_send_flag == 0 )
        {
            send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__LOW_BATT);
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt\r\n", car_voltage);

            evt_send_flag = 1;
        }
        else
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt skip...\r\n", car_voltage);
    }
    else if ( (low_batt > 0 ) && ( low_batt < g_car_batt_level ) ) // evt clear condition
    {
        skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_LOW);
        evt_send_flag = 0;
    }

    return 0;
}

int skyan_tools__get_low_batt_level()
{
    return g_low_batt_level;
}

int skyan_tools__set_low_batt_level(int low_batt)
{
    g_low_batt_level = low_batt;
    return g_low_batt_level;
}


// ---------------------------------------------------
// gps tools
// ---------------------------------------------------
static int g_gps_ant_stat = SKYAN_TOOLS__GPS_ANT_STAT_OK;
int skyan_tools__get_gps_ant_stat()
{
    return g_gps_ant_stat;
}

int skyan_tools__set_gps_ant_stat(int stat)
{
    g_gps_ant_stat = stat;
    return g_gps_ant_stat;
}


int skyan_tools__get_gps_stat(int gps_stat)
{
    int status = 0;

    if ( gps_stat == 1 )
    {
        status = 2; // 유효 2d
    }
    else
    {
        if ( skyan_tools__get_gps_ant_stat() == SKYAN_TOOLS__GPS_ANT_STAT_OK )
            status = 1; // 무효 2d
        else
            status = 0; // 단선
    }

    return status;
}

// ---------------------------------------------------
// key tools
// ---------------------------------------------------
static int g_key_on_time = 0;
static int g_key_off_time = 0;
static int g_key_cur_stat = SKYAN_KEY_STAT_INVALID;

int skyan_tools__get_key_on_time()
{
    return g_key_on_time;
}

int skyan_tools__get_key_off_time()
{
    return g_key_off_time;
}

int skyan_tools__set_key_on_time(int cur_time)
{
    g_key_on_time = cur_time;
    return g_key_on_time;
}

int skyan_tools__set_key_off_time(int cur_time)
{
    g_key_off_time = cur_time;
    return g_key_off_time;
}

int skyan_tools__set_key_stat(int key_stat)
{
    g_key_cur_stat = key_stat;

    if ( key_stat == SKYAN_KEY_STAT_ON)
    {
        skyan_tools__set_key_on_time(get_modem_time_utc_sec());
    }

    if ( key_stat == SKYAN_KEY_STAT_OFF)
    {
        skyan_tools__set_key_off_time(get_modem_time_utc_sec());
    }
    return g_key_cur_stat;
}

int skyan_tools__get_key_stat()
{
    return g_key_cur_stat;
}

// -------------------------------------------------------
// stat bit field
// -------------------------------------------------------
static unsigned char g_devstat_bit_field = 0;
int skyan_tools__get_devstat()
{
    LOGI(eSVC_MODEL, "[SKYAN] GET DEV STAT bit [0x%x] \r\n", g_devstat_bit_field);
    return g_devstat_bit_field;
}

int skyan_tools__set_devstat(int bit)
{
    unsigned char bit_position = bit;
    unsigned char tempbyte = 0x01;

    //set bit
    g_devstat_bit_field = (g_devstat_bit_field | (tempbyte << bit_position));// set the bit at the position given by bit_position

    LOGI(eSVC_MODEL, "[SKYAN] GET DEV SET bit [%d] => [0x%x] \r\n", bit, g_devstat_bit_field);

    return g_devstat_bit_field;
}

int skyan_tools__clear_devstat(int bit)
{
    unsigned char bit_position = bit;
    unsigned char tempbyte = 0x01;

    //clear bit
    g_devstat_bit_field = (g_devstat_bit_field & ~(tempbyte << bit_position));// set the bit at the position given by bit_position

    LOGI(eSVC_MODEL, "[SKYAN] GET DEV CLR bit [%d] => [0x%x] \r\n", bit, g_devstat_bit_field);

    return g_devstat_bit_field;
}

int skyan_tools__get_devstat_is_set(int bit)
{
    unsigned char bit_position = bit;
    unsigned char tempbyte = 0x01;

    //clear bit
    return (g_devstat_bit_field & (tempbyte << bit_position));// set the bit at the position given by bit_position
}


// ---------------------------------------------------
// save and resume data tool
// ---------------------------------------------------
SKYAN_RESUME_DATA_V1_T g_resume_data;

void skyan_tools__save_resume_data()
{
    int try_cnt = 4;

    while(try_cnt--)
    {
        if( storage_save_file(SKYAN_RESUME_DATA_PATH_V1, &g_resume_data, sizeof(g_resume_data)) >= 0)
        {
            LOGI(eSVC_MODEL, "[SKYAN] SAVE RESUME DATA SUCCESS \r\n");
            break;  
        }
    }

    if( try_cnt < 0 )
        LOGE(eSVC_MODEL, "[SKYAN] SAVE RESUME DATA FAIL [%d] \r\n", try_cnt);
}

void skyan_tools__load_resume_data()
{
    SKYAN_RESUME_DATA_V1_T resume_data = {0,};

    if ( storage_load_file(SKYAN_RESUME_DATA_PATH_V1, &resume_data, sizeof(resume_data)) >= 0 )
    {
        memcpy(&g_resume_data, &resume_data, sizeof(g_resume_data));
        LOGI(eSVC_MODEL, "[SKYAN] LOAD RESUME DATA SUCCESS : nostart_flag [%d] \r\n", g_resume_data.nostart_flag);
    }
    else
    {
        g_resume_data.nostart_flag = SKYAN_TOOLS__NORMAL_MODE;
        skyan_tools__save_resume_data(); // init save..
        LOGE(eSVC_MODEL, "[SKYAN] LOAD RESUME DATA FAIL \r\n");
    }

    if ( g_resume_data.nostart_flag == SKYAN_TOOLS__NOSTART_MODE )
    {
        skyan_tools__set_nostart_flag(SKYAN_TOOLS__NOSTART_MODE);
    }
    else
    {
        skyan_tools__set_nostart_flag(SKYAN_TOOLS__NORMAL_MODE);
    }
}


// -------------------------------------------------
// nostart flag tool
// -------------------------------------------------
static int g_nostart_flag = SKYAN_TOOLS__NORMAL_MODE;
int skyan_tools__get_nostart_flag()
{
    return g_nostart_flag;
}

int skyan_tools__set_nostart_flag(int mode)
{
    LOGT(eSVC_MODEL, "[SKYAN] SET NOSTART FLAG [%d]\r\n", mode);

    if ( mode == SKYAN_TOOLS__NOSTART_MODE)
    {
        skyan_tools__set_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__KEYON_STAT_1);
        g_nostart_flag = SKYAN_TOOLS__NOSTART_MODE;
    }
    else if ( mode == SKYAN_TOOLS__NORMAL_MODE)
    {
        skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__KEYON_STAT_1);
        g_nostart_flag = SKYAN_TOOLS__NORMAL_MODE;
    }

    if ( mode != g_resume_data.nostart_flag )
    {
        g_resume_data.nostart_flag = mode;
        skyan_tools__save_resume_data();
    }

    return g_nostart_flag;
}

// ------------------------------------------------
// firmware
// ------------------------------------------------
int skyan_tools__get_firm_ver(char* ver_str)
{
    strncpy(ver_str, SKYAN_APP_VER_STR, strlen(SKYAN_APP_VER_STR));
    return 0;
}



// ------------------------------------------------
// setting info tools
// ------------------------------------------------
static int g_setting_info_is_save = 0;
SKY_AUTONET_PKT__SETTING_INFO_T g_setting_info;
void skyan_tools__set_setting_info(SKY_AUTONET_PKT__SETTING_INFO_T* setting_info)
{
    memcpy(&g_setting_info, setting_info, sizeof(g_setting_info));
    g_setting_info_is_save = 1;
}

SKY_AUTONET_PKT__SETTING_INFO_T* skyan_tools__get_setting_info()
{
    if ( g_setting_info_is_save == 1)
        return &g_setting_info;
    else
        return NULL;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/storage.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <logd_rpc.h>

#include "netcom.h"
#include <logd_rpc.h>

#include "alloc2_senario.h"
#include "seco_obd_1.h"

#include <mdsapi/mds_api.h>

static int _g_cur_stat = e_STAT_NONE;

int set_cur_status(e_ALLOC2_SENARIO_STAT stat)
{
    _g_cur_stat = stat;
    return _g_cur_stat;
}

int get_cur_status()
{
    return _g_cur_stat;
}


// ------------------------------------------------------------------
static ALLOC_PKT_RECV__MDM_SETTING_VAL _g_mdm_setting_val;
static int _mdm_setting_init_success = 0;

int init_mdm_setting_pkt_val()
{
    int ret = 0;
    memset(&_g_mdm_setting_val, 0x00, sizeof(_g_mdm_setting_val));

    ret = storage_load_file(ALLOC2_MDM_SETTING_INFO, &_g_mdm_setting_val, sizeof(_g_mdm_setting_val));

    if( ret >= 0 )
    {
        printf(" -----------------> mdm info load from [%s] success!!!\r\n", ALLOC2_MDM_SETTING_INFO);
        _mdm_setting_init_success = 1;
    }
    else
    {
        printf(" -----------------> mdm info load from [%s] fail!!! clr memory \r\n", ALLOC2_MDM_SETTING_INFO);
        _mdm_setting_init_success = 0;
        memset(&_g_mdm_setting_val, 0x00, sizeof(_g_mdm_setting_val));

        _g_mdm_setting_val.key_on_gps_report_interval = 60;    // (b-2) ig on gps 정보보고 시간설정(초단위) 운행시
        _g_mdm_setting_val.key_off_gps_report_interval = 180;    // (b-2) ig off gps 정보보고 시간설정(초단위) 비운행시
    }

    return 0;
}

int set_mdm_setting_pkt_val(ALLOC_PKT_RECV__MDM_SETTING_VAL* setting_val)
{
    init_mdm_setting_pkt_val();
    memcpy(&_g_mdm_setting_val, setting_val, sizeof(_g_mdm_setting_val));

	storage_save_file(ALLOC2_MDM_SETTING_INFO, (void*)&_g_mdm_setting_val, sizeof(_g_mdm_setting_val));

    _mdm_setting_init_success = 1;
    return 0;
}

ALLOC_PKT_RECV__MDM_SETTING_VAL* get_mdm_setting_val()
{
    return &_g_mdm_setting_val;
}

// ------------------------------------------------------------------

static ALLOC_PKT_RECV__OBD_DEV_INFO _g_obd_dev_info;
static int _g_obd_dev_info_init_success = 0;

int init_obd_dev_pkt_info()
{
    int ret = 0;
    memset(&_g_obd_dev_info, 0x00, sizeof(_g_obd_dev_info));

    ret = storage_load_file(ALLOC2_OBD_SETTING_INFO, &_g_obd_dev_info, sizeof(_g_obd_dev_info));

    if( ret >= 0 )
    {
        printf(" -----------------> obd info load from [%s] success!!!\r\n", ALLOC2_OBD_SETTING_INFO);
        _g_obd_dev_info_init_success = 1;
    }
    else
    {
        printf(" -----------------> obd info load from [%s] fail!!! clr memory \r\n", ALLOC2_OBD_SETTING_INFO);
        memset(&_g_obd_dev_info, 0x00, sizeof(_g_obd_dev_info));
    }
    
    return 0;
}

int set_obd_dev_pkt_info(ALLOC_PKT_RECV__OBD_DEV_INFO* setting_val)
{
    // init_obd_dev_pkt_info();
    memcpy(&_g_obd_dev_info, setting_val, sizeof(_g_obd_dev_info));

	storage_save_file(ALLOC2_OBD_SETTING_INFO, (void*)&_g_obd_dev_info, sizeof(_g_obd_dev_info));

    _g_obd_dev_info_init_success = 1;
    return 0;
}

ALLOC_PKT_RECV__OBD_DEV_INFO* get_obd_dev_info()
{
    if ( _g_obd_dev_info_init_success == 0 ) 
        return NULL;
    else
        return &_g_obd_dev_info;
}


static int _keyon_distance = 0;
int chk_keyon_section_distance(int total_distance)
{
    int cur_keyon_distance = 0;
    static int saved_keyon_distance = 0;

    if ( _keyon_distance > 0 )
    {
        cur_keyon_distance =  total_distance - _keyon_distance;
    }
    else
    {
        _keyon_distance = total_distance;
        cur_keyon_distance = 0;
    }

	if ( ( cur_keyon_distance >= 0 ) && ( cur_keyon_distance >= saved_keyon_distance) )
	{
		saved_keyon_distance = cur_keyon_distance;
	}
	else
	{
		cur_keyon_distance = saved_keyon_distance;
	}

    return cur_keyon_distance;
}

int init_keyon_section_distance(int total_distance)
{
    _keyon_distance = total_distance;
    return 0;
}


int get_sms_pkt_cmd_code(unsigned char code)
{
    if ( code <= 10 )
        return 0;
    else if ( code <= 29 )
        return 10;
    else if ( code <= 39 )
        return 30;
    else if ( code <= 49 )
        return 40;
    else if ( code <= 59 )
        return 50;
    else if ( code <= 69 )
        return 60;
    else if ( code <= 79 )
        return 70;
    else if ( code <= 89 )
        return 80;
    else if ( code <= 99 )
        return 90;
    
    return 0;

}




void alloc2_poweroff_proc(char* msg)
{
    mileage_write();
	gps_valid_data_write();
	devel_webdm_send_log(msg);

    devel_webdm_send_log("Accumulate distance : %um at the start\n", mileage_get_m());
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	poweroff("sernaio", strlen("sernaio"));

}

// --------------------------------------------------------
static int _car_ctrl_flag = 0;
int set_car_ctrl_enable(int flag)
{
    _car_ctrl_flag = flag;
    return _car_ctrl_flag;
}

int get_car_ctrl_enable()
{
    return _car_ctrl_flag;
}

// ----------------------------------------------------------
int set_no_send_pwr_evt_reboot()
{
    int evt_code = e_evt_code_mdm_reset;

    char touch_cmd[128] = {0};
    sprintf(touch_cmd, "touch %s &",NO_SEND_TO_PWR_EVT_FLAG_PATH);
    system(touch_cmd);
    

	sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
	sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);

	alloc2_poweroff_proc("senario power off");
}

int get_no_send_pwr_evt_reboot(int flag)
{
    int ret_flag = SEND_TO_PWR_EVT_NOK;

    static int power_evt_cnt = 0;
    static int igi_evt_cnt = 0;

    // flag 없으면 바로 ok 리턴
    if ( mds_api_check_exist_file(NO_SEND_TO_PWR_EVT_FLAG_PATH, 0) != DEFINES_MDS_API_OK)
    {
        printf("NO SEND EVT ==> ret OK\r\n");
        return SEND_TO_PWR_EVT_OK;
    }

    switch(flag)
    {
        case EVT_TYPE_POWER_ON:
        case EVT_TYPE_POWER_OFF:
        {
            if ( power_evt_cnt++ > 1 )
                ret_flag = SEND_TO_PWR_EVT_OK;
            break;
        }
        case EVT_TYPE_IGI_ON:
        case EVT_TYPE_IGI_OFF:
        {
            if ( igi_evt_cnt++ > 1 )
                ret_flag = SEND_TO_PWR_EVT_OK;
            break;
        }
    }

    printf("NO SEND EVT ==> [%d], [%d], [%d] => ret [%d] \r\n", flag, power_evt_cnt, igi_evt_cnt, ret_flag);

    if ( ( igi_evt_cnt > 1 ) && ( power_evt_cnt > 1 ) )
        unlink(NO_SEND_TO_PWR_EVT_FLAG_PATH);

    return ret_flag;
}

int clr_no_send_pwr_evt_reboot()
{
    if ( mds_api_check_exist_file(NO_SEND_TO_PWR_EVT_FLAG_PATH, 0) == DEFINES_MDS_API_OK)
    {
        unlink(NO_SEND_TO_PWR_EVT_FLAG_PATH);
    }
    
}
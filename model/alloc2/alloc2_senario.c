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

#include "alloc2_nettool.h"

#include "alloc2_senario.h"
#include "seco_obd_1.h"

#include <mdsapi/mds_api.h>

#include <at/at_util.h>

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
	sender_wait_empty_network(20);
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
    
    save_resume_data();

	sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
	sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));

	alloc2_poweroff_proc("senario power off");
}

int get_no_send_pwr_evt_reboot(int flag)
{
    int ret_flag = SEND_TO_PWR_EVT_NOK;

    static int power_evt_cnt = 0;
    static int igi_evt_cnt = 0;

    load_resume_data();
    
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

int chk_read_sms()
{
    // SK
//    AT+CMGR=0
    // KT / LG
//    AT+CMGR=24
    at_chk_read_sms();
}

static int _g_car_batt_level = 0;
int get_car_batt_level()
{
    return _g_car_batt_level;
}

// 1초당 한번씩 불린다.
int chk_car_batt_level(int low_batt, int chk_flag)
{
    static int batt_chk_interval = 0;
    static int evt_send_flag = 0;

    int car_voltage = 0;

    batt_chk_interval ++;

    printf(" ----------> batt level is [%d] / [%d]\r\n", _g_car_batt_level, batt_chk_interval);

    // batt 값을 강제로 업데이트 할 경우다.
    if ( chk_flag == 1 )
        batt_chk_interval = BATT_CHK_INTERVAL_SEC;

    // 최초 실행시 유효값 얻는다.
    if ( ( _g_car_batt_level <= 0 ) || ( _g_car_batt_level > 420 ) )
    {
        at_get_adc_main_pwr(&car_voltage);
        // check batt valid range..
        if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
            return 0;
        else
             _g_car_batt_level = car_voltage;
    }

    if ( batt_chk_interval++ >= BATT_CHK_INTERVAL_SEC )
    {
        at_get_adc_main_pwr(&car_voltage);
        car_voltage = car_voltage * 10;
        batt_chk_interval = 0;
    }

    
    // check batt valid range..
    if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
        return 0;
    else
         _g_car_batt_level = car_voltage;

    if ( (low_batt > 0 ) && ( low_batt > _g_car_batt_level ) )
    {
        int evt_code = e_evt_code_car_low_batt;

        if ( evt_send_flag == 0 )
        {
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt\r\n", car_voltage);

            sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
            sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
            evt_send_flag = 1;
        }
        else
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt skip...\r\n", car_voltage);
    }
    else
    {
        evt_send_flag = 0;
    }

    return 0;
}


// ---------------------------------------------------------------------
static int knocksensor_setting_result=KNOCKSENSOR_SETTING__INIT;

int chk_bcm_knocksensor_setting()
{
    switch(knocksensor_setting_result)
    {
        case KNOCKSENSOR_SETTING__INIT:
        {
            sender_add_data_to_buffer(e_bcm_knocksensor_setting_val, NULL, get_pkt_pipe_type(e_bcm_knocksensor_setting_val,0));
            knocksensor_setting_result = KNOCKSENSOR_SETTING__SETTING_VAL_CHECKING;
            break;
        }
        case KNOCKSENSOR_SETTING__SETTING_VAL_CHECKING:
        {
            break;
        }
        case KNOCKSENSOR_SETTING__SETTING_VAL_CHECK_DONE:
        {
            unsigned short knock_sensor_id = 0;
            unsigned short knock_sensor_pass = 0;
            
            LOGT(eSVC_MODEL, "[BCM KNOCK SENSOR] SETTING_VAL_CHECK_DONE\r\n");      

            if ( get_bcm_knocksensor_val_id_pass(&knock_sensor_id, &knock_sensor_pass) == KNOCKSENSOR_RET_SUCCESS )            
            {
                LOGT(eSVC_MODEL, " [BCM KNOCK SENSOR] SETTING_VAL_CHECK_DONE ok => id[0x%x], pass[0x%x]  \r\n", knock_sensor_id, knock_sensor_pass);
            
                allkey_bcm_ctr__knocksensor_set_passwd(knock_sensor_pass);
                allkey_bcm_ctr__knocksensor_set_id(knock_sensor_id);
                allkey_bcm_ctr__knocksensor_set_modemtime();
            }
            else
                LOGE(eSVC_MODEL, " [BCM KNOCK SENSOR] SETTING_VAL_CHECK_DONE nok => id[0x%x], pass[0x%x]  \r\n", knock_sensor_id, knock_sensor_pass);

            knocksensor_setting_result = KNOCKSENSOR_SETTING__BCM_DO_SETTING;

            break;
        }
        case KNOCKSENSOR_SETTING__SETTING_VAL_ID:
        {
            unsigned short knock_sensor_id = 0;

            if ( get_bcm_knocksensor_val_id(&knock_sensor_id) == KNOCKSENSOR_RET_SUCCESS )
            {
                allkey_bcm_ctr__knocksensor_set_id(knock_sensor_id);
                LOGE(eSVC_MODEL, " [BCM KNOCK SENSOR] KNOCKSENSOR_SETTING__SETTING_VAL_ID ok => id[0x%x], pass[0x%x]  \r\n", knock_sensor_id, 0);                
            }
            else
            {
                LOGE(eSVC_MODEL, " [BCM KNOCK SENSOR] KNOCKSENSOR_SETTING__SETTING_VAL_ID nok => id[0x%x], pass[0x%x]  \r\n", knock_sensor_id, 0);                
            }
            
            knocksensor_setting_result = KNOCKSENSOR_SETTING__BCM_DONE;

            break;
        }
        case KNOCKSENSOR_SETTING__SETTING_VAL_PASS:
        {
            unsigned short knock_sensor_pass = 0;            

            if ( get_bcm_knocksensor_val_pass(&knock_sensor_pass) == KNOCKSENSOR_RET_SUCCESS )
            {
                allkey_bcm_ctr__knocksensor_set_passwd(knock_sensor_pass);
                LOGT(eSVC_MODEL, " [BCM KNOCK SENSOR] KNOCKSENSOR_SETTING__SETTING_VAL_PASS ok => id[0x%x], pass[0x%x]  \r\n", 0, knock_sensor_pass);                
            }
            else
            {
                LOGE(eSVC_MODEL, " [BCM KNOCK SENSOR] KNOCKSENSOR_SETTING__SETTING_VAL_PASS nok => id[0x%x], pass[0x%x]  \r\n", 0, knock_sensor_pass);                
            }
            knocksensor_setting_result = KNOCKSENSOR_SETTING__BCM_DONE;

            break;
        }
        case KNOCKSENSOR_SETTING__BCM_DO_SETTING:
        {
            knocksensor_setting_result = KNOCKSENSOR_SETTING__BCM_DONE;
            break;
        }
        case KNOCKSENSOR_SETTING__BCM_DONE:
        {
            LOGT(eSVC_MODEL, "[BCM KNOCK SENSOR] KNOCK SENSOR DONE\r\n");            
            break;
        }
        default : 
            break;
    }
    return knocksensor_setting_result;
}

int set_bcm_knocksensor_setting(int flag)
{
    knocksensor_setting_result = flag;
    return knocksensor_setting_result;
}

static unsigned short knock_sensor_id = -1;
static unsigned short knock_sensor_master_number = -1;

int set_bcm_knocksensor_val_id_pass(unsigned short id, unsigned short master_number)
{
    int ret = KNOCKSENSOR_RET_SUCCESS;

    if ( id >= 0 )
        knock_sensor_id = id;
    else
        ret = KNOCKSENSOR_RET_FAIL;

    if ( master_number >= 0 )
        knock_sensor_master_number = master_number;
    else
        ret = KNOCKSENSOR_RET_FAIL;

    if ( ret == KNOCKSENSOR_RET_SUCCESS )
        set_bcm_knocksensor_setting(KNOCKSENSOR_SETTING__SETTING_VAL_CHECK_DONE);
    else
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__, id, master_number);

    return ret;
}

int get_bcm_knocksensor_val_id_pass(unsigned short* id, unsigned short* master_number)
{
    int ret_val = KNOCKSENSOR_RET_SUCCESS;

    if (( knock_sensor_id >= 0 ) && ( knock_sensor_master_number >= 0 ))
    {
        *id = knock_sensor_id;
        *master_number = knock_sensor_master_number;
        ret_val = KNOCKSENSOR_RET_SUCCESS;
    }
    else
    {
        *id = -1;
        *master_number = KNOCKSENSOR_RET_FAIL;
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__, knock_sensor_id, knock_sensor_master_number);
    }
    return ret_val;
}


int set_bcm_knocksensor_val_id(unsigned short id)
{
    int ret = KNOCKSENSOR_RET_SUCCESS;

    if ( id >= 0 )
    {
        knock_sensor_id = id;
        set_bcm_knocksensor_setting(KNOCKSENSOR_SETTING__SETTING_VAL_ID);
        ret = KNOCKSENSOR_RET_SUCCESS;
    }
    else
    {
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__, id, 0);
        ret = KNOCKSENSOR_RET_FAIL;
    }
    
    return ret;
}

int get_bcm_knocksensor_val_id(unsigned short* id)
{
    int ret_val = KNOCKSENSOR_RET_SUCCESS;

    if ( knock_sensor_id >= 0 )
    {
        *id = knock_sensor_id;
        ret_val = KNOCKSENSOR_RET_SUCCESS;
    }
    else
    {
        *id = -1;
        ret_val = KNOCKSENSOR_RET_FAIL;
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__, knock_sensor_id, 0);
    }
    return ret_val;
}

int set_bcm_knocksensor_val_pass(unsigned short master_number)
{
    int ret = KNOCKSENSOR_RET_SUCCESS;

    if ( master_number >= 0 )
    {
        knock_sensor_master_number = master_number;
        set_bcm_knocksensor_setting(KNOCKSENSOR_SETTING__SETTING_VAL_PASS);
        ret = KNOCKSENSOR_RET_SUCCESS;
    }
    else
    {
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__, 0, master_number);
        ret = KNOCKSENSOR_RET_FAIL;
    }
    
    return ret;
}

int get_bcm_knocksensor_val_pass(unsigned short* master_number)
{
    int ret_val = KNOCKSENSOR_RET_SUCCESS;
    
    if ( knock_sensor_master_number >= 0 )
    {
        *master_number = knock_sensor_master_number;
        ret_val = KNOCKSENSOR_RET_SUCCESS;
    }
    else
    {
        *master_number = -1;
        ret_val = KNOCKSENSOR_RET_FAIL;
        devel_webdm_send_log("%s():%d fail [%d],[%d]", __func__, __LINE__,  0, knock_sensor_master_number);
    }

    return ret_val;
}

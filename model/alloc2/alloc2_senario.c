<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>
#include <time.h>

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

#include <base/gpstool.h>

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

void alloc2_poweroff_proc_2(char* msg) // immediately reset
{
    mileage_write();
	gps_valid_data_write();

	poweroff("net emer", strlen("net emer"));
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

#ifdef SERVER_ABBR_ALM1
	sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
	sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
#endif

#ifdef SERVER_ABBR_ALM2
	sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif

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

    //printf(" ----------> batt level is [%d] / [%d]\r\n", _g_car_batt_level, batt_chk_interval);

    // batt 값을 강제로 업데이트 할 경우다.
    if ( chk_flag == 1 )
        batt_chk_interval = BATT_CHK_INTERVAL_SEC;

    // 최초 실행시 유효값 얻는다.
    if ( ( _g_car_batt_level <= 0 ) || ( _g_car_batt_level > 420 ) )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        // check batt valid range..
        if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
            return 0;
        else
             _g_car_batt_level = car_voltage;
    }

    if ( batt_chk_interval++ >= BATT_CHK_INTERVAL_SEC )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        //car_voltage = car_voltage * 10;
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

            if ( get_low_batt_send_timing(low_batt) )
            {
#ifdef SERVER_ABBR_ALM1
                sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
                sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif

#ifdef SERVER_ABBR_ALM2
                sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif
            }
            evt_send_flag = 1;
        }
        else
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt skip...\r\n", car_voltage);
    }
    else if ( (low_batt > 0 ) && ( low_batt < _g_car_batt_level ) ) // evt clear condition
    {
        evt_send_flag = 0;
    }

    return 0;
}

int chk_powersave_mode(int start_voltage, int end_voltage)
{
    int cur_voltage = get_car_batt_level();
    static int initial_delay = 90;
    static int cur_mode = POWER_SAVE_MODE__NORMAL;
    static int last_mode = POWER_SAVE_MODE__INIT;

    static int run_cnt = 0;
    cur_mode = get_powersave_mode();

    // LOGT(eSVC_MODEL, "[PWR SAVE MODE] cur [%d] / last [%d]\r\n", cur_mode, last_mode);
    
    if ( initial_delay-- > 0)
    {
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] initial delay [%d]  \r\n", initial_delay);
        return 0;
    }

    if ((run_cnt ++ % 15 ) == 0)
        LOGT(eSVC_MODEL, "[PWR SAVE MODE] start_voltage [%d] / end_voltage [%d] / cur_voltage [%d]\r\n", start_voltage, end_voltage, cur_voltage);
    
    if ( (start_voltage <= 0 ) || (end_voltage <= 0) )
    {
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] invalid setting val return.. \r\n");
        return 0;
    }

    if ( ( cur_voltage >= end_voltage ) && (cur_mode == POWER_SAVE_MODE__POWER_SAVE) && (cur_mode != last_mode))
    {
        // normal mode 로 변경시에는 먼저 mode 변경후에 패킷을 보낸다.
        set_powersave_mode(POWER_SAVE_MODE__NORMAL);

        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");

        // SEND PKT 
        // - 주기보고 발송 => 정상 동작
        sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info,0));

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
        gps_set_deact_cnt(300);
#endif
        gps_reset_immediately(GPS_BOOT_COLD);

        last_mode = cur_mode;
    }

    if (( cur_voltage <= start_voltage ) && (cur_mode == POWER_SAVE_MODE__NORMAL) && (cur_mode != last_mode))
    {
        int evt_code = e_evt_code_powersave_mode_start;
        
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");

        // SEND PKT
        // - Power Save Mode 이벤트 1회 발송 
        sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));

        // power save mode 로 변경시에는 먼저 패킷을 변경한 후에 mode를 변경한다.
        set_powersave_mode(POWER_SAVE_MODE__POWER_SAVE);

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
        gps_set_deact_cnt(2147483647);
#endif
        gps_off();

        last_mode = cur_mode;
    }
    

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

int get_gpio_send_timing(int gpio)
{
    static time_t last_cycle = 0;
    static time_t last_send_time = 0;
    time_t cur_time = 0;

	struct timeval tv;
	struct tm ttm;

    static int evt_cnt = 0;
    static int evt_1min_flag = 0;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);
    cur_time = mktime(&ttm);

    ALLOC_PKT_RECV__MDM_SETTING_VAL* cur_mdm_setting = get_mdm_setting_val();

    int max_send_cnt = cur_mdm_setting->over_speed_limit_km;
    int reset_send_cnt_sec = cur_mdm_setting->over_speed_limit_time*60;

    //max_send_cnt = 3;
    //reset_send_cnt_sec = 2*60;
    max_send_cnt = max_send_cnt * 2; // pair evt

    LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => debug : max_send_cnt [%d] / reset_send_cnt_sec [%d] / evt_1min_flag [%d] \r\n",max_send_cnt, reset_send_cnt_sec, evt_1min_flag);

    // 30 sec interval network chk
    // 네트워크 쓰레드의 경우 불리는 주기가 불규칙, 때문에 시간계산하여 30초마다 한번씩 불리도록

    if ( evt_1min_flag == 1 )
    {
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => evt over debug : difftime [%d]/[%d]/ evt_cnt [%d]\r\n",(cur_time - last_cycle), reset_send_cnt_sec, evt_cnt);
        if( (cur_time - last_cycle) < reset_send_cnt_sec )
        {
            LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 0 set : retrun true [%d]\r\n",evt_cnt);
            return 0;
        }
        else
        {
            last_cycle = 0;
            evt_cnt = 0;
            evt_1min_flag = 0;
            LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 0 clear : retrun true [%d]\r\n",evt_cnt);
        }
    }

    if (cur_time == 0)
    {
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 1 : retrun true [%d]\r\n",evt_cnt);
        return 1;
    }
    
    if (last_cycle == 0)
    {
        last_cycle = cur_time;
        evt_cnt++;

        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => reset cnt : retrun true [%d]\r\n",evt_cnt);

        return 1;
    }

    // 1분동안 들어온 이벤트의 갯수를 카운트한다.
    if( (cur_time - last_cycle) < 60 )
    {
        evt_cnt++;
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt inc : [%d]\r\n",evt_cnt);
    }
    else
    {
        last_cycle = cur_time;
        evt_cnt = 0;
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt reset : [%d]\r\n",evt_cnt);
    }

    LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => debug : evt_cnt [%d] / evt_1min_flag [%d] / difftime [%d]/[%d]\r\n",evt_cnt, evt_1min_flag, (cur_time - last_cycle), max_send_cnt);

    if ( evt_cnt > max_send_cnt)
    {
        LOGI(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt over! cnt retry time : [%d]\r\n",evt_cnt);
        evt_1min_flag = 1;
        return 0;
    }
    else
    {
        LOGI(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt under.. clear : [%d]\r\n",evt_cnt);
        evt_1min_flag = 0;
        last_cycle = cur_time;
        return 1;
    }


    return 1;
}

int get_low_batt_send_timing(int batt_level)
{
    ALLOC_PKT_RECV__MDM_SETTING_VAL* cur_mdm_setting = get_mdm_setting_val();

    return 1;
}

static int _g_cur_power_save_mode = POWER_SAVE_MODE__NORMAL;
int set_powersave_mode(int flag)
{
    if (flag == POWER_SAVE_MODE__POWER_SAVE)
    {
        _g_cur_power_save_mode = POWER_SAVE_MODE__POWER_SAVE;
        // NETWORK THREAD 에서 power save mode 일경우 패킷전송후에 rf 를 끈다.
        /*
        nettool_set_state(NET_TOOL_SET__DISABLE);
        sleep(10);
        nettool_set_rf_pwr(NET_TOOL_SET__RF_DISABLE);
        sleep(10);
        */

    }
    else
    {
        _g_cur_power_save_mode = POWER_SAVE_MODE__NORMAL;
    }
    return _g_cur_power_save_mode;
}

int get_powersave_mode()
{
    return _g_cur_power_save_mode;
}
=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>
#include <time.h>

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

#include <base/gpstool.h>

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

void alloc2_poweroff_proc_2(char* msg) // immediately reset
{
    mileage_write();
	gps_valid_data_write();

	poweroff("net emer", strlen("net emer"));
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

#ifdef SERVER_ABBR_ALM1
	sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
	sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
#endif

#ifdef SERVER_ABBR_ALM2
	sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif

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

    //printf(" ----------> batt level is [%d] / [%d]\r\n", _g_car_batt_level, batt_chk_interval);

    // batt 값을 강제로 업데이트 할 경우다.
    if ( chk_flag == 1 )
        batt_chk_interval = BATT_CHK_INTERVAL_SEC;

    // 최초 실행시 유효값 얻는다.
    if ( ( _g_car_batt_level <= 0 ) || ( _g_car_batt_level > 420 ) )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        // check batt valid range..
        if ( ( car_voltage <= 0 ) || ( car_voltage > 420 ) )
            return 0;
        else
             _g_car_batt_level = car_voltage;
    }

    if ( batt_chk_interval++ >= BATT_CHK_INTERVAL_SEC )
    {
        get_adc_main_pwr2_tl500(&car_voltage);
        //car_voltage = car_voltage * 10;
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

            if ( get_low_batt_send_timing(low_batt) )
            {
#ifdef SERVER_ABBR_ALM1
                sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
                sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif

#ifdef SERVER_ABBR_ALM2
                sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
#endif
            }
            evt_send_flag = 1;
        }
        else
            LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! voltage [%d] send evt skip...\r\n", car_voltage);
    }
    else if ( (low_batt > 0 ) && ( low_batt < _g_car_batt_level ) ) // evt clear condition
    {
        evt_send_flag = 0;
    }

    return 0;
}

int chk_powersave_mode(int start_voltage, int end_voltage)
{
    int cur_voltage = get_car_batt_level();
    static int initial_delay = 90;
    static int cur_mode = POWER_SAVE_MODE__NORMAL;
    static int last_mode = POWER_SAVE_MODE__INIT;

    static int run_cnt = 0;
    cur_mode = get_powersave_mode();

    // LOGT(eSVC_MODEL, "[PWR SAVE MODE] cur [%d] / last [%d]\r\n", cur_mode, last_mode);
    
    if ( initial_delay-- > 0)
    {
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] initial delay [%d]  \r\n", initial_delay);
        return 0;
    }

    if ((run_cnt ++ % 15 ) == 0)
        LOGT(eSVC_MODEL, "[PWR SAVE MODE] start_voltage [%d] / end_voltage [%d] / cur_voltage [%d]\r\n", start_voltage, end_voltage, cur_voltage);
    
    if ( (start_voltage <= 0 ) || (end_voltage <= 0) )
    {
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] invalid setting val return.. \r\n");
        return 0;
    }

    if ( ( cur_voltage >= end_voltage ) && (cur_mode == POWER_SAVE_MODE__POWER_SAVE) && (cur_mode != last_mode))
    {
        // normal mode 로 변경시에는 먼저 mode 변경후에 패킷을 보낸다.
        set_powersave_mode(POWER_SAVE_MODE__NORMAL);

        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change normal mode \r\n");

        // SEND PKT 
        // - 주기보고 발송 => 정상 동작
        sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info,0));

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
        gps_set_deact_cnt(300);
#endif
        gps_reset_immediately(GPS_BOOT_COLD);

        last_mode = cur_mode;
    }

    if (( cur_voltage <= start_voltage ) && (cur_mode == POWER_SAVE_MODE__NORMAL) && (cur_mode != last_mode))
    {
        int evt_code = e_evt_code_powersave_mode_start;
        
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");
        LOGE(eSVC_MODEL, "[PWR SAVE MODE] change save mode \r\n");

        // SEND PKT
        // - Power Save Mode 이벤트 1회 발송 
        sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));

        // power save mode 로 변경시에는 먼저 패킷을 변경한 후에 mode를 변경한다.
        set_powersave_mode(POWER_SAVE_MODE__POWER_SAVE);

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
        gps_set_deact_cnt(2147483647);
#endif
        gps_off();

        last_mode = cur_mode;
    }
    

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

int get_gpio_send_timing(int gpio)
{
    static time_t last_cycle = 0;
    static time_t last_send_time = 0;
    time_t cur_time = 0;

	struct timeval tv;
	struct tm ttm;

    static int evt_cnt = 0;
    static int evt_1min_flag = 0;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);
    cur_time = mktime(&ttm);

    ALLOC_PKT_RECV__MDM_SETTING_VAL* cur_mdm_setting = get_mdm_setting_val();

    int max_send_cnt = cur_mdm_setting->over_speed_limit_km;
    int reset_send_cnt_sec = cur_mdm_setting->over_speed_limit_time*60;

    //max_send_cnt = 3;
    //reset_send_cnt_sec = 2*60;
    max_send_cnt = max_send_cnt * 2; // pair evt

    LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => debug : max_send_cnt [%d] / reset_send_cnt_sec [%d] / evt_1min_flag [%d] \r\n",max_send_cnt, reset_send_cnt_sec, evt_1min_flag);

    // 30 sec interval network chk
    // 네트워크 쓰레드의 경우 불리는 주기가 불규칙, 때문에 시간계산하여 30초마다 한번씩 불리도록

    if ( evt_1min_flag == 1 )
    {
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => evt over debug : difftime [%d]/[%d]/ evt_cnt [%d]\r\n",(cur_time - last_cycle), reset_send_cnt_sec, evt_cnt);
        if( (cur_time - last_cycle) < reset_send_cnt_sec )
        {
            LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 0 set : retrun true [%d]\r\n",evt_cnt);
            return 0;
        }
        else
        {
            last_cycle = 0;
            evt_cnt = 0;
            evt_1min_flag = 0;
            LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 0 clear : retrun true [%d]\r\n",evt_cnt);
        }
    }

    if (cur_time == 0)
    {
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => err case 1 : retrun true [%d]\r\n",evt_cnt);
        return 1;
    }
    
    if (last_cycle == 0)
    {
        last_cycle = cur_time;
        evt_cnt++;

        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => reset cnt : retrun true [%d]\r\n",evt_cnt);

        return 1;
    }

    // 1분동안 들어온 이벤트의 갯수를 카운트한다.
    if( (cur_time - last_cycle) < 60 )
    {
        evt_cnt++;
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt inc : [%d]\r\n",evt_cnt);
    }
    else
    {
        last_cycle = cur_time;
        evt_cnt = 0;
        LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt reset : [%d]\r\n",evt_cnt);
    }

    LOGT(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => debug : evt_cnt [%d] / evt_1min_flag [%d] / difftime [%d]/[%d]\r\n",evt_cnt, evt_1min_flag, (cur_time - last_cycle), max_send_cnt);

    if ( evt_cnt > max_send_cnt)
    {
        LOGI(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt over! cnt retry time : [%d]\r\n",evt_cnt);
        evt_1min_flag = 1;
        return 0;
    }
    else
    {
        LOGI(eSVC_MODEL, "[GPS THREAD] get_gpio_send_timing => 1min cnt under.. clear : [%d]\r\n",evt_cnt);
        evt_1min_flag = 0;
        last_cycle = cur_time;
        return 1;
    }


    return 1;
}

int get_low_batt_send_timing(int batt_level)
{
    ALLOC_PKT_RECV__MDM_SETTING_VAL* cur_mdm_setting = get_mdm_setting_val();

    return 1;
}

static int _g_cur_power_save_mode = POWER_SAVE_MODE__NORMAL;
int set_powersave_mode(int flag)
{
    if (flag == POWER_SAVE_MODE__POWER_SAVE)
    {
        _g_cur_power_save_mode = POWER_SAVE_MODE__POWER_SAVE;
        // NETWORK THREAD 에서 power save mode 일경우 패킷전송후에 rf 를 끈다.
        /*
        nettool_set_state(NET_TOOL_SET__DISABLE);
        sleep(10);
        nettool_set_rf_pwr(NET_TOOL_SET__RF_DISABLE);
        sleep(10);
        */

    }
    else
    {
        _g_cur_power_save_mode = POWER_SAVE_MODE__NORMAL;
    }
    return _g_cur_power_save_mode;
}

int get_powersave_mode()
{
    return _g_cur_power_save_mode;
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

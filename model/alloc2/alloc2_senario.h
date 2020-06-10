<<<<<<< HEAD
#ifndef __ALLOC2_SENARIO_H__
#define __ALLOC2_SENARIO_H__

#include "alloc2_pkt.h"
#include "board/board_system.h"

#ifdef PKT_VER_POWERSAVE_MODE
#define ALLOC2_MDM_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_mdm_setting_v16_p.dat")
#define ALLOC2_OBD_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_obd_setting_v16_p.dat")
#else
#define ALLOC2_MDM_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_mdm_setting_v16.dat")
#define ALLOC2_OBD_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_obd_setting_v16.dat")
#endif

typedef enum {
    e_STAT_NONE,
    e_STAT_START = e_STAT_NONE,
    e_SEND_TO_SETTING_INFO = e_STAT_NONE,
    e_SEND_TO_SETTING_INFO_ING,
    e_SEND_TO_SETTING_INFO_COMPLETE,
    e_SEND_TO_ONLY_GPS_DATA = e_SEND_TO_SETTING_INFO_COMPLETE,
    e_SEND_TO_OBD_INFO,
    e_SEND_TO_OBD_INFO_ING,
    e_SEND_TO_OBD_INFO_COMPLETE,
    e_SEND_TO_ONLY_OBD_DATA = e_SEND_TO_OBD_INFO_COMPLETE,
    e_SEND_REPORT_RUN,
} e_ALLOC2_SENARIO_STAT;


int get_cur_status();
int set_cur_status(e_ALLOC2_SENARIO_STAT stat);

int init_mdm_setting_pkt_val();
int set_mdm_setting_pkt_val(ALLOC_PKT_RECV__MDM_SETTING_VAL* setting_val);
ALLOC_PKT_RECV__MDM_SETTING_VAL* get_mdm_setting_val();

int init_obd_dev_pkt_info();
int set_obd_dev_pkt_info(ALLOC_PKT_RECV__OBD_DEV_INFO* setting_val);
ALLOC_PKT_RECV__OBD_DEV_INFO* get_obd_dev_info();

int get_sms_pkt_cmd_code(unsigned char code);

void alloc2_poweroff_proc(char* msg);
void alloc2_poweroff_proc_2(char* msg); // immediately reset


#define CAR_CTRL_ENABLE     1
#define CAR_CTRL_DISABLE    -1
int set_car_ctrl_enable(int flag);
int get_car_ctrl_enable();


#define SEND_TO_PWR_EVT_OK      1
#define SEND_TO_PWR_EVT_NOK     -1

#define EVT_TYPE_POWER_ON       1
#define EVT_TYPE_POWER_OFF      2
#define EVT_TYPE_IGI_ON         3
#define EVT_TYPE_IGI_OFF        4

#define NO_SEND_TO_PWR_EVT_FLAG_PATH    CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_v16.dat")
#define NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH      CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_info_v16.dat")
#define NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH_2    CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_info_v16.dat.bak")

int set_no_send_pwr_evt_reboot();
int get_no_send_pwr_evt_reboot(int flag);

#define CHK_SMS_INTERVAL_SEC   15
int chk_read_sms();

#define BATT_CHK_INTERVAL_SEC 			60
int get_car_batt_level();
int chk_car_batt_level(int low_batt, int chk_flag);

#define KNOCKSENSOR_RET_SUCCESS     1
#define KNOCKSENSOR_RET_FAIL        -1

#define KNOCKSENSOR_SETTING__INIT                0
#define KNOCKSENSOR_SETTING__SETTING_VAL_CHECKING       1
#define KNOCKSENSOR_SETTING__SETTING_VAL_CHECK_DONE     2
#define KNOCKSENSOR_SETTING__SETTING_VAL_ID             4
#define KNOCKSENSOR_SETTING__SETTING_VAL_PASS           5
#define KNOCKSENSOR_SETTING__BCM_DO_SETTING             6
#define KNOCKSENSOR_SETTING__BCM_DONE                   7

int chk_bcm_knocksensor_setting();
int set_bcm_knocksensor_setting(int flag);

int set_bcm_knocksensor_val_id_pass(unsigned short id, unsigned short master_number);
int get_bcm_knocksensor_val_id_pass(unsigned short* id, unsigned short* master_number);
int set_bcm_knocksensor_val_id(unsigned short id);
int get_bcm_knocksensor_val_id(unsigned short* id);
int set_bcm_knocksensor_val_pass(unsigned short master_number);
int get_bcm_knocksensor_val_pass(unsigned short* master_number);

int set_bcm_knocksensor_val(unsigned short id, unsigned short master_number);
int get_bcm_knocksensor_val(unsigned short* id, unsigned short* master_number);

int get_gpio_send_timing(int gpio);
int get_low_batt_send_timing(int batt_level);

#define POWER_SAVE_MODE__INIT           0
#define POWER_SAVE_MODE__POWER_SAVE     1
#define POWER_SAVE_MODE__NORMAL         2

int set_powersave_mode(int flag);
int get_powersave_mode();
int chk_powersave_mode(int start_voltage, int end_voltage);
int clr_no_send_pwr_evt_reboot();

#endif // __ALLOC2_SENARIO_H__

=======
#ifndef __ALLOC2_SENARIO_H__
#define __ALLOC2_SENARIO_H__

#include "alloc2_pkt.h"
#include "board/board_system.h"

#ifdef PKT_VER_POWERSAVE_MODE
#define ALLOC2_MDM_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_mdm_setting_v16_p.dat")
#define ALLOC2_OBD_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_obd_setting_v16_p.dat")
#else
#define ALLOC2_MDM_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_mdm_setting_v16.dat")
#define ALLOC2_OBD_SETTING_INFO CONCAT_STR(USER_DATA_DIR, "/alloc_obd_setting_v16.dat")
#endif

typedef enum {
    e_STAT_NONE,
    e_STAT_START = e_STAT_NONE,
    e_SEND_TO_SETTING_INFO = e_STAT_NONE,
    e_SEND_TO_SETTING_INFO_ING,
    e_SEND_TO_SETTING_INFO_COMPLETE,
    e_SEND_TO_ONLY_GPS_DATA = e_SEND_TO_SETTING_INFO_COMPLETE,
    e_SEND_TO_OBD_INFO,
    e_SEND_TO_OBD_INFO_ING,
    e_SEND_TO_OBD_INFO_COMPLETE,
    e_SEND_TO_ONLY_OBD_DATA = e_SEND_TO_OBD_INFO_COMPLETE,
    e_SEND_REPORT_RUN,
} e_ALLOC2_SENARIO_STAT;


int get_cur_status();
int set_cur_status(e_ALLOC2_SENARIO_STAT stat);

int init_mdm_setting_pkt_val();
int set_mdm_setting_pkt_val(ALLOC_PKT_RECV__MDM_SETTING_VAL* setting_val);
ALLOC_PKT_RECV__MDM_SETTING_VAL* get_mdm_setting_val();

int init_obd_dev_pkt_info();
int set_obd_dev_pkt_info(ALLOC_PKT_RECV__OBD_DEV_INFO* setting_val);
ALLOC_PKT_RECV__OBD_DEV_INFO* get_obd_dev_info();

int get_sms_pkt_cmd_code(unsigned char code);

void alloc2_poweroff_proc(char* msg);
void alloc2_poweroff_proc_2(char* msg); // immediately reset


#define CAR_CTRL_ENABLE     1
#define CAR_CTRL_DISABLE    -1
int set_car_ctrl_enable(int flag);
int get_car_ctrl_enable();


#define SEND_TO_PWR_EVT_OK      1
#define SEND_TO_PWR_EVT_NOK     -1

#define EVT_TYPE_POWER_ON       1
#define EVT_TYPE_POWER_OFF      2
#define EVT_TYPE_IGI_ON         3
#define EVT_TYPE_IGI_OFF        4

#define NO_SEND_TO_PWR_EVT_FLAG_PATH    CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_v16.dat")
#define NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH      CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_info_v16.dat")
#define NO_SEND_TO_PWR_EVT_SAVE_INFO_PATH_2    CONCAT_STR(USER_DATA_DIR, "/no_send_pwr_info_v16.dat.bak")

int set_no_send_pwr_evt_reboot();
int get_no_send_pwr_evt_reboot(int flag);

#define CHK_SMS_INTERVAL_SEC   15
int chk_read_sms();

#define BATT_CHK_INTERVAL_SEC 			60
int get_car_batt_level();
int chk_car_batt_level(int low_batt, int chk_flag);

#define KNOCKSENSOR_RET_SUCCESS     1
#define KNOCKSENSOR_RET_FAIL        -1

#define KNOCKSENSOR_SETTING__INIT                0
#define KNOCKSENSOR_SETTING__SETTING_VAL_CHECKING       1
#define KNOCKSENSOR_SETTING__SETTING_VAL_CHECK_DONE     2
#define KNOCKSENSOR_SETTING__SETTING_VAL_ID             4
#define KNOCKSENSOR_SETTING__SETTING_VAL_PASS           5
#define KNOCKSENSOR_SETTING__BCM_DO_SETTING             6
#define KNOCKSENSOR_SETTING__BCM_DONE                   7

int chk_bcm_knocksensor_setting();
int set_bcm_knocksensor_setting(int flag);

int set_bcm_knocksensor_val_id_pass(unsigned short id, unsigned short master_number);
int get_bcm_knocksensor_val_id_pass(unsigned short* id, unsigned short* master_number);
int set_bcm_knocksensor_val_id(unsigned short id);
int get_bcm_knocksensor_val_id(unsigned short* id);
int set_bcm_knocksensor_val_pass(unsigned short master_number);
int get_bcm_knocksensor_val_pass(unsigned short* master_number);

int set_bcm_knocksensor_val(unsigned short id, unsigned short master_number);
int get_bcm_knocksensor_val(unsigned short* id, unsigned short* master_number);

int get_gpio_send_timing(int gpio);
int get_low_batt_send_timing(int batt_level);

#define POWER_SAVE_MODE__INIT           0
#define POWER_SAVE_MODE__POWER_SAVE     1
#define POWER_SAVE_MODE__NORMAL         2

int set_powersave_mode(int flag);
int get_powersave_mode();
int chk_powersave_mode(int start_voltage, int end_voltage);
int clr_no_send_pwr_evt_reboot();

#endif // __ALLOC2_SENARIO_H__

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

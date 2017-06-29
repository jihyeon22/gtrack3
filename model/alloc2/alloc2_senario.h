#ifndef __ALLOC2_SENARIO_H__
#define __ALLOC2_SENARIO_H__

#include "alloc2_pkt.h"

#define ALLOC2_MDM_SETTING_INFO "/data/mds/data/alloc_mdm_setting.dat"
#define ALLOC2_OBD_SETTING_INFO "/data/mds/data/alloc_obd_setting.dat"


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

int chk_keyon_section_distance(int total_distance);
int init_keyon_section_distance(int total_distance);

void alloc2_poweroff_proc(char* msg);


#endif // __ALLOC2_SENARIO_H__


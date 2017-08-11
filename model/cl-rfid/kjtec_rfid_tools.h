#ifndef __KJTEC_RFID_MGR_H__
#define __KJTEC_RFID_MGR_H__


#include "kjtec_rfid_cmd.h"
#define FW_DOWNLOAD_OFFSET_BYTE     4
#define FW_DOWNLOAD_MAX_FAIL_RETRY_CNT   RFID_CMD_FIRMWARE_ONE_PKT_MAX_RETRY
#define FW_DOWNLOAD_FILE_PATH           "/system/mds/system/bin/rfid_fw.bin"
#define FW_DOWNLOAD_FILE_VER            "1.3.0.BusSR-t9"


int kjtec_rfid_mgr__dev_init_chk(RFID_DEV_INFO_T* info);

int kjtec_rfid_mgr__write_to_dev_user_info(int all_erase);
int kjtec_rfid_mgr__write_user_info();

int kjtec_rfid_mgr__clr_all_user_data();

int kjtec_rfid_mgr__alive_dev();

int kjtec_rfid_mgr__download_fw(char* path);
int kjtec_rfid_mgr__download_sms_noti_enable(int enable, char* phone_num);
int kjtec_rfid_mgr__download_sms_noti_msg(char* msg);

extern int g_need_to_rfid_ver_chk;


#endif

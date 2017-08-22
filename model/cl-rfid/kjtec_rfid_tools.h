#ifndef __KJTEC_RFID_MGR_H__
#define __KJTEC_RFID_MGR_H__


#include "kjtec_rfid_cmd.h"
#define FW_DOWNLOAD_OFFSET_BYTE     4
#define FW_DOWNLOAD_MAX_FAIL_RETRY_CNT   RFID_CMD_FIRMWARE_ONE_PKT_MAX_RETRY
#define FW_DOWNLOAD_FILE_PATH           "/system/mds/system/bin"
// full path : "/system/mds/system/bin/rfid_fw_%s.bin"
#define FW_DOWNLOAD_FILE_LAST_VER_STR   "1.3.0.BusSR-t11"
#define FW_DOWNLOAD_FILE_LAST_VER_NUM   "t11"

int kjtec_rfid_mgr__dev_init_chk(RFID_DEV_INFO_T* info);

int kjtec_rfid_mgr__write_to_dev_user_info(int all_erase);
int kjtec_rfid_mgr__write_user_info();

int kjtec_rfid_mgr__clr_all_user_data();

int kjtec_rfid_mgr__alive_dev();

int kjtec_rfid_mgr__download_fw(char* path);
int kjtec_rfid_mgr__download_sms_noti_enable(int enable, char* phone_num);
int kjtec_rfid_mgr__download_sms_noti_msg(char* msg);

int set_fwdown_chksum_offset(int offset);
int get_fwdown_chksum_offset();

extern int g_need_to_rfid_ver_chk;

#define FW_NAME_MAX_LEN     512
int set_fwdown_target_ver(char* buff);
int get_fwdown_target_ver(char* buff);
int get_fwdown_target_path(char* ver);

#endif

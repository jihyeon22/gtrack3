#ifndef __KJTEC_RFID_MGR_H__
#define __KJTEC_RFID_MGR_H__


#include "ext/rfid/kjtec_rfid/kjtec_rfid_cmd.h"
#define FW_DOWNLOAD_OFFSET_BYTE     4
#define FW_DOWNLOAD_MAX_FAIL_RETRY_CNT   RFID_CMD_FIRMWARE_ONE_PKT_MAX_RETRY
#define FW_DOWNLOAD_FILE_PATH           "/system/mds/system/bin"
// full path : "/system/mds/system/bin/rfid_fw_%s.bin"

// hundai auto bus server model.
#if defined(SERVER_ABBR_CLR1) || defined(SERVER_ABBR_CLRA1) || defined(SERVER_ABBR_CLRB1)
#define FW_DOWNLOAD_FILE_LAST_VER_STR   "1.3.4ABUS"
#define FW_DOWNLOAD_FILE_LAST_VER_NUM   "134A"
// cl server model.
#elif defined(SERVER_ABBR_CLR0) || defined(SERVER_ABBR_CLRA0) || defined(SERVER_ABBR_CLRB0) || defined(SERVER_ABBR_CLR2)
#define FW_DOWNLOAD_FILE_LAST_VER_STR   "1.3.4r2 KAL"
#define FW_DOWNLOAD_FILE_LAST_VER_NUM   "134r2H"
#else // default model firmware
#define FW_DOWNLOAD_FILE_LAST_VER_STR   "1.3.4HBUS"
#define FW_DOWNLOAD_FILE_LAST_VER_NUM   "134H"
#endif // SERVER_ABBR_CLR0

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
extern int g_need_to_rfid_info;

#define FW_NAME_MAX_LEN     512
int set_fwdown_target_ver(char* buff);
int get_fwdown_target_ver(char* buff);
int get_fwdown_target_path(char* ver);

int mds_api_remove_etc_char(const char *s, char* target, int target_len);

#endif

#ifndef __KJTEC_RFID_MGR_H__
#define __KJTEC_RFID_MGR_H__


#include "kjtec_rfid_cmd.h"

int kjtec_rfid_mgr__dev_init_chk(RFID_DEV_INFO_T* info);

int kjtec_rfid_mgr__write_to_dev_user_info(int all_erase);
int kjtec_rfid_mgr__write_user_info();

int kjtec_rfid_mgr__clr_all_user_data();

int kjtec_rfid_mgr__alive_dev();

int kjtec_rfid_mgr__download_fw(char* path);



#endif

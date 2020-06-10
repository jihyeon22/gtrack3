#ifndef __BASE_KT_FOTA_H__
#define __BASE_KT_FOTA_H__

#include <board/board_system.h>

//jwrho persistant data path modify++
//#define PATH_FOTA_DATA "/data/mds/data/fota.dat"
#define PATH_FOTA_DATA CONCAT_STR(USER_DATA_DIR, "/fota.dat")
//jwrho persistant data path modify--


void kt_fota_init(void);
void kt_fota_send(void);
void kt_fota_deinit(void);
int kt_fota_check_cycle(void);
void set_kt_fota_req_report(int val);
void set_kt_fota_qry_report(int val);
int KT_FOTA_NOTI_RECEIVE(char *buf);
void check_atl_buffer_data(char *buffer);

#endif

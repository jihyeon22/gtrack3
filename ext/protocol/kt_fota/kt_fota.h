#ifndef __BASE_KT_FOTA_H__
#define __BASE_KT_FOTA_H__

#define PATH_FOTA_DATA "/data/mds/data/fota.dat"


void kt_fota_init(void);
void kt_fota_send(void);
void kt_fota_deinit(void);
int kt_fota_check_cycle(void);
void set_kt_fota_req_report(int val);
void set_kt_fota_qry_report(int val);
int KT_FOTA_NOTI_RECEIVE(char *buf);
void check_atl_buffer_data(char *buffer);

#endif

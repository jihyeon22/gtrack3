#ifndef __BASE_DEVEL_H__
#define __BASE_DEVEL_H__

int devel_webdm_send_status_current(const int no_event);
int devel_webdm_send_log(const char *format, ...);
int devel_send_sms_noti(const char* buff, const int buff_len, int do_wait);
void devel_log_poweroff(const char *log, const int log_len);

#endif

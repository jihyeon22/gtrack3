#ifndef __UTIL_WEBDM_H__
#define __UTIL_WEBDM_H__

int webdm_register(const char* imei, const char* imsi, const char* ip, const int port);
int webdm_status(const char* imei, const char* imsi, const char* ip, const int no_event, const int port, const float lat, const float lon, const int ingn, const int dist);
int webdm_log(const char* phonenum, const char* imei, const char* model, const int  type, const char* log);

#endif

<<<<<<< HEAD
#ifndef __UTIL_VALIDATION_H__
#define __UTIL_VALIDATION_H__

#include <include/defines.h>

mdsReturn_t validation_check_phonenum(const char *buff, const int buff_len);
mdsReturn_t validation_check_imei(const char *buff, const int buff_len);
mdsReturn_t validation_check_ip(const char *buff, const int buff_len);
mdsReturn_t validation_check_dns_addr(const char *buff, const int buff_len);
mdsReturn_t validation_check_apn_addr(const char *buff, const int buff_len);
mdsReturn_t validation_check_is_num(const char *buff, const int buff_len);
mdsReturn_t validation_check_lat_lon(float lat, float lon);

#endif

=======
#ifndef __UTIL_VALIDATION_H__
#define __UTIL_VALIDATION_H__

#include <include/defines.h>

mdsReturn_t validation_check_phonenum(const char *buff, const int buff_len);
mdsReturn_t validation_check_imei(const char *buff, const int buff_len);
mdsReturn_t validation_check_ip(const char *buff, const int buff_len);
mdsReturn_t validation_check_dns_addr(const char *buff, const int buff_len);
mdsReturn_t validation_check_apn_addr(const char *buff, const int buff_len);
mdsReturn_t validation_check_is_num(const char *buff, const int buff_len);
mdsReturn_t validation_check_lat_lon(float lat, float lon);

#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

<<<<<<< HEAD
#ifndef __UTIL_NETTOOL_H__
#define __UTIL_NETTOOL_H__

#include <netinet/in.h>
#include <sys/types.h>

#include <include/defines.h>

mdsReturn_t nettool_get_state(void);
mdsReturn_t nettool_get_ip(const char* netif_buff, const int netif_buff_len, char *ip_buff, const int ip_buff_len);
mdsReturn_t nettool_get_domain_to_ip(const char *dn_buff, const int dn_buff_len, char *ip_buff, const int ip_buff_len);
int nettool_connect_timeo(int sock, const struct sockaddr *addr, socklen_t addr_len, const unsigned int timeout);
ssize_t nettool_send_timedwait(int sockfd, const void *buf, size_t len, int flags, int sec);
ssize_t nettool_recv_timedwait(int sockfd, void *buf, size_t len, int flags, int sec);
unsigned long int nettool_get_host_name(const char *host_name);
void nettool_init_hostbyname_func(void);

#define NET_TOOL_SET__ENABLE    0
#define NET_TOOL_SET__DISABLE    1
#define NET_TOOL_SET__MAX_CHK_TIME_SEC 10
mdsReturn_t nettool_set_state(int flag);

#define NET_TOOL_SET__RF_ENABLE    0
#define NET_TOOL_SET__RF_DISABLE    1
mdsReturn_t nettool_set_rf_pwr(int flag);


#endif
=======
#ifndef __UTIL_NETTOOL_H__
#define __UTIL_NETTOOL_H__

#include <netinet/in.h>
#include <sys/types.h>

#include <include/defines.h>

mdsReturn_t nettool_get_state(void);
mdsReturn_t nettool_get_ip(const char* netif_buff, const int netif_buff_len, char *ip_buff, const int ip_buff_len);
mdsReturn_t nettool_get_domain_to_ip(const char *dn_buff, const int dn_buff_len, char *ip_buff, const int ip_buff_len);
int nettool_connect_timeo(int sock, const struct sockaddr *addr, socklen_t addr_len, const unsigned int timeout);
ssize_t nettool_send_timedwait(int sockfd, const void *buf, size_t len, int flags, int sec);
ssize_t nettool_recv_timedwait(int sockfd, void *buf, size_t len, int flags, int sec);
unsigned long int nettool_get_host_name(const char *host_name);
void nettool_init_hostbyname_func(void);

#define NET_TOOL_SET__ENABLE    0
#define NET_TOOL_SET__DISABLE    1
#define NET_TOOL_SET__MAX_CHK_TIME_SEC 10
mdsReturn_t nettool_set_state(int flag);

#define NET_TOOL_SET__RF_ENABLE    0
#define NET_TOOL_SET__RF_DISABLE    1
mdsReturn_t nettool_set_rf_pwr(int flag);


#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

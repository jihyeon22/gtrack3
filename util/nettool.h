#ifndef __UTIL_NETTOOL_H__
#define __UTIL_NETTOOL_H__

#include <netinet/in.h>
#include <sys/types.h>

#include <include/defines.h>

#define NETTOOL_NETIF "rmnet_data0"

mdsReturn_t nettool_get_state(void);
mdsReturn_t nettool_get_ip(const char* netif_buff, const int netif_buff_len, char *ip_buff, const int ip_buff_len);
mdsReturn_t nettool_get_domain_to_ip(const char *dn_buff, const int dn_buff_len, char *ip_buff, const int ip_buff_len);
int nettool_connect_timeo(int sock, const struct sockaddr *addr, socklen_t addr_len, const unsigned int timeout);
ssize_t nettool_send_timedwait(int sockfd, const void *buf, size_t len, int flags, int sec);
ssize_t nettool_recv_timedwait(int sockfd, void *buf, size_t len, int flags, int sec);
unsigned long int nettool_get_host_name(const char *host_name);
void nettool_init_hostbyname_func(void);

#endif

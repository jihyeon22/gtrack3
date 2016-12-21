#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <include/defines.h>
#include <board/board_system.h>
#include <util/validation.h>
#include <util/tools.h>
#include <base/devel.h>
#include "nettool.h"
#include <logd_rpc.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

#define LOG_DNS 0

int pthread_mutex_timedlock( pthread_mutex_t * mutex, const struct timespec * abs_timeout );
struct hostent *_get_host_by_name(const char *name, struct hostent *hbuf);

/*===========================================================================================*/
mdsReturn_t nettool_get_state(void)
{
	int ret = 0;
	int sock;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0) {
		return DEFINES_MDS_NOK;
	}
	sprintf((char *)&ifr.ifr_name, "%s", NETTOOL_NETIF);
	if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		ret = DEFINES_MDS_NOK;
	}
	else
	{
		ret = DEFINES_MDS_OK;
	}
	close(sock);
	return ret;
}

mdsReturn_t nettool_get_ip(const char* netif_buff, const int netif_buff_len, char *ip_buff, const int ip_buff_len)
{
	int ret = 0;
	int sock;
	struct ifreq ifr;
	struct sockaddr_in * addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		printf("%s:%d> socket open fail\n", __func__, __LINE__);
		return DEFINES_MDS_NOK;
	}
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, netif_buff, IFNAMSIZ - 1);
	if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		printf("%s:%d> socket ioctl fail\n", __func__, __LINE__);
		ret = DEFINES_MDS_NOK;
	}
	else
	{
		addr = (struct sockaddr_in *)&ifr.ifr_addr;
		if(inet_ntop(AF_INET, &addr->sin_addr, ip_buff, ip_buff_len) < 0)
		{
			printf("%s:%d> socket inet_ntop fail\n", __func__, __LINE__);
			ret = DEFINES_MDS_NOK;
		}
		else
		{
			ret = DEFINES_MDS_OK;
		}
	}
	close(sock);
	return ret;
}

mdsReturn_t nettool_get_domain_to_ip(const char *dn_buff, const int dn_buff_len, char *ip_buff, const int ip_buff_len)
{
	struct hostent *host_entry;

	if(validation_check_ip(dn_buff, dn_buff_len) == DEFINES_MDS_OK)
	{
		strncpy(ip_buff, dn_buff, 40);
	}
	else
	{
		host_entry = gethostbyname(dn_buff);
		if(host_entry == NULL)
		{
			return DEFINES_MDS_NOK;
		}
		sprintf(ip_buff, "%s", inet_ntoa(*(struct in_addr*)host_entry->h_addr_list[0]));
	}
	return validation_check_ip(ip_buff, ip_buff_len);
}

int nettool_connect_timeo(int sock, const struct sockaddr *saddr, socklen_t addrsize, const unsigned int timeout)
{
	int newSockStat;
	int orgSockStat;
	int res, n;
	fd_set  rset, wset;
	struct timeval tval;

	int error = 0;
	int esize;

	if((newSockStat = fcntl(sock, F_GETFL, NULL)) < 0)
	{
		perror("F_GETFL error");
		return -1;
	}

	orgSockStat = newSockStat;
	newSockStat |= O_NONBLOCK;

	// Non blocking 상태로 만든다.
	if(fcntl(sock, F_SETFL, newSockStat) < 0)
	{
		perror("F_SETLF error");
		return -1;
	}

	// 연결을 기다린다.
	// Non blocking 상태이므로 바로 리턴한다.
	if((res = connect(sock, saddr, addrsize)) < 0)
	{
		if(errno != EINPROGRESS) {
			fcntl(sock, F_SETFL, orgSockStat);
			return -1;
		}
	}

	//printf("RES : %d\n", res);
	// 즉시 연결이 성공했을 경우 소켓을 원래 상태로 되돌리고 리턴한다.
	if(res == 0)
	{
		printf("Connect Success\n");
		fcntl(sock, F_SETFL, orgSockStat);
		return 1;
	}

	FD_ZERO(&rset);
	FD_SET(sock, &rset);
	wset = rset;

	tval.tv_sec     = timeout;
	tval.tv_usec    = 0;

	if((n = select(sock + 1, &rset, &wset, NULL, &tval)) == 0)
	{
		// timeout
		errno = ETIMEDOUT;
		fcntl(sock, F_SETFL, orgSockStat);
		return -1;
	}

	// 읽거나 쓴 데이터가 있는지 검사한다.
	if(FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset))
	{
		//printf("Read data\n");
		esize = sizeof(int);
		if((n = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0) {
			fcntl(sock, F_SETFL, orgSockStat);
			return -1;
		}
	}
	else
	{
		perror("Socket Not Set");
		fcntl(sock, F_SETFL, orgSockStat);
		return -1;
	}

	fcntl(sock, F_SETFL, orgSockStat);
	if(error)
	{
		errno = error;
		perror("Socket");
		return -1;
	}

	return 1;
}

ssize_t nettool_send_timedwait(int sockfd, const void *buf, size_t len, int flags, int sec)
{
	int result;
	struct timeval tval;
	tval.tv_sec = sec;
	tval.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tval, sizeof(struct timeval)) < 0) {
		perror("SO_SNDTIMEO err");
		return -1;
	}

	result = send(sockfd, buf, len, flags);

	return result;
}

ssize_t nettool_recv_timedwait(int sockfd, void *buf, size_t len, int flags, int sec)
{
	int result;
	struct timeval tval;
	tval.tv_sec = sec;
	tval.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval)) < 0) {
		perror("SO_SNDTIMEO err");
		return -1;
	}

	result = recv(sockfd, buf, len, flags);

	return result;
}

typedef struct {
	char host_name[256];
	unsigned long int saddr;
}host_by_name_t;

int g_hbn_count = 0;
host_by_name_t g_hbn[10];
pthread_mutex_t mutex_hostbyname = PTHREAD_MUTEX_INITIALIZER;
struct timespec mutex_wait;

unsigned long int gethostbyname_local(const char *host_name)
{
	int i;
	unsigned long int result = 0;
	int mutex_res = 0;

	//pthread_mutex_lock(&mutex_hostbyname);
	clock_gettime(CLOCK_REALTIME , &mutex_wait);
	mutex_wait.tv_sec += 2;
	
	mutex_res = pthread_mutex_timedlock(&mutex_hostbyname, &mutex_wait);
	if(mutex_res != 0)
	{
		LOGE(LOG_TARGET, "Mutex timeout![err:%d]\n", mutex_res);
		devel_webdm_send_log("dns mutex error!");
	}
	
	for(i = 0; i < g_hbn_count; i++) 
	{
		if(!strcmp(g_hbn[i].host_name, host_name)) {
			LOGT(LOG_TARGET, "%s:%d> %s local buffer host name %lu\n", __func__, __LINE__, host_name, g_hbn[i].saddr);
			result =  g_hbn[i].saddr;
		}
	}

	if(mutex_res == 0)
	{
		pthread_mutex_unlock(&mutex_hostbyname);
	}

#if LOG_DNS
	{
		char logbuf[255] = {0};
		snprintf(logbuf, sizeof(logbuf)-1, "gethostbyname_local %s %u\n", host_name, result);
		tools_write_data("/var/log/dnstry.log", logbuf, strlen(logbuf), 1);
	}
#endif
	
	return result;
}

void sethostbyname_local(const char *host_name, unsigned long int saddr)
{
	int mutex_res = 0;

	if(g_hbn_count > 9)
	{
		printf("hostbyname_local max count error\n");
		return;
	}
	
	LOGT(LOG_TARGET, "%s:%d> %s local buffer host name %lu\n", __func__, __LINE__, host_name, saddr);

	if(gethostbyname_local(host_name) == 0) {
	
		//pthread_mutex_lock(&mutex_hostbyname);
		clock_gettime(CLOCK_REALTIME , &mutex_wait);
		mutex_wait.tv_sec += 2;
		
		mutex_res = pthread_mutex_timedlock(&mutex_hostbyname, &mutex_wait);
		if(mutex_res != 0)
		{
			LOGE(LOG_TARGET, "Mutex timeout![err:%d]\n", mutex_res);
			devel_webdm_send_log("dns mutex error!");
		}
		
		strcpy(g_hbn[g_hbn_count].host_name, host_name);
		g_hbn[g_hbn_count].saddr = saddr;
		g_hbn_count += 1;

		if(mutex_res == 0)
		{
			pthread_mutex_unlock(&mutex_hostbyname);
		}
		
#if LOG_DNS
		{
			char logbuf[255] = {0};
			snprintf(logbuf, sizeof(logbuf)-1, "sethostbyname_local %s %u\n", host_name, saddr);
			tools_write_data("/var/log/dnstry.log", logbuf, strlen(logbuf), 1);
		}	
#endif

	}
	return;
}

void nettool_init_hostbyname_func(void)
{
	int mutex_res = 0;

	//pthread_mutex_lock(&mutex_hostbyname);
	clock_gettime(CLOCK_REALTIME , &mutex_wait);
	mutex_wait.tv_sec += 2;	
	
	mutex_res = pthread_mutex_timedlock(&mutex_hostbyname, &mutex_wait);
	if(mutex_res != 0)
	{
		LOGE(LOG_TARGET, "Mutex timeout![err:%d]\n", mutex_res);
		devel_webdm_send_log("dns mutex error!");
	}

	memset(&g_hbn, 0, sizeof(g_hbn));
	g_hbn_count = 0;

	if(mutex_res == 0)
	{
		pthread_mutex_unlock(&mutex_hostbyname);
	}

	return;
}

unsigned long int nettool_get_host_name(const char *host_name)
{
	unsigned long int s_addr;

	long int *add;
	struct hostent host_temp;
    struct hostent *host_entry;
	int n_try = 3;

	if(host_name == NULL) {
		printf("!!!!!Wrong format of host name : NULL Error!!!!!");
		return 0;
	}

	if( validation_check_ip(host_name, 255) == DEFINES_MDS_OK)
	{
		return inet_addr(host_name);
	}
	
	if(g_hbn_count > 0) {
		s_addr = gethostbyname_local(host_name);
		if(s_addr > 0) {
			return s_addr;
		}
	}
	
	while(n_try-- > 0)
	{
		host_entry = _get_host_by_name(host_name, &host_temp);
		if (!host_entry) {
			LOGE(LOG_TARGET, "!!!!!Wrong format of host name<%s>. #1!!!!! err[%d]\n", host_name, h_errno);
			LOGE(LOG_TARGET, "-->%s\n", hstrerror( h_errno));
			if(h_errno == 2) {
				//if gethostbyname was called when ppp device don't load completely
				//socket communication operate wrong unlimitly. untill ppp device re-load
			}
			return 0;
		} else {
			if (*host_entry->h_addr_list != NULL) {
				add = (long int *)*host_entry->h_addr_list;
				s_addr = *add;
#if LOG_DNS
				{
					char logbuf[255] = {0};
					snprintf(logbuf, sizeof(logbuf)-1, "gethostbyname %s %u\n", host_name, s_addr);
					tools_write_data("/var/log/dnstry.log", logbuf, strlen(logbuf), 1);
				}
#endif
				
				if(s_addr == 16777343)
				{
#if LOG_DNS
					tools_write_data("/var/log/dnstry.log", "gethostbyname retry\n", strlen("gethostbyname retry\n"), 1);
#endif
					continue;
				}
				break;
			}
			else {
				LOGE(LOG_TARGET, "!!!!!Wrong format of host name<%s>. #2!!!!![%d]\n", host_name, errno);
				LOGE(LOG_TARGET, "-->%s\n", strerror( errno));
				return 0;
			}
		}
	}
	if(n_try < 0)
	{
		return 0;
	}

	sethostbyname_local(host_name, s_addr);
	
	return s_addr;
}

#define SIZE_BUF_HOSTBYNAME 2048
char buf_gethostbyname[2048];

struct hostent *_get_host_by_name(const char *name, struct hostent *hbuf)
{
	int rc, err;
	struct hostent *result = NULL;
	
	if(name == NULL)
	{
		LOGE(LOG_TARGET, "ERROR : Address is NULL.\n");
		return NULL;
	}
	
	if((rc = gethostbyname_r(name, hbuf, buf_gethostbyname, sizeof(buf_gethostbyname), &result, &err)) == ERANGE)
	{
		LOGE(LOG_TARGET, "ERROR : buf_gethostbyname is not enough\n");
		return NULL;
	}

	if (0 != rc || NULL == result) {
		return NULL;
	}
	
	return result;
}

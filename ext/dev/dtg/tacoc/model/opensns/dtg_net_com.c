#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <time.h>

#include <util/nettool.h>

#include <util/crc16.h>
#include <board/board_system.h>

#include <wrapper/dtg_log.h>

#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"

int connect_to_server(char *host_name, int host_port)
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

	int sock_fd = -1;
	struct sockaddr_in my_addr;

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
	{
		goto ERR_FINISH;
	}

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1)
	{
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);

	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto ERR_FINISH;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	return sock_fd; //success
ERR_FINISH:
	if(sock_fd > 0)
		close(sock_fd);

	return -1;
	
}

int send_to_dtg_server(int sock_fd, unsigned char *buffer_in, int buffer_len, char *func, int line, char *msg)
{
	int bytecount;
	int err = 0;

	int sent_len = 0;
	int sending_len = 0;
	int retry_cnt = 0;

	if(sock_fd < 0) {
		DTG_LOGE("%s:%d> %s ----> socket error", func, line, msg);
		return -1;
	}

	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	DTG_LOGI("%s:%d> %s", func, line, msg);

	//printf("%s> ==============================> start\n", msg);
	//dump_packet(buffer_in, buffer_len, msg);
	//printf("%s> ==============================> end\n", msg);

	while(1) {
		bytecount = nettool_send_timedwait(sock_fd, (const char *)buffer_in, buffer_len, 0, 20);
			if(bytecount <= 0) {
				DTG_LOGE("nettool_send_timedwait err: %s", strerror(errno));
				retry_cnt += 1;
			}
			else {
				sent_len += bytecount;
				if(buffer_len <= sent_len) {
					break;
				}
			}
		if(retry_cnt > 10) {
			DTG_LOGE("nettool_send_timedwait retry_cnt[%d] ,err: %s", retry_cnt, strerror(errno));
			return -2;
		}
	}

	
	DTG_LOGI("OpenSNS Server send success!! : sent length[%d], sending length[%d]", sent_len, buffer_len);

	return err;
}

int receive_response(int sock_fd, unsigned char *recv_buf, int max_recv_buf_len)
{
	int ret;
	ret = nettool_recv_timedwait(sock_fd, recv_buf, max_recv_buf_len, 0, 30);
	if(ret <= 0) {
		DTG_LOGE("nettool_recv_timedwait err: %s", strerror(errno));
		return -1;
	}

	return ret; //received length
}

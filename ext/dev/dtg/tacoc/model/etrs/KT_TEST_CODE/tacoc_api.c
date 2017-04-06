/*
 * tacoc_api.c
 *
 *  Created on: 2013. 8. 28.
 *      Author: gbuddha
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>

#include <util/nettool.h>

#include <board/board_system.h>
#include <standard_protocol.h>
#include <base/dmmgr.h>

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_send.h>
#endif


int send_record_data_msg(unsigned char* stream, int stream_size, int line, char *msg)
{
	int i;
	int sock_fd;
	int ret;
	char config_code;
	unsigned char recv_buf[512];

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
	{
		return -1;
	}
	
	sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
	if (sock_fd < 0) {
		DTG_LOGE("server connection fail %s/%d", get_server_ip_addr(), get_server_port());
		return -1;
	}
	DTG_LOGI("server connection success %s/%d", get_server_ip_addr(), get_server_port());
	ret = send_to_dtg_server(sock_fd, stream, stream_size, (char *)__func__, line, msg);
	if(ret < 0)
	{
		DTG_LOGE("++++++Network Sending Error");
		close(sock_fd);
		return -2;
	}

	DTG_LOGI("waiting server response....");
	ret = receive_response(sock_fd, recv_buf, sizeof(recv_buf));
	if(ret <= 0) {
		DTG_LOGE("++++++Network Receive Error");
		close(sock_fd);
		return -3;
	}
	else {
		DTG_LOGI("Network received length = [%d]\n", ret);
		DTG_LOGI("respnse code = [%c]", recv_buf[0]);
		if(recv_buf[0] == 'E')
		{
			DTG_LOGI("server error = [%c]", recv_buf[0]);
			close(sock_fd);
			return -4;
		}
	}

	close(sock_fd);
	return 0;	
}


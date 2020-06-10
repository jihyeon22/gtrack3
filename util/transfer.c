#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <netdb.h>

#include <util/tools.h>
#include <util/nettool.h>
#include <util/debug.h>
#include "transfer.h"

#include <logd_rpc.h>

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

int transfer_packet(const transferSetting_t *setting , const unsigned char *buff, const int buff_len)
{
	int bytecount = 0;
	int retry_cnt = 0;
	int err = 0;

	int hsock;
	struct sockaddr_in c_addr;

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1) {
		err = -1;
		printf("Error initializing socket %d : %s\n", errno, strerror(errno));
		goto FINISH;
	}
	printf("#1 hsock init value = %d\n", hsock);

	memset(&c_addr, 0, sizeof(c_addr));
	LOGI(LOG_TARGET, "Server Connect : %s %d\n", setting->ip, setting->port);
	c_addr.sin_addr.s_addr = nettool_get_host_name(setting->ip);
	if(c_addr.sin_addr.s_addr == 0)
	{
		err = -3;
		goto FINISH;
	}
	
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(setting->port);

#ifdef NONBLOCK_NETWORK
	sflag = fcntl(hsock, F_GETFL, 0);
	fcntl(hsock, F_SETFL, sflag | O_NONBLOCK);
#endif

	while(1) {
		if(nettool_connect_timeo(hsock, (struct sockaddr*)&c_addr, sizeof(c_addr), setting->timeout_secs) < 0) {
			if(retry_cnt++ > setting->retry_count_connect) {
				printf("Error connecting socket val:%d errno:%d %s\n", hsock, errno, strerror(errno));
				err = -11;
				goto FINISH;
			}
		}
		else {
			LOGI(LOG_TARGET, "server connect success\n");
			break;
		}
		sleep(1);
	}

SEND_AGAIN_DATA:
	LOGT(LOG_TARGET, "%s %d\n", __FUNCTION__, buff_len);
	if((bytecount = nettool_send_timedwait(hsock, buff, buff_len, MSG_NOSIGNAL, setting->timeout_secs)) <= 0) {
		retry_cnt++;
		if(retry_cnt >= setting->retry_count_send) {
			printf("Error sending data %d : %s\n", errno, strerror(errno));
			err = -2;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != buff_len) {
		retry_cnt ++;
		if(retry_cnt >= setting->retry_count_send) {
			printf("Error send bytes is not same sent bytes : [%d] %s\n", errno, strerror(errno));
			err = -3;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	retry_cnt = 0;
	bytecount = 0;

FINISH:
	if(hsock != 0 && errno != 88) {
		close(hsock);
	}
	return err;
}

int transfer_packet_recv(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len)
{
	int bytecount = 0;
	int retry_cnt = 0;
	int recv_cnt = 0;
	int total_read_byte=0;
	int err = 0;
	char recv_buf[128] = {0,};

	int hsock;
	struct sockaddr_in c_addr;

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1) {
		err = -1;
		printf("Error initializing socket %d : %s\n", errno, strerror(errno));
		goto FINISH;
	}
	printf("#2 hsock init value = %d\n", hsock);

	memset(&c_addr, 0, sizeof(c_addr));
	LOGI(LOG_TARGET, "Server Connect : %s %d\n", setting->ip, setting->port);
	c_addr.sin_addr.s_addr = nettool_get_host_name(setting->ip);
	if(c_addr.sin_addr.s_addr == 0)
	{
		err = -3;
		goto FINISH;
	}
	
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(setting->port);

#ifdef NONBLOCK_NETWORK
	sflag = fcntl(hsock, F_GETFL, 0);
	fcntl(hsock, F_SETFL, sflag | O_NONBLOCK);
#endif

	LOGI(LOG_TARGET, "network connecting....\n");
	while(1) {
		if(nettool_connect_timeo(hsock, (struct sockaddr*)&c_addr, sizeof(c_addr), setting->timeout_secs) < 0) 
		{
			LOGE(LOG_TARGET, "nettool_connect_timeo timeout!! retry cnt [%d/%d]\n",retry_cnt, setting->retry_count_connect);
			if(retry_cnt++ > setting->retry_count_connect) 
			{
				LOGE(LOG_TARGET,"Error connecting socket val:%d errno:%d, %s\n", hsock, errno, strerror(errno));
				err = -11;
				goto FINISH;
			}
		}
		else {
			LOGI(LOG_TARGET, "server connect success\n");
			break;
		}
		sleep(1);
	}


SEND_AGAIN_DATA:
	LOGT(LOG_TARGET, "%s send tbuff_len %d\n", __FUNCTION__, tbuff_len);
	if((bytecount = nettool_send_timedwait(hsock, tbuff, tbuff_len, MSG_NOSIGNAL, setting->timeout_secs)) <= 0) 
	{
		LOGE(LOG_TARGET, "nettool_send_timedwait timeout!!! retry_cnt [%d/%d]\n", retry_cnt, setting->retry_count_send);
		retry_cnt++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error sending data %d : %s\n", errno, strerror(errno));
			err = -2;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != tbuff_len) {
		LOGE(LOG_TARGET, " -- send error!!!? [%d] , [%d]\n", bytecount, tbuff_len);
		retry_cnt ++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error send bytes is not same sent bytes : [%d] %s\n", errno, strerror(errno));
			err = -3;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	retry_cnt = 0;
	bytecount = 0;
	recv_cnt = 0;
	total_read_byte = 0;

	//usleep(250000); //if recv packet straight,can't recv packet. so need some sleep time.
	while(1) {
		memset(recv_buf,0x00, sizeof(recv_buf));
		if( (bytecount = nettool_recv_timedwait(hsock, recv_buf, sizeof(recv_buf), 0, setting->timeout_secs))  > 0 ) 
		{
			if ( (total_read_byte + bytecount) > rbuff_len )
			{
				bytecount = ( rbuff_len - total_read_byte );
			}

			memcpy((rbuff + total_read_byte), recv_buf, bytecount);

			total_read_byte += bytecount;
			if (total_read_byte == rbuff_len)
			{
				// success;
				break;
			}
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);
			if(retry_cnt++ > setting->retry_count_receive)
			{
				LOGE(LOG_TARGET,"packet_transfer> Error receiving data %d : %s\n", errno, strerror(errno));
				err = -4;
				goto FINISH;
			}
			sleep(1);
		}
	}
	printf("%s recv rbuflen %d\n", __FUNCTION__, bytecount);
FINISH:
	if(hsock != 0 && errno != ENOTSOCK) {
		/*ENOTSOCK : Socket operation on non-socket */
		close(hsock);
	}
	return err;
}

int transfer_packet_recv_etx(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx)
{
	int bytecount = 0;
	int retry_cnt = 0;
	int recv_cnt = 0;
	int total_read_byte=0;
	int err = 0;
	char recv_buf[128] = {0,};

	int hsock;
	struct sockaddr_in c_addr;

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1) {
		err = -1;
		printf("Error initializing socket %d : %s\n", errno, strerror(errno));
		goto FINISH;
	}
	printf("#3 hsock init value = %d\n", hsock);

	memset(&c_addr, 0, sizeof(c_addr));
	LOGI(LOG_TARGET, "Server Connect : %s %d\n", setting->ip, setting->port);
	c_addr.sin_addr.s_addr = nettool_get_host_name(setting->ip);
	if(c_addr.sin_addr.s_addr == 0)
	{
		err = -3;
		goto FINISH;
	}
	
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(setting->port);

#ifdef NONBLOCK_NETWORK
	sflag = fcntl(hsock, F_GETFL, 0);
	fcntl(hsock, F_SETFL, sflag | O_NONBLOCK);
#endif

	while(1) {
		if(nettool_connect_timeo(hsock, (struct sockaddr*)&c_addr, sizeof(c_addr), setting->timeout_secs) < 0) 
		{
			LOGE(LOG_TARGET, "nettool_connect_timeo timeout!! retry cnt [%d/%d]\n",retry_cnt, setting->retry_count_connect);
			if(retry_cnt++ > setting->retry_count_connect) 
			{
				LOGE(LOG_TARGET,"Error connecting socket val:%d errno:%d %s\n", hsock, errno, strerror(errno));
				err = -11;
				goto FINISH;
			}
		}
		else {
			LOGI(LOG_TARGET, "server connect success\n");
			break;
		}
		sleep(1);
	}


SEND_AGAIN_DATA:
	LOGT(LOG_TARGET, "%s send tbuff_len %d\n", __FUNCTION__, tbuff_len);
	if((bytecount = nettool_send_timedwait(hsock, tbuff, tbuff_len, MSG_NOSIGNAL, setting->timeout_secs)) <= 0) 
	{
		LOGE(LOG_TARGET, "nettool_send_timedwait timeout!!! retry_cnt [%d/%d]\n", retry_cnt, setting->retry_count_send);
		retry_cnt++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error sending data %d %s\n", errno, strerror(errno));
			err = -2;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != tbuff_len) {
		LOGE(LOG_TARGET, " -- send error!!!? [%d] , [%d]\n", bytecount, tbuff_len);
		retry_cnt ++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error send bytes is not same sent bytes : [%d] %s\n", errno, strerror(errno));
			err = -3;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	retry_cnt = 0;
	bytecount = 0;
	recv_cnt = 0;
	total_read_byte = 0;

	usleep(250000); //if recv packet straight,can't recv packet. so need some sleep time.
	while(1) {
		memset(recv_buf,0x00, sizeof(recv_buf));
		if( (bytecount = nettool_recv_timedwait(hsock, recv_buf, sizeof(recv_buf), 0, setting->timeout_secs))  > 0 ) 
		{
			if ( (total_read_byte + bytecount) > rbuff_len )
			{
				bytecount = ( rbuff_len - total_read_byte );
			}

			memcpy((rbuff + total_read_byte), recv_buf, bytecount);

			total_read_byte += bytecount;
			
			if (strchr(recv_buf, etx) != NULL)
			{
				// success;
				break;
			}
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);
			if(retry_cnt++ > setting->retry_count_receive)
			{
				LOGE(LOG_TARGET,"packet_transfer> Error receiving data %d %s\n", errno, strerror(errno));
				err = -4;
				goto FINISH;
			}
			sleep(1);
		}
	}
	printf("%s recv rbuflen %d\n", __FUNCTION__, total_read_byte);
FINISH:
	if(hsock != 0 && errno != ENOTSOCK) {
		/*ENOTSOCK : Socket operation on non-socket */
		close(hsock);
	}
	return err;
}


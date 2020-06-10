<<<<<<< HEAD
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
#include <util/nettool.h>
#include <logd_rpc.h>
#include <mdsapi/mds_api.h>
#include <util/transfer.h>

#include "color_printf.h"
#define LOG_TARGET eSVC_NETWORK


int transfer_packet_recv_nisso(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len)
{
	int bytecount = 0;
	int retry_cnt = 0;
	int recv_cnt = 0;
	int total_read_byte=0;
	int err = 0;
	char recv_buf[128] = {0,};

	int hsock;
	struct sockaddr_in c_addr;

    int recv_wait_time = 10;
    int recv_success_cnt = 0;

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
        
        int chk_char_1 = 0;
        int chk_char_2 = 0;

		memset(recv_buf,0x00, sizeof(recv_buf));

		if( (bytecount = nettool_recv_timedwait(hsock, recv_buf, sizeof(recv_buf), 0, recv_wait_time))  > 0 ) 
		{

            LOGT(LOG_TARGET, " nettool_recv_timedwait !! [%d] [%s]\n", bytecount, recv_buf);

			if ( (total_read_byte + bytecount) > rbuff_len )
			{
				bytecount = ( rbuff_len - total_read_byte );
			}

			memcpy((rbuff + total_read_byte), recv_buf, bytecount);

			total_read_byte += bytecount;
            
            // data set 이 맞아야 정상리턴한다.
            chk_char_1 = mds_api_count_char(rbuff, total_read_byte, '[');
            chk_char_2 = mds_api_count_char(rbuff, total_read_byte, ']');

            // 혹시나 한번에 여러개의 데이터가 올수도있을것 같다.
            // 그래서 루틴을 강제로 한번더 태운다.
            if ((chk_char_1 == chk_char_2) && ( chk_char_1 > 0 ) && ( chk_char_2 > 0 ) && (recv_success_cnt > 0) )
            {
                // success;
                break;
            }
            
            recv_success_cnt ++;
            recv_wait_time = 5;
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, 10);
            
            // data set 이 맞아야 정상리턴한다.
            chk_char_1 = mds_api_count_char(rbuff, total_read_byte, '[');
            chk_char_2 = mds_api_count_char(rbuff, total_read_byte, ']');

            // 혹시나 한번에 여러개의 데이터가 올수도있을것 같다.
            // 그래서 루틴을 강제로 한번더 태운다.
            if ((chk_char_1 == chk_char_2) && ( chk_char_1 > 0 ) && ( chk_char_2 > 0 ) && (recv_success_cnt > 0) )
            {
                // success;
                break;
            }

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
int transfer_packet_recv_etxflood(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx)
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

	int i = 0;

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


			if(recv_buf[bytecount-1] == 0x03)
			{
				//print_yellow("recv_buf[%d] : 0x%02x\n", i, recv_buf[i]);
				break;
			}
			
			// if (strchr(recv_buf, etx) != NULL)
			// {
			// 	// success;
			// 	break;
			// }
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);

			print_yellow(" nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);
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
=======
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
#include <util/nettool.h>
#include <logd_rpc.h>
#include <mdsapi/mds_api.h>
#include <util/transfer.h>

#include "color_printf.h"
#define LOG_TARGET eSVC_NETWORK


int transfer_packet_recv_nisso(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len)
{
	int bytecount = 0;
	int retry_cnt = 0;
	int recv_cnt = 0;
	int total_read_byte=0;
	int err = 0;
	char recv_buf[128] = {0,};

	int hsock;
	struct sockaddr_in c_addr;

    int recv_wait_time = 10;
    int recv_success_cnt = 0;

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
        
        int chk_char_1 = 0;
        int chk_char_2 = 0;

		memset(recv_buf,0x00, sizeof(recv_buf));

		if( (bytecount = nettool_recv_timedwait(hsock, recv_buf, sizeof(recv_buf), 0, recv_wait_time))  > 0 ) 
		{

            LOGT(LOG_TARGET, " nettool_recv_timedwait !! [%d] [%s]\n", bytecount, recv_buf);

			if ( (total_read_byte + bytecount) > rbuff_len )
			{
				bytecount = ( rbuff_len - total_read_byte );
			}

			memcpy((rbuff + total_read_byte), recv_buf, bytecount);

			total_read_byte += bytecount;
            
            // data set 이 맞아야 정상리턴한다.
            chk_char_1 = mds_api_count_char(rbuff, total_read_byte, '[');
            chk_char_2 = mds_api_count_char(rbuff, total_read_byte, ']');

            // 혹시나 한번에 여러개의 데이터가 올수도있을것 같다.
            // 그래서 루틴을 강제로 한번더 태운다.
            if ((chk_char_1 == chk_char_2) && ( chk_char_1 > 0 ) && ( chk_char_2 > 0 ) && (recv_success_cnt > 0) )
            {
                // success;
                break;
            }
            
            recv_success_cnt ++;
            recv_wait_time = 5;
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, 10);
            
            // data set 이 맞아야 정상리턴한다.
            chk_char_1 = mds_api_count_char(rbuff, total_read_byte, '[');
            chk_char_2 = mds_api_count_char(rbuff, total_read_byte, ']');

            // 혹시나 한번에 여러개의 데이터가 올수도있을것 같다.
            // 그래서 루틴을 강제로 한번더 태운다.
            if ((chk_char_1 == chk_char_2) && ( chk_char_1 > 0 ) && ( chk_char_2 > 0 ) && (recv_success_cnt > 0) )
            {
                // success;
                break;
            }

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
int transfer_packet_recv_etxflood(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx)
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

	int i = 0;

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


			if(recv_buf[bytecount-1] == 0x03)
			{
				//print_yellow("recv_buf[%d] : 0x%02x\n", i, recv_buf[i]);
				break;
			}
			
			// if (strchr(recv_buf, etx) != NULL)
			// {
			// 	// success;
			// 	break;
			// }
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);

			print_yellow(" nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);
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
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}
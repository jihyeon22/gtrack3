
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <base/devel.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <util/poweroff.h>

#include "logd/logd_rpc.h"

#include "netcom.h"
#include "alloc_packet.h"
#include "alloc_packet_tool.h"

#include "color_printf.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

#define _RCV_DATA_CONTINUE			0
#define _RCV_DATA_DONE				1
#define _RCV_DATA_ACK_AND_DONE		2
#define _RCV_DATA_NOT_AGREEMENT		3

#define _MAX_ACK_BUFF				64

#define _SEND_ACK_TIMEOUT_SEC		10
#define _SEND_ACK_RETRY_CNT			10
#define _SEND_ACK_TIMEOUT_SEC		10

void _deinit_essential_functions(void);

int _send_ack_pkt(int hsock, const unsigned char *tbuff, const int tbuff_len)
{
	int retry_cnt = 0;
	int bytecount = 0;
	int err = 0;
SEND_AGAIN_DATA:

	printf("%s send tbuff_len %d\n", __FUNCTION__, tbuff_len);
	
	if ( tbuff_len <= 0 )
		return -1;
	
	if((bytecount = nettool_send_timedwait(hsock, tbuff, tbuff_len, MSG_NOSIGNAL, _SEND_ACK_TIMEOUT_SEC)) <= 0) 
	{
		LOGE(LOG_TARGET, "_send_ack_pkt timeout!!! retry_cnt [%d/%d]\n", retry_cnt, _SEND_ACK_RETRY_CNT);
		retry_cnt++;
		if(retry_cnt >= _SEND_ACK_RETRY_CNT) {
			LOGE(LOG_TARGET, "_send_ack_pkt : Error sending data %d\n", errno);
			err = -2;
			//goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	if(bytecount != tbuff_len) {
		LOGE(LOG_TARGET, " -- _send_ack_pkt : send error!!!? [%d] , [%d]\n", bytecount, tbuff_len);
		retry_cnt ++;
		if(retry_cnt >= _SEND_ACK_RETRY_CNT) {
			LOGE(LOG_TARGET, "_send_ack_pkt : Error send bytes is not same sent bytes : [%d]\n", errno);
			err = -3;
			//goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	return err;
}

int _recv_data(int sock, int op, char* rcv_buff, int rcv_len, unsigned char* ack_buff, int* ack_len)
{
	packet_frame_t result = {{0},0};
	
	char pkt_id = 0;
	int  pkt_ret = 0;
	
	
	printf("-------------------------------------------\r\n");
	printf("rcv_data op[%d]\r\n",op);
	printf("rcv_data len[%d]\r\n",rcv_len);
	printf("rcv_data data[%s]\r\n",rcv_buff);
	printf("-------------------------------------------\r\n");
	
	if ( _alloc_get_packet_frame(rcv_buff, rcv_len, &result) != 0 )
		return _RCV_DATA_CONTINUE;
	
	if (result.size <= 0 )
		return _RCV_DATA_CONTINUE;
	
	// -----------------------------------------
	// binary pkt receive
	// -----------------------------------------
	pkt_id = _alloc_get_binary_packet_id(result);
	switch ( pkt_id )
	{
		case eRCV_PKT_ID_PASSENGER_INFO:
		{
			LOGT(LOG_TARGET, "BIN RECV : RFID \r\n");
			pkt_ret = save_passenger_info(result);
			if ( get_passenger_info_stat() == eGET_PASSENGER_STAT_COMPLETE)
			{
				char ack_cmd_id[10] = {0,};
				get_passenger_cmd_id(ack_cmd_id);
				printf ("recv passenger ret [%d]\r\n",pkt_ret);
				
				LOGI(LOG_TARGET, "BIN RECV SUCCESS : RFID id [%s] \r\n", ack_cmd_id);
				
				_send_ack_pkt(sock, ack_cmd_id, strlen(ack_cmd_id));
				return _RCV_DATA_DONE;
			}
			break;
		}
		case eRCV_PKT_ID_STOP_INFO:
			LOGT(LOG_TARGET, "BIN RECV : GEOFENCE \r\n");
			print_yellow("eRCV_PKT_ID_STOP_INFO \r\n");
		
			pkt_ret = save_geofence_info(result);
			if ( get_geo_fence_info_stat() == eGET_GEOFENCE_STAT_COMPLETE)
			{
				char ack_cmd_id[10] = {0,};
				get_geo_fence_cmd_id(ack_cmd_id);
				
				LOGI(LOG_TARGET, "BIN RECV SUCCESS : GEOFENCE id [%s] \r\n", ack_cmd_id);
				
				print_geo_fence_info();
				_send_ack_pkt(sock, ack_cmd_id, strlen(ack_cmd_id));
				return _RCV_DATA_DONE;
			}
			
		break;
		case eRCV_PKT_ID_FTP_INFO:
			LOGT(LOG_TARGET, "BIN RECV : FTP \r\n");
			print_yellow("eRCV_PKT_ID_FTP_INFO \r\n");
		
			pkt_ret = save_ftpserver_info(result);
			if ( pkt_ret > 1)
			{
				char ack_cmd_id[10] = {0,};
				//print_ftpserver_info(ack_cmd_id);
				
				LOGI(LOG_TARGET, "BIN RECV SUCCESS : FTP Sever id [%s] \r\n", ack_cmd_id);
				
				print_ftpserver_info();
				_send_ack_pkt(sock, ack_cmd_id, strlen(ack_cmd_id));
				return _RCV_DATA_DONE;
			}
			
		break;
		case eRCV_PKT_ID_SEND_PERIOD:
			LOGT(LOG_TARGET, "BIN RECV : PERIOD \r\n");
		break;
		default:
		break;
	}
	
	// -----------------------------------------
	// non binary pkt...
	// -----------------------------------------
	switch (op)
	{
		case PACKET_TYPE_GET_RFID:
		{
			if ( strncmp(result.packet_content, "P", 1) == 0 )
			{
				char ack_cmd_id[10] = {0,};
				
				LOGI(LOG_TARGET, "PKT RETRUN: RFID VER IS LASTEST \r\n");
				
				get_passenger_cmd_id(ack_cmd_id);

				_send_ack_pkt(sock, ack_cmd_id, strlen(ack_cmd_id));
				return _RCV_DATA_DONE;
				//return _RCV_DATA_CONTINUE;
			}
			break;
		}
		case PACKET_TYPE_EVENT:
		case PACKET_TYPE_REPORT:
		case PACKET_TYPE_GEO_FENCE_IN:
		case PACKET_TYPE_GEO_FENCE_OUT:
		{
			if ( strncmp(result.packet_content, "M", 1) == 0 )
			{
				LOGI(LOG_TARGET, "PKT RETRUN: PERIOD PKT SUCCESS \r\n");
				
				return _RCV_DATA_DONE;
			}
			break;
		}
		case PACKET_TYPE_TAGGING:
		{
			if ( strncmp(result.packet_content, "T", 1) == 0 )
			{
				LOGI(LOG_TARGET, "PKT RETRUN: TAGGING PKT SUCCESS \r\n");
				
				return _RCV_DATA_DONE;
			}
			else
			{
				return _RCV_DATA_NOT_AGREEMENT;
			}
			break;			
		}
		case PACKET_TYPE_RESP_SMS:
		{
			if ( strncmp(result.packet_content, "B", 1) == 0 )
			{
				LOGI(LOG_TARGET, "PKT RETRUN: SMS PKT SUCCESS \r\n");
				
				return _RCV_DATA_DONE;
			}
			else
			{
				return _RCV_DATA_NOT_AGREEMENT;
			}
			break;
		}
		default:
		break;
	}

	if ( strncmp(result.packet_content, "(RRR)", 5) == 0 )
	{
		_deinit_essential_functions();
		poweroff("Reset CMD\n", sizeof("Reset CMD\n"));
	}
	
	return _RCV_DATA_CONTINUE;
}

int transfer_packet_recv_call(int op, const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len)
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
		printf("Error initializing socket %d\n", errno);
		goto FINISH;
	}
	//printf("hsock init value = %d\n", hsock);

	memset(&c_addr, 0, sizeof(c_addr));
	
	LOGI(LOG_TARGET, "Server Connect : %s %d\n", setting->ip, setting->port);
	
	// printf("Server Connect : %s %d\n", setting->ip, setting->port);
	c_addr.sin_addr.s_addr = nettool_get_host_name(setting->ip);
	if(c_addr.sin_addr.s_addr == 0)
	{
		err = -3;
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
				LOGE(LOG_TARGET,"Error connecting socket val:%d errno:%d\n", hsock, errno);
				err = -11;
				goto FINISH;
			}
		}
		else {
			LOGI(LOG_TARGET,"server connect success\n");
			break;
		}
		sleep(1);
	}


SEND_AGAIN_DATA:
	printf("%s send tbuff_len %d\n", __FUNCTION__, tbuff_len);
	if((bytecount = nettool_send_timedwait(hsock, tbuff, tbuff_len, MSG_NOSIGNAL, setting->timeout_secs)) <= 0) 
	{
		LOGE(LOG_TARGET, "nettool_send_timedwait timeout!!! retry_cnt [%d/%d]\n", retry_cnt, setting->retry_count_send);
		retry_cnt++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error sending data %d\n", errno);
			err = -2;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != tbuff_len) {
		LOGE(LOG_TARGET, " -- send error!!!? [%d] , [%d]\n", bytecount, tbuff_len);
		retry_cnt ++;
		if(retry_cnt >= setting->retry_count_send) {
			LOGE(LOG_TARGET, "Error send bytes is not same sent bytes : [%d]\n", errno);
			err = -3;
			goto FINISH;
		}
		goto SEND_AGAIN_DATA;
	}

	retry_cnt = 0;
	bytecount = 0;
	recv_cnt = 0;
	total_read_byte = 0;

	printf("send pkt success!!!???\r\n");
	
	usleep(2500); //if recv packet straight,can't recv packet. so need some sleep time.
	while(1) 
	{
		memset(recv_buf,0x00, sizeof(recv_buf));
		
		if( (bytecount = nettool_recv_timedwait(hsock, recv_buf, sizeof(recv_buf), 0, setting->timeout_secs))  > 0 ) 
		{
			unsigned char ack_buff[_MAX_ACK_BUFF];
			int ack_len = 0;
			int ret_case = 0;
			//printf("receive pkt success!!!\r\n");
			ret_case = _recv_data(hsock, op, recv_buf, bytecount, ack_buff, &ack_len);
			switch(ret_case)
			{
				case _RCV_DATA_CONTINUE:
				{
					continue;
				}
				case _RCV_DATA_DONE:
				{
					goto FINISH;
				}
				case _RCV_DATA_NOT_AGREEMENT:
				{
					devel_webdm_send_log("Received abnormal response packet. [%d][%.127s]",op,recv_buf);
					goto FINISH;
				}
				default :
					goto FINISH;
			}
			break;
		}
		else
		{
			LOGE(LOG_TARGET, " nettool_recv_timedwait timeout!!! [%d/%d]\n", retry_cnt, setting->retry_count_receive);
			retry_cnt++;
			if(retry_cnt >= setting->retry_count_receive)
			{
				LOGE(LOG_TARGET,"packet_transfer> Error receiving data %d\n", errno);
				err = -4;
				goto FINISH;
			}
			//sleep(1);
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




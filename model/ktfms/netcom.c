#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <callback.h>
#include "netcom.h"


#include "pkt_manage.h"
#include "kt_fms_packet.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int get_network_param(transferSetting_t* network_param);

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	unsigned short packet_size;
	switch(op)
	{
		case MDS_PKT_1_EVENT:
		{
			mds_packet_1_set_send_stat(PACKET_TRANSFER_STAT__SENDING);
			res = mds_packet_1_get_front_pkt(&packet_size, packet_buf);
			if ( res == PACKET_RET_SUCCESS )
			{
				printf("packet_len is [%d]\r\n",packet_size);
				*packet_len = packet_size;
			}
			else
			{
				res = -1;
			}
			break;
		}
		default:
			res = -1;
	}
	
	return 0;

}

//#define SERVER_RET_TEST

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	static int pkt_send_fail_cnt = 0;
	static int pkt_send_retry_cnt = 0;
	static int pkt_send_total_fail_cnt = 0;
	char rcv_packet[1024] = {0,};

	int ret = 0;
	int server_ret_code = 0;
	
	static server_fail_policy_t cur_srv_policy = {0,};
	static int last_ret_code = 0 ;
	
	transferSetting_t network_param = {0,};
	
	printf("SEND body (%d) -------------------- \r\n",packet_len);
	printf("%s", packet_buf);
	printf("---------------------------------- \r\n");
	
	// 만약 fail 정책이면 전송을 하지 않는다.
	if ( get_send_policy() == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP ) 
	{
		LOGE(LOG_TARGET, " -> SEND :: FAIL POLICY\n" );
		// 기존에 쌓여있던 패킷을 모두 삭제한다.
		while (1)
		{
			if (mds_packet_1_clear_rear() != 0)
				break;
		}
		
		// fail 시에는 전송막는다.
		set_server_send_interval(999999999);
		mds_packet_1_set_send_stat(PACKET_TRANSFER_STAT__COMPLETE);
		return 0;
	}
	
#ifndef SERVER_RET_TEST
	get_network_param(&network_param);
	
	LOGI(LOG_TARGET, "SERVER IP [%s] : [%d] \n", network_param.ip, network_param.port);
	
	ret = transfer_packet_recv(&network_param, packet_buf, packet_len, (unsigned char *)&rcv_packet, sizeof(rcv_packet));
	
	printf("recv [%s]\r\n",rcv_packet);
	

	
	// 서버응답값이야 어찌되었건.. cli 커맨드에서 reset 명령을 내렸으면 리셋한다.
	// cli 커맨드를 받고 바로 처리, 리셋 했더니..
	// 서버에서는 제대로 처리를 못한것으로 인식되어 계속 리셋명령어를 내린다.
	// 때문에 cli cmd 처리 명령어를 보낸후 리셋하도록한다.
	// 리셋할것이기 때문에 에러처리나 서버시나리오는 모두 무시하도록한다.
	if ( get_cli_cmd_reset_stat() > 0 )
	{
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");
		LOGI(LOG_TARGET, "CLI CMD : req reset!!!!!\r\n");

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
		send_device_off_packet(30);
#endif
		//poweroff("CLI CMD : REQ REBOOT", strlen("CLI CMD : REQ REBOOT"));
		
		while(1)
		{
			sleep(5);
			system("poweroff");
		}
		return 0;
	}
	
	// 응답값이 있을때..
	if (strlen(rcv_packet) > 0 )
	{
		server_ret_code = set_server_stat_from_return_str(rcv_packet);
	}
	else // 응답값이 없을때... 기존 error 시나리오를 따른다.
	{
		server_ret_code = KT_FMS_SERVER_RET__SERVER_NO_RESP;
	}
	
	g_last_dev_stat.svr_ret_code = server_ret_code;
	
#else
	server_ret_code = KT_FMS_SERVER_RET__SUCCESS;
	//server_ret_code = KT_FMS_SERVER_RET__INVAILD_DATA_1;
#endif

	get_server_fail_policy(server_ret_code, &cur_srv_policy);
	
	if ( last_ret_code != server_ret_code )
	{
		pkt_send_fail_cnt = 0;
		pkt_send_retry_cnt = 0;
	}
	
	last_ret_code = server_ret_code;
	
	LOGI(LOG_TARGET, "SERVER POLICY cur send [%d]-> retry [%d] / rm [%d] / sleep [%d] / stop [%d]\n", 
					get_send_policy(),
					cur_srv_policy.pkt_send_fail_retry_cnt, 
					cur_srv_policy.pkt_send_fail_remove_cnt,
					cur_srv_policy.pkt_send_retry_delay_sec,
					cur_srv_policy.pkt_send_fail_stop_cnt);
					
	//  pwr off 이면 해당 패킷은 무조건 완료시키고..
	// 재전송 정책을 모두 skip 한다.
	if  (( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT ) ||
		  ( get_send_policy() == KT_FMS_SEND_POLICY__NONE ))
	{
		// 무조건 패킷 성공으로 처리
		LOGI(LOG_TARGET, " -> SEND :: POWEROFF POLICY\n" );
		
		mds_packet_1_clear_rear();
		// 나머지 데이터들 모두 전송을 위해 interval 을 1초로 세팅
		set_server_send_interval(5);
		mds_packet_1_set_send_stat(PACKET_TRANSFER_STAT__COMPLETE);
		return 0;
	}
	
	switch (server_ret_code)
	{
		case KT_FMS_SERVER_RET__SUCCESS:
		{
			pkt_send_fail_cnt = 0;
			pkt_send_retry_cnt = 0;
			
			// 기존에 쌓여있던 패킷이 있으면, 다음 전송주기는 바로 하도록한다.
			if ( mds_packet_1_get_use_cnt() > 1 )
			{
				printf(" pkt buffer is not empty [%d] . send imediate\r\n",mds_packet_1_get_use_cnt());
				set_server_send_interval(1);
			}
			else
			{
				set_server_send_interval_default();
			}
			
			mds_packet_1_clear_rear();
			break;
		}
		// 미등록단말일 경우 전송중단
		case KT_FMS_SERVER_RET__NOT_REGI_DEV:	// 403
		{
			// 해당 패킷을 삭제한다.
			set_server_send_interval(999999999);
			set_send_policy(KT_FMS_SEND_POLICY__SERVER_FAIL_STOP);
				
			pkt_send_fail_cnt = 0;
			pkt_send_retry_cnt = 0;
			pkt_send_total_fail_cnt = 0;
			
			// 기존에 쌓여있던 패킷을 모두 삭제한다.
			while (1)
			{
				if (mds_packet_1_clear_rear() != 0)
					break;
			}
			break;
		}
		// need more data 의 경우 아무리 재전송해봐야 계속 에러를 뱉어낸다.
		// 때문에 해당 패킷은 그냥 삭제
		case KT_FMS_SERVER_RET__SERVER_ERR_1:	// 500
		case KT_FMS_SERVER_RET__SERVER_ERR_2:	// 503
		case KT_FMS_SERVER_RET__NEED_MORE_DATA:	// 480
		case KT_FMS_SERVER_RET__INVAILD_DATA_1:	// 400
		case KT_FMS_SERVER_RET__INVAILD_DATA_2:	// 481
		case KT_FMS_SERVER_RET__INVAILD_DATA_3:	// 482
		case KT_FMS_SERVER_RET__SERVER_NO_RESP:
		{
			pkt_send_fail_cnt++;
			
			if ( pkt_send_fail_cnt > cur_srv_policy.pkt_send_fail_retry_cnt )
			{
				LOGE(LOG_TARGET, "SERVER RESP ERR CASE 1 : fail [%d] / retry [%d]\n",  pkt_send_fail_cnt, pkt_send_retry_cnt);
				// pkt_send_fail_retry_cnt 만큼 시도했을때도 전송실패하면..
				// pkt_send_retry_delay_sec 만큼 쉰 후에 재전송
				set_server_send_interval(cur_srv_policy.pkt_send_retry_delay_sec);
				pkt_send_fail_cnt = 0;
				pkt_send_retry_cnt ++;
				pkt_send_total_fail_cnt++;
			}
			else
			{
				LOGE(LOG_TARGET, "SERVER RESP ERR CASE 2 : fail [%d] / retry [%d]\n",  pkt_send_fail_cnt, pkt_send_retry_cnt);
				// 실패시 일단, 60초 만큼 쉰다.
				set_server_send_interval(DEFAILT_FMS_SEND_FAIL_NORMAL_SLEEP_SEC);
			}
			
			// 여러번 시도했는데도 실패라면...
			if ( pkt_send_retry_cnt >= (cur_srv_policy.pkt_send_fail_remove_cnt)) 
			{
				LOGE(LOG_TARGET, "SERVER RESP ERR CASE 3 : fail [%d] / retry [%d]\n",  pkt_send_fail_cnt, pkt_send_retry_cnt);
				// 해당 패킷을 삭제한다.
				mds_packet_1_clear_rear();
				set_server_send_interval(cur_srv_policy.pkt_send_retry_delay_sec);
				pkt_send_retry_cnt = 0;
			}
			
			// 연속으로 최소 여러번 실패면 아예 멈추도록 한다.
			if ( pkt_send_total_fail_cnt >= (cur_srv_policy.pkt_send_fail_stop_cnt)) 
			{
				LOGE(LOG_TARGET, "SERVER RESP ERR CASE 4 : fail [%d] / retry [%d]\n",  pkt_send_fail_cnt, pkt_send_retry_cnt);
				// 해당 패킷을 삭제한다.
				set_server_send_interval(999999999);
				set_send_policy(KT_FMS_SEND_POLICY__SERVER_FAIL_STOP);
				
				pkt_send_fail_cnt = 0;
				pkt_send_retry_cnt = 0;
				pkt_send_total_fail_cnt = 0;
				
				// 기존에 쌓여있던 패킷을 모두 삭제한다.
				while (1)
				{
					if (mds_packet_1_clear_rear() != 0)
						break;
				}
			}
			
			LOGE(LOG_TARGET, "SERVER RESP ERR End : [%d], [%d], [%d]. [%d] \n", 0, pkt_send_fail_cnt, pkt_send_retry_cnt, pkt_send_total_fail_cnt);
			
			break;
		}
		default :
		{
			LOGI(LOG_TARGET,"SERVER RESP : unsupport server ret code ..[%d]\r\n", server_ret_code);
			break;
		}
	}
	
	mds_packet_1_set_send_stat(PACKET_TRANSFER_STAT__COMPLETE);
	
	return 0;
}

int free_packet(void *packet)
{
	if(packet != NULL)
	{
		free(packet);
	}
	
	return 0;
}


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <at/at_util.h>
#include <board/power.h>
#include <base/watchdog.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <callback.h>
#include "netcom.h"

#include "katech-packet.h"

#define LOG_TARGET eSVC_NETWORK

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	
	switch(op)
	{
		case KATECH_PKT_ID_AUTH:
		{
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_AUTH\r\n", __func__);
			res = katech_pkt_auth_make(packet_len, packet_buf);
			break;
		}
		case KATECH_PKT_ID_FW_CHK:
		{
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_FW_CHK\r\n", __func__);
			res = katech_pkt_fw_make(packet_len, packet_buf);
			break;
		}
		case KATECH_PKT_ID_REPORT_1:
		{
            int pkt_att = (int*)param;
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_1\r\n", __func__);
			res = katech_pkt_report_data_1_make(packet_len, packet_buf, pkt_att);
			break;
		}
		case KATECH_PKT_ID_REPORT_2:
		{
            int pkt_att = (int*)param;
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_2\r\n", __func__);
			res = katech_pkt_report_data_2_make(packet_len, packet_buf, pkt_att);
			break;
		}
		default:
		{
			break;
			res = -1;
		}
	}
	return res;
}

static int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int recv_size = 0;
	//unsigned char rcv_packet[1024] = {0,};
	
	int res = 0;
	
	
	transferSetting_t network_param = {0,};

	{
		// echo -e "hello\n\n" | nc api.vias.co.kr 10006
		strncpy(network_param.ip, "api.vias.co.kr", strlen("api.vias.co.kr"));
		network_param.port = 10006;

		// report :: network socket api setting
		network_param.retry_count_connect = DEFAULT_SETTING_SOCK_CONN_RETRY_CNT;
		network_param.retry_count_send = DEFAULT_SETTING_SOCK_SEND_RETRY_CNT;
		network_param.retry_count_receive = DEFAULT_SETTING_SOCK_RCV_RETRY_CNT;
		network_param.timeout_secs = DEFAULT_SETTING_SOCK_TIMEOUT_SEC;		
	}
	
	//printf("%s() start...\r\n");
	switch(op)
	{
		case KATECH_PKT_ID_AUTH:
		{
            KATCH_PKT_AUTH_RESP rcv_packet;
            memset(&rcv_packet, 0x00, sizeof(rcv_packet));
            recv_size = sizeof(KATCH_PKT_AUTH_RESP);
            rcv_packet.resp_code = -1;

			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_AUTH - send and resp. \r\n", __func__);
			
			res = transfer_packet_recv(&network_param, 
										packet_buf, 
										packet_len, 
										(unsigned char *)&rcv_packet, 
										recv_size);
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_AUTH - rcv ret is [%d] size is [%d]\r\n", __func__, res, recv_size);
			res = katech_pkt_auth_parse(res, &rcv_packet);
			break;
		}
		case KATECH_PKT_ID_FW_CHK:
		{
            KATCH_PKT_FW_RESP rcv_packet;
			recv_size = sizeof(KATCH_PKT_FW_RESP);
			memset(&rcv_packet, 0x00, sizeof(rcv_packet));
            rcv_packet.resp_code = -1;

			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_FW_CHK - send and resp. \r\n", __func__);

			res = transfer_packet_recv(&network_param, 
										packet_buf, 
										packet_len, 
										(unsigned char *)&rcv_packet, 
										recv_size);
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_FW_CHK - rcv ret is [%d] size is [%d]\r\n", __func__, res, recv_size);
			res = katech_pkt_fw_parse(res, &rcv_packet);
			break;
		}
		case KATECH_PKT_ID_REPORT_1:
		{
            KATCH_PKT_REPORT_DATA_1_RESP rcv_packet;
			recv_size = sizeof(KATCH_PKT_REPORT_DATA_1_RESP);
			memset(&rcv_packet, 0x00, sizeof(rcv_packet));
			rcv_packet.resp_code = -1;

			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_1 - send and resp. \r\n", __func__);

			res = transfer_packet_recv(&network_param, 
										packet_buf, 
										packet_len, 
										(unsigned char *)&rcv_packet, 
										recv_size);
			
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_1 - rcv ret is [%d] size is [%d]\r\n", __func__, res, recv_size);
			res = katech_pkt_report_data_1_resp(res, &rcv_packet);
			break;
		}
		case KATECH_PKT_ID_REPORT_2:
		{
            KATCH_PKT_REPORT_DATA_2_RESP rcv_packet;
            recv_size = sizeof(KATCH_PKT_REPORT_DATA_2_RESP);
			memset(&rcv_packet, 0x00, sizeof(rcv_packet));
			rcv_packet.resp_code = -1;

			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_2 - send and resp. \r\n", __func__);

			res = transfer_packet_recv(&network_param, 
										packet_buf, 
										packet_len, 
										(unsigned char *)&rcv_packet, 
										recv_size);
			LOGT(LOG_TARGET, "%s() : KATECH_PKT_ID_REPORT_2 - rcv ret is [%d] size is [%d]\r\n", __func__, res, recv_size);
			res = katech_pkt_report_data_2_resp(res, &rcv_packet);
			break;
		}
		default:
		{
			printf("%s() : unknown... \r\n", __func__);
			break;
			res = -1;
		}
	}
	return res;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
    int send_res = -1;
    int max_send_retry = 10;
    
    while(max_send_retry--)
    {
        send_res = __send_packet(op, packet_buf, packet_len);
        if (send_res == 0)
            break;
        watchdog_set_cur_ktime(eWdNet1);
        watchdog_set_cur_ktime(eWdNet2);
        sleep(10);
    }

    return send_res;
}

int free_packet(void *packet)
{
	if(packet != NULL)
	{
		free(packet);
	}
	
	return 0;
}


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <base/devel.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"


#include "at/at_util.h"
#include "at/at_log.h"

#include "netcom.h"
#include "packet.h"
#include "config.h"
#include "mds_ctx.h"
#include "mds.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

static void _process_packet_recv(void* pdata);

transferSetting_t gSetting_report;
transferSetting_t gSetting_request;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	switch(op)
	{
		case REPORT_PERIOD_EVENT:
		{
			unsigned short packet_size;
			
			printf("REPORT_PERIOD_EVENT start\r\n");
			res = get_packet_keyon_first(&packet_size, packet_buf);
			
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
		case REPORT_TURNON_EVENT:
		case REPORT_TURNOFF_EVENT:
		{
			unsigned short packet_size;
			
			printf("REPORT_TURNOFF_EVENT start\r\n");
			res = get_packet_keyoff_first(&packet_size, packet_buf);
			
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
		case REPORT_DEPARTURE_EVENT:
		case REPORT_RETURN_EVENT:
		case REPORT_ARRIVAL_EVENT:
		case REPORT_WAITING_EVENT:
		case REPORT_POWER_EVENT:
			printf("make_event_packet %x\n", op);
//			res = make_event_packet(packet_buf, packet_len, op, *(int *)param);
			break;
		default:
			res = -1;
	}
	return res;
}


int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int timeout = SEND_PACKET_RETRY_CNT;
	int fail_sleep_time = SEND_FAIL_SLEEP_TIME;
	int server_err_retry = FAIL_PACKET_RETY_CNT;
	
	int trynum = 1;
	int res = 0;

	// kksworks hardcording
	
	TRIPHOS_RESP_PACKET__SERVER_RESP_RCV rcv_packet;
	
	transferSetting_t network_param = {0,};
	
	//debug_hexdump_buff(packet_buf, packet_len);

	// debug log...
	/*
	tools_write_data("/data/packet.log", packet_buf, packet_len, true);
	*/
	
	while(1)
	{
		get_report_network_param(&network_param);

		LOGI(LOG_TARGET,"send packet - server is [%s]:[%d] \n",network_param.ip, network_param.port);
		
		res = transfer_packet_recv(&network_param, packet_buf, packet_len, (unsigned char *)&rcv_packet, sizeof(TRIPHOS_RESP_PACKET__SERVER_RESP_RCV));
		
		// test code..
		//rcv_packet.response = MDT_SERVER_STAT__ERR_SERVER_OVER;
		//if (res != 0)
		LOGI(LOG_TARGET, "send packet res is [%d] \n",res);
		
		if (res == 0 && (rcv_packet.protocol_id == SERVER_RESP_RCV))
		{
			if ( (rcv_packet.response != MDT_SERVER_STAT__SUCCESS) && (server_err_retry -- >= 0) )
			{
				LOGE(LOG_TARGET, "server response err code is [%c] \n",rcv_packet.response);
				trynum++;
				continue;
			}
		}

		if ( (res == 0 || (trynum >= timeout && timeout != 0)) )
		{
			break;
		}

		LOGE(LOG_TARGET, "send packet fail retry cnt  [%d] \n",trynum);
		LOGE(LOG_TARGET, "send packet fail retry cnt  [%d] \n",trynum);
		LOGE(LOG_TARGET, "send packet fail retry cnt  [%d] \n",trynum);
		LOGE(LOG_TARGET, "send packet fail retry cnt  [%d] \n",trynum);
		
		if (res != 0)
		{
			while(fail_sleep_time-- > 0)
			{
				LOGE(LOG_TARGET, "send packe err sleep time... [%d] \n",fail_sleep_time);
				sleep(1);
			}
		}

		trynum++;
	}

	// debug log
	/*
	{
		char temp_buff[128] = {0,0};
		sprintf(temp_buff,"%02x%02x", res, rcv_packet.response);
		tools_write_data(LOG_PACKET_PATH, temp_buff, strlen(temp_buff), true);
	}
	*/
	
	if (res == 0) 
		_process_packet_recv(&rcv_packet);
	
			
	return SEND_PACKET_RET;
}


void init_network_param()
{
	// report :: server info setting 
	strncpy(gSetting_report.ip, DEFAULT_SETTING_SERVER_IP, 40);
	gSetting_report.port = DEFAULT_SETTING_SERVER_PORT;
	
	// report :: network socket api setting
	gSetting_report.retry_count_connect = DEFAULT_SETTING_SOCK_CONN_RETRY_CNT;
	gSetting_report.retry_count_send = DEFAULT_SETTING_SOCK_SEND_RETRY_CNT;
	gSetting_report.retry_count_receive = DEFAULT_SETTING_SOCK_RCV_RETRY_CNT;
	gSetting_report.timeout_secs = DEFAULT_SETTING_SOCK_TIMEOUT_SEC;

	// request :: server info setting
	strncpy(gSetting_request.ip, DEFAULT_SETTING_SERVER_IP, 40);
	gSetting_request.port = DEFAULT_SETTING_SERVER_PORT;
	
	// request :: network socket api setting
	gSetting_request.retry_count_connect = DEFAULT_SETTING_SOCK_CONN_RETRY_CNT;
	gSetting_request.retry_count_send = DEFAULT_SETTING_SOCK_SEND_RETRY_CNT;
	gSetting_request.retry_count_receive = DEFAULT_SETTING_SOCK_RCV_RETRY_CNT;
	gSetting_request.timeout_secs = DEFAULT_SETTING_SOCK_TIMEOUT_SEC;
}

int setting_network_param(void)
{
	configurationModel_t *conf = get_config_model();
	
	// report :: server info setting 
	strncpy(gSetting_report.ip, conf->model.report_ip, 40);
	gSetting_report.port = conf->model.report_port;
	
	// report :: network socket api setting --> for hard coding
	/*
	gSetting_report.retry_count_connect = conf->model.tcp_connect_retry_count;
	gSetting_report.retry_count_send = conf->model.tcp_send_retry_count;
	gSetting_report.timeout_secs_receive = conf->model.tcp_send_retry_count;
	*/
	
	// request :: server info setting
//	strncpy(gSetting_request.ip, conf->model.request_ip, 40);
//	gSetting_request.port = conf->model.request_port;
	
	// request :: network socket api setting --> for hard coding
	/*
	gSetting_request.retry_count_connect = conf->model.tcp_connect_retry_count;
	gSetting_request.retry_count_send = conf->model.tcp_send_retry_count;
	gSetting_request.timeout_secs_receive = conf->model.tcp_send_retry_count;
	*/
	return 0;
}

int get_report_network_param(transferSetting_t* param)
{
	// report :: server info setting 
	strncpy(param->ip, gSetting_report.ip, 40);
	param->port = gSetting_report.port;
	
	// report :: network socket api setting
	param->retry_count_connect = DEFAULT_SETTING_SOCK_CONN_RETRY_CNT;
	param->retry_count_send = DEFAULT_SETTING_SOCK_SEND_RETRY_CNT;
	param->retry_count_receive = DEFAULT_SETTING_SOCK_RCV_RETRY_CNT;
	param->timeout_secs = DEFAULT_SETTING_SOCK_TIMEOUT_SEC;

	return 0;
}

int get_request_network_param(transferSetting_t* param)
{
	// report :: server info setting 
	strncpy(param->ip, gSetting_request.ip, 40);
	param->port = gSetting_request.port;
	
	// report :: network socket api setting
	param->retry_count_connect = DEFAULT_SETTING_SOCK_CONN_RETRY_CNT;
	param->retry_count_send = DEFAULT_SETTING_SOCK_SEND_RETRY_CNT;
	param->retry_count_receive = DEFAULT_SETTING_SOCK_RCV_RETRY_CNT;
	param->timeout_secs = DEFAULT_SETTING_SOCK_TIMEOUT_SEC;

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

static void _process_packet_recv(void* pdata)
{
	TRIPHOS_RESP_PACKET__SERVER_RESP_RCV prcv_packet;
	prcv_packet = *(TRIPHOS_RESP_PACKET__SERVER_RESP_RCV *) pdata;
	
	model_mds_server_req(prcv_packet.response);
	
	printf("------------ receive buff ---------------\r\n");
	debug_hexdump_buff((unsigned char *)&prcv_packet, sizeof(TRIPHOS_RESP_PACKET__SERVER_RESP_RCV));
	printf("-----------------------------------------\r\n");
}

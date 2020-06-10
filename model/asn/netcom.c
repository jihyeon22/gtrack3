#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <logd_rpc.h>


#include <callback.h>
#include "netcom.h"
#include "data-list.h"
#include "config.h"
#include "user_func.h"


transferSetting_t gSetting_report;

//extern char g_uart_buffer[1024*1024];
#define LIMIT_TRANSFER_PACKET_COUNT	100
int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int packet_length;
	unsigned char *p_packet;
	int list_count = 0;
	list_count = list_get_num(&cu_buffer_list);
	if(list_count <= 0)
	{
		LOGD(eSVC_MODEL, "%s> list count error %d\n", __func__, list_count);
		return -1;
	}
	
	if(list_pop(&cu_buffer_list, (void *)&p_packet) < 0)
		return -1;

	memcpy(&packet_length, p_packet, 4);
	*packet_len = packet_length;
	*packet_buf = p_packet;

	return 0;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	setting_network_param();
	int res;
	int retry = PACKET_RETRY_COUNT;
	int wait_count;

	LOGD(eSVC_MODEL, "\n\nsend_packet : op[%d]============>\n", op);
	//hdlc_async_print_data(packet_buf, packet_len);

	dump_data("network data packet", &packet_buf[4], packet_len);
	while(retry-- > 0) 
	{
		res = transfer_packet(&gSetting_report, &packet_buf[4], packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(WAIT_INTERVAL_TIME);
	}

	if(retry <= 0)
	{
		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			//LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			LOGD(eSVC_MODEL, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		//LOGI(LOG_TARGET, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		LOGD(eSVC_MODEL, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		return -1; //
	}

	LOGD(eSVC_MODEL, "\n\nsend_packet : res[%d]============>\n", res);
	return res;
}

int free_packet(void *packet)
{
	LOGD(eSVC_MODEL, "%s> asn ++\n", __func__);
	if(packet != NULL)
	{
		free(packet);
	}
	
	LOGD(eSVC_MODEL, "%s> asn --\n", __func__);
	return 0;
}

int setting_network_param(void)
{
	configurationModel_t *conf = get_config_model();
	strncpy(gSetting_report.ip, conf->model.report_ip, sizeof(gSetting_report.ip)-1);
	gSetting_report.port = conf->model.report_port;
	gSetting_report.retry_count_connect = 3;
	gSetting_report.retry_count_send = 3;
	gSetting_report.retry_count_receive = 3;
	gSetting_report.timeout_secs = 30;

	return 0;
}

int get_report_network_param(transferSetting_t* param)
{
    // report :: server info setting
	setting_network_param();

    strncpy(param->ip, gSetting_report.ip, sizeof(param->ip));
    param->port = gSetting_report.port;

    // report :: network socket api setting
    param->retry_count_connect = 3;
    param->retry_count_send = 3;
    param->retry_count_receive = 3;
    param->timeout_secs = 30;

    return 0;
}


int get_time_request_network_param(transferSetting_t* param)
{
	setting_network_param();

    // report :: server info setting
    strncpy(param->ip, gSetting_report.ip, sizeof(param->ip));
    param->port = gSetting_report.port;

    // report :: network socket api setting
    param->retry_count_connect = 0;
    param->retry_count_send =0;
    param->retry_count_receive = 0;
    param->timeout_secs = 1;

    return 0;
}
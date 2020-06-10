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

#include <config.h>
#include <callback.h>
#include "netcom.h"
#include "http_thermal_pkt.h"

transferSetting_t gHTTP_request;

unsigned char g_html_recv_buff[1024] = {0,};
static int _setting_network_param(void);


int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = -1;

	if ( op == PACKET_TYPE_THERMAL_HTTP)
	{
		res = make_http_thermal_pkt__send_themal_val(packet_buf, packet_len, (thermaldata_t *)param);
	}
	
	return 0;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res = 0;
	if (op == PACKET_TYPE_THERMAL_HTTP)
	{
		static int fail_retry_cnt = 0;
		int recv_buff_len = 0;
		int recv_ret = 0;

		
		_setting_network_param();
		
		recv_buff_len = sizeof(g_html_recv_buff);
		memset(&g_html_recv_buff, 0x00, recv_buff_len);

		recv_ret = transfer_packet_recv(&gHTTP_request, packet_buf, packet_len, (unsigned char *)&g_html_recv_buff, recv_buff_len);

		res = parse_http_thermal_pkt__send_themal_val(g_html_recv_buff, recv_ret);
		

	}
	return res;
}


static int _setting_network_param(void)
{
	configurationModel_t *conf = get_config_model();

	strncpy(gHTTP_request.ip, conf->model.request_http_ip, 40);
	gHTTP_request.port = conf->model.request_http_port;
	gHTTP_request.retry_count_connect = 2;
	gHTTP_request.retry_count_send = 2;
	gHTTP_request.retry_count_receive = 0;
	gHTTP_request.timeout_secs = 1;
	
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


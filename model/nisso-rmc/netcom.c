#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <base/config.h>
#include <at/at_util.h>
#include <base/watchdog.h>
#include <base/thermtool.h>
#include <board/power.h>
#include <board/led.h>
#include <board/battery.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include "callback.h"
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"
#include "config.h"

#include "server_resp.h"

#include <nisso_mdt800/packet.h>
#include <nisso_mdt800/gpsmng.h>
#include <nisso_mdt800/hdlc_async.h>
#include <nisso_mdt800/file_mileage.h>

#define SEND_MIRRORED_PACKET 0

transferSetting_t gSetting_report;
//transferSetting_t gSetting_request;
#if SEND_MIRRORED_PACKET
transferSetting_t gSetting_report_mds_mdt800 =
{
	.ip="219.251.4.178",
	.port = 30004,
	.retry_count_connect = 3,
	.retry_count_send = 3,
	.retry_count_receive = 3,
	.timeout_secs = 30
};
#endif



int make_period_packet(unsigned char **pbuf, unsigned short *packet_len)
{
	int i;
	int ret;
	unsigned short crc = 0;
	int enclen = 0;
	int packet_count = 0;
	unsigned char *p_encbuf;
	nisso_packet2_t *p_packet2 = NULL;
	int list_count = 0;
	etrace_packet_t *p_etr_pack = NULL;
	unsigned short extend_pack_idx = 0;
	time_t last_time;
	struct tm gps_time;
	time_t *p_utc_time_list = NULL;
	int data_length = 0;
	configurationModel_t * conf = get_config_model();

	list_count = list_get_num(&gps_buffer_list);
	if(list_count <= 0)
	{
		LOGE(LOG_TARGET, "%s> list count error %d\n", __func__, list_count);
		return -1;
	}

	if(list_count > LIMIT_TRANSFER_PACKET_COUNT)
		list_count = LIMIT_TRANSFER_PACKET_COUNT;

	create_report2_divert_buffer(&p_encbuf, list_count);

	if(ret < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;
	extend_pack_idx = 0;


	while(packet_count < list_count)
	{
		data_length = 0;
		crc = 0;

		//if(conf->model.packet_type == 81)
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
				break;
			data_length = sizeof(nisso_packet2_t);

			crc = crc8(crc, (unsigned char *)p_packet2, data_length);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet2, data_length);
			free(p_packet2);
			//enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
			//p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

		}
		

		packet_count += 1;
#if(FEATURE_DONT_USE_MERGED_PACKET)
		break;
#endif
	}

	if(crc == 0 && enclen == 0) {
		LOGE(LOG_TARGET, "gathered report packet have nothing.\n");
		free(p_encbuf);
		return -1;
	}

	*packet_len = enclen;
	*pbuf = p_encbuf;

	//p_encbuf : p_encbuf will free base code
	return 0;
}

int make_event2_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	nisso_packet2_t packet;
	int org_size = 0;

	gps_get_curr_data(&gpsdata);

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report2 divert buffer fail\n", __func__);
		return -1;
	}

	org_size = create_report2_data(eventCode, &packet, gpsdata, NULL, 0);
	LOGI(LOG_TARGET, "%s> report size %d\n", __func__, org_size);

	crc = crc8(crc, (unsigned char *)&packet, org_size);
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, org_size);
	//enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	//p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;

	return 0;
}

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{

	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);
	configurationModel_t * conf = get_config_model();

	switch(op)
	{
		case eCYCLE_REPORT_EVC:
			res = make_period_packet(packet_buf, packet_len);
			break;
		default:
			res = make_event2_packet(packet_buf, packet_len, op);
			printf("%s() -> %d\r\n", __func__, __LINE__);
			break;
	}
	return res;
}

int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int wait_count;
	char svr_resp[SERVER_RESP_MAX_LEN] = {0,};
	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);
	
	while(1) 
	{
		memset(&svr_resp, 0x00, sizeof(svr_resp));
		res = transfer_packet_recv_etx(&gSetting_report, packet_buf, packet_len, svr_resp, sizeof(svr_resp), ']');

		if(res == 0) //send and recv success
		{
			LOGI(LOG_TARGET, "%s> send success : get data >> \"%s\"\n", __func__, svr_resp);
			server_resp_proc(svr_resp);
			break;
		}

		LOGE(LOG_TARGET, "%s> Fail to send packet [%d]\n", __func__, op);

		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		
		if(op == eCYCLE_REPORT_EVC)
		{
			watchdog_set_cur_ktime(eWdNet1);
		}
		else
		{
			watchdog_set_cur_ktime(eWdNet2);
		}
	}

#if SEND_MIRRORED_PACKET
	int retry_mirrored = 2;
	while(retry_mirrored-- > 0) 
	{
		res = transfer_packet(&gSetting_report_mds_mdt800, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(5);
	}
#endif

	return 0;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int ret;
	setting_network_param(); //real-time ip/port change

	LOGI(LOG_TARGET, "ip %s : %d send packet!!\n", gSetting_report.ip, gSetting_report.port);
	ret = __send_packet(op, packet_buf, packet_len);
	if(ret < 0) {
		LOGE(LOG_TARGET, "op[%d] __send_packet error return\n", op);
		return -1;
	}

	LOGI(LOG_TARGET, "send_packet op[%d] send success\n", op);

	return 0;
}


int setting_network_param(void)
{
	configurationModel_t *conf = get_config_model();
	strncpy(gSetting_report.ip, conf->model.report_ip, 40);
	gSetting_report.port = conf->model.report_port;
	gSetting_report.retry_count_connect = 3;
	gSetting_report.retry_count_send = 3;
	gSetting_report.retry_count_receive = 3;
	gSetting_report.timeout_secs = 30;

	return 0;
}


int free_packet(void *packet)
{
	printf("===========================================\n");
	
	
	if(packet != NULL)
	{
		printf("free_packet ++\n");
		free(packet);
	}
	printf("===========================================\n");
	return 0;
}
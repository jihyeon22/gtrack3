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
#include <data-list.h>
#include <arpa/inet.h>
#include "logd/logd_rpc.h"
#include "debug.h"

#include <config.h>
#include <netcom.h>
#include "callback.h"
#include "lotte_packet.h"
#include "lotte_gpsmng.h"
#include "hdlc_async.h"
#include "lotte_file_mileage.h"

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
	unsigned short crc = 0;
	int enclen = 0;
	int packet_count = 0;
	unsigned char *p_encbuf;
	lotte_packet_t *p_packet;
	int list_count = 0;

	list_count = list_get_num(&gps_buffer_list);
	if(list_count <= 0)
	{
		LOGE(LOG_TARGET, "%s> list count error %d\n", __func__, list_count);
		return -1;
	}

	if(list_count > LIMIT_TRANSFER_PACKET_COUNT)
		list_count = LIMIT_TRANSFER_PACKET_COUNT;

	if(create_report_divert_buffer(&p_encbuf, list_count) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;
	
	while(packet_count < list_count)
	{
		crc = 0;
		if(list_pop(&gps_buffer_list, (void *)&p_packet) < 0)
			break;

		crc = crc8(crc, (unsigned char *)p_packet, sizeof(lotte_packet_t));
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet, sizeof(lotte_packet_t));
		free(p_packet);
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
		p_encbuf[enclen++] = LOTTE_PACKET_END_FLAG;

		packet_count += 1;
		//if(is_available_report_divert_buffer(packet_count++) == 0) {
		//	LOGT(LOG_TARGET, "report divert buffer full...\n");
		//	break;
		//}
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


int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	lotte_packet_t packet;

	gps_get_curr_data(&gpsdata);

	if(create_report_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	create_report_data(eventCode, &packet, gpsdata);

	crc = crc8(crc, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = LOTTE_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}



int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{

	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);
	
	switch(op)
	{
		case eCYCLE_REPORT_EVC:
			res = make_period_packet(packet_buf, packet_len);
			break;
		default:
			if(op > 0 && op < eMAX_EVENT_CODE) {
				res = make_event_packet(packet_buf, packet_len, op);
			}
			break;
	}

    save_vaild_data();
    
	return res;
}

int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int retry = PACKET_RETRY_COUNT;
	int wait_count;

	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);

	while(retry-- > 0) 
	{
		res = transfer_packet(&gSetting_report, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(WAIT_INTERVAL_TIME);
	}

#if SEND_MIRRORED_PACKET
	int retry_mirrored = PACKET_RETRY_COUNT;
	while(retry_mirrored-- > 0) 
	{
		res = transfer_packet(&gSetting_report_mds_mdt800, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(5);
	}
#endif

	if(retry <= 0)
	{
		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(5);
		}
		LOGI(LOG_TARGET, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		return -1; //
	}

	return res;
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

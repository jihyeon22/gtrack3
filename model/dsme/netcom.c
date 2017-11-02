#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/watchdog.h>
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
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "welding_machine.h"

#define LOG_TARGET eSVC_MODEL

transferSetting_t gSetting_report;

int make_period_packet(unsigned char **pbuf, unsigned short *packet_len)
{
	int packet_count = 0;
	int list_count = 0;
	int data_length = 0;
	char *p_packet = NULL;
	unsigned char *p_packet_buffer = NULL;
	char *p_welding_machine_id;


	list_count = list_get_num(&welding_machine_buffer_list);
	if(list_count <= 0)
	{
		printf("%s> list count error %d\n", __func__, list_count);
		LOGE(LOG_TARGET, "%s> list count error %d\n", __func__, list_count);
		return -1;
	}

	p_welding_machine_id = get_welding_machine_id();
	if(p_welding_machine_id == NULL) {
		LOGE(LOG_TARGET, "%s> welding_machine_id not yet\n", __func__);
		return -1;
	}

	p_packet_buffer = (unsigned char *)malloc(1024);
	if(p_packet_buffer == NULL) {
		LOGE(LOG_TARGET, "%s> p_packet_buffer malloc error %d\n", __func__);
		return -1;
	}

	memset(p_packet_buffer, 0x00, 1024);
	memcpy(&p_packet_buffer[data_length], p_welding_machine_id, strlen(p_welding_machine_id));
	data_length += strlen(p_welding_machine_id);

	packet_count = 0;
	while(packet_count < list_count)
	{

		if(list_pop(&welding_machine_buffer_list, (void *)&p_packet) < 0)
			break;

		if(data_length + strlen(p_packet) > 512) {
			printf("Max Size Over...#1\n");
			LOGI(LOG_TARGET, "%s> Max Size Over...#1\n", __func__);
			free(p_packet);
			break;
		}

		memcpy(&p_packet_buffer[data_length], p_packet, strlen(p_packet));
		data_length += strlen(p_packet);
		printf("data_length : %d\n", data_length);

		if(data_length > 450) {
			printf("Max Size Over...#2\n");
			LOGI(LOG_TARGET, "%s> Max Size Over...#2\n", __func__);
			free(p_packet);
			break;
		}

		packet_count += 1;
		free(p_packet);
	}

	*packet_len = data_length;
	*pbuf = p_packet_buffer;

	printf("\n\npacket data %s\n\n", p_packet_buffer);
	LOGI(LOG_TARGET, "\n\npacket data %s\n\n", p_packet_buffer);

	return 0;
}

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);

	switch(op)
	{
		case eREPORT_USER_DATA:
			res = make_period_packet(packet_buf, packet_len);
		break;
	}

	return res;
}


int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int wait_count;

	while(1) 
	{
		res = transfer_packet(&gSetting_report, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		LOGE(LOG_TARGET, "%s> Fail to send packet [%d]\n", __func__, op);

		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		
		if(op == eREPORT_USER_DATA)
		{
			watchdog_set_cur_ktime(eWdNet1);
		}
		else
		{
			watchdog_set_cur_ktime(eWdNet2);
		}
	}

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

int free_packet(void *packet)
{
	if(packet != NULL)
	{
		free(packet);
	}
	
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

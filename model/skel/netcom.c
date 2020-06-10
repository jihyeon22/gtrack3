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
#include "config.h"

transferSetting_t gSetting_report;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	return 0;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
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
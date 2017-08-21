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
#include "logd/logd_rpc.h"

#include <board/board_system.h>

#include <netcom.h>
#include <report.h>
#include <config.h>
#include "callback.h"

#define LOG_ENABLE_PACKET 0

//jwrho persistant data path modify++
//#define LOG_PACKET_PATH "/data/mds/data/packet"
#define LOG_PACKET_PATH CONCAT_STR(USER_DATA_DIR, "/packet")
//jwrho persistant data path modify--

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

transferSetting_t gSetting_report;
transferSetting_t gSetting_request;

int make_partial_period_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata)
{
	unsigned char *encbuf;
	int enclen;
	int pwr = 0;

	encbuf = malloc(sizeof(struct report_packet) * 2);
	if(encbuf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

	if(power_get_power_source() == POWER_SRC_DC)
	{
		pwr = 0;
	}
	else
	{
		pwr = 1;
	}
	enclen = report_make_packet(encbuf, REPORT_PERIOD_EVENT, gpsdata, pwr);

	*pbuf = encbuf;
	*packet_len = enclen;

#if 1
	printf("%s():\n", __func__);
	report_print_encoded_packet(encbuf, enclen);
#endif
	return 0;
}

int make_period_packet(unsigned char **pbuf, unsigned short *packet_len)
{
	int res = 0;
	unsigned char *encbuf;
	unsigned char *rebuf;
	int enclen = 0;

	encbuf = NULL;

	periodData_t *temp_period;
	while(1)
	{

		res = list_pop(&packet_list, (void *)&temp_period);
		if(res < 0)
		{
			break;
		}

		if(encbuf == NULL)
		{
			encbuf = malloc(temp_period->enclen);
			if(encbuf == NULL)
			{
				printf("malloc fail - %s\n", __FUNCTION__);
				free(temp_period->encbuf);
				free(temp_period);
				return -1;
			}
		}
		else
		{
			rebuf = realloc(encbuf, enclen + temp_period->enclen);
			if(rebuf == NULL)
			{
				printf("realloc fail - %s\n", __FUNCTION__);
				free(temp_period->encbuf);
				free(temp_period);
				break;
			}
			else
			{
				encbuf = rebuf;
			}
		}
		memcpy(encbuf + enclen, temp_period->encbuf, temp_period->enclen);
		enclen += temp_period->enclen;
		free(temp_period->encbuf);
		free(temp_period);
	}

	if(enclen == 0)
	{
		return -1;
	}
	else
	{
		int i = 0;
		printf("dump data %d\n[", enclen);
		for(i = 0; i < enclen; i++)
		{
			printf("%02x ", encbuf[i]);
		}
		printf("]\n");
	}

	*packet_len = enclen;
	*pbuf = encbuf;
	return 0;
}

int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode, int powerType)
{
	uint8_t *encbuf;
	int enclen;
	gpsData_t gpsdata;
	gps_get_curr_data(&gpsdata);
	encbuf = malloc(sizeof(struct report_packet) * 2);
	if(encbuf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	enclen = report_make_packet(encbuf, eventCode, &gpsdata, powerType);
	*packet_len = enclen;
	*pbuf = encbuf;
#if 1
	printf("%s():\n", __func__);
	report_print_encoded_packet(encbuf, enclen);
#endif
	return 0;
}

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	switch(op)
	{
		case REPORT_PERIOD_EVENT:
			LOGD(LOG_TARGET, "make_period_packet\n");
			res = make_period_packet(packet_buf, packet_len);
			break;
		case REPORT_SET_IP:
		case REPORT_SET_INTERVAL:
		case REPORT_SET_MILEAGE:
		case REPORT_STATUS:
		case REPORT_RESET:
		case REPORT_TURNON_EVENT:
		case REPORT_TURNOFF_EVENT:
		case REPORT_DEPARTURE_EVENT:
		case REPORT_RETURN_EVENT:
		case REPORT_ARRIVAL_EVENT:
		case REPORT_WAITING_EVENT:
		case REPORT_POWER_EVENT:
			LOGD(LOG_TARGET, "make_event_packet %x\n", op);
			res = make_event_packet(packet_buf, packet_len, op, *(int *)param);
			break;
		default:
			res = -1;
	}
	return res;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int trynum = 1;
	int timeout = 2;
	int res = 0;
	
	debug_hexdump_buff(packet_buf, packet_len);
#if LOG_ENABLE_PACKET
	tools_write_data(LOG_PACKET_PATH, packet_buf, packet_len, true);
#endif

	while(1)
	{
		res = transfer_packet(&gSetting_report, packet_buf, packet_len);
		if(res == 0 || (trynum >= timeout && timeout != 0)) {
			break;
		}
		trynum++;
	}
	return res;
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
	gSetting_report.retry_count_connect = conf->model.tcp_connect_retry_count;
	gSetting_report.retry_count_send = conf->model.tcp_send_retry_count;
	gSetting_report.retry_count_receive = conf->model.tcp_receive_retry_count;
	gSetting_report.timeout_secs = conf->model.tcp_timeout_secs;

	strncpy(gSetting_request.ip, conf->model.request_ip, 40);
	gSetting_request.port = conf->model.request_port;
	gSetting_request.retry_count_connect = conf->model.tcp_connect_retry_count;
	gSetting_request.retry_count_send = conf->model.tcp_send_retry_count;
	gSetting_request.retry_count_receive = conf->model.tcp_receive_retry_count;
	gSetting_request.timeout_secs = conf->model.tcp_timeout_secs;
	return 0;
}


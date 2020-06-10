#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>

#include <base/error.h>
#include <base/mileage.h>
#include <base/watchdog.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include "callback.h"
#include "netcom.h"
#include "packet.h"
#include "config.h"
#include "command.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static int _setting_network_param(void);

transferSetting_t gSetting_report;
transferSetting_t gSetting_request;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	
	switch(op)
	{
		case PACKET_TYPE_EVENT:
		{
			locationData_t temp_loc;

			LOGD(LOG_TARGET, "make_packet event %d\n", *((char *)param));
			
			temp_loc.acc_status = get_key_stat();
			temp_loc.event_code = *((char *)param);
			temp_loc.mileage_m = 0;
			gps_get_curr_data(&temp_loc.gpsdata);
			temp_loc.avg_speed = 0;
			res = make_event_packet(packet_buf, packet_len, &temp_loc);
			break;
		}
		
		case PACKET_TYPE_REPORT:
		{
			res = make_report_packet(packet_buf, packet_len);
			break;
		}

		case PACKET_TYPE_RFID:
		{
			locationData_t temp_loc;

			LOGD(LOG_TARGET, "make_packet rfid\n");
			
			temp_loc.acc_status = get_key_stat();
			temp_loc.event_code = CL_RFID_BOARDING_CODE;
			temp_loc.mileage_m = 0;
			gps_get_curr_data(&temp_loc.gpsdata);
			temp_loc.avg_speed = 0;
			
			res = make_rfid_packet(packet_buf, packet_len, &temp_loc, (rfidData_t *)param);
			break;
		}
		
		case PACKET_TYPE_MSI:
		{
			res = make_msi_packet(packet_buf, packet_len, ((paramMsi_t *)param)->ip, ((paramMsi_t *)param)->port);
			break;
		}

		case PACKET_TYPE_MIT:
		{
			res = make_mit_packet(packet_buf, packet_len, ((paramMit_t *)param)->interval, ((paramMit_t *)param)->max_packet);
			break;
		}

		case PACKET_TYPE_CSS:
		{
			res = make_css_packet(packet_buf, packet_len, *((int *)param));
			break;
		}

		case PACKET_TYPE_RAW:
		{
			res = make_raw_packet(packet_buf, packet_len, (bufData_t *)param);
			break;
		}

		default:
			res = -1;
	}
	
	return res;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res = 0;
	int ret = 0;
	CL_RESP_PACKET resp =
	{
		.data  = {0}
	};

	LOGT(LOG_TARGET, "send packet\n");

	while(1)
	{
		watchdog_set_cur_ktime(eWdNet1);
		
		_setting_network_param();
		LOGT(LOG_TARGET, "send_packet op:%d", op);
		
		ret = transfer_packet_recv_etx(&gSetting_report, packet_buf, packet_len, (unsigned char *)&resp, sizeof(resp), ']');
		if(ret == 0)
		{
			//ret = cl_resp_packet_check(resp.error_code);
			LOGT(LOG_TARGET, "Responce code %x", ret);
			switch(resp.error_code)
			{
				case CL_SUC_ERROR_CODE:
				case CL_CHK_ERROR_CODE:
				case CL_LEN_ERROR_CODE:
				{
					res = 0;
					break;
				}
				case CL_AUT_ERROR_CODE:
				{
					LOGE(LOG_TARGET, "AUTH ERROR : ALL OPERATION IS STOPPED!!!");
					error_critical(eERROR_FINAL, "AUTH ERROR : ALL OPERATION IS STOPPED!!!");
					//�ܸ��� ��ü ���� ����
					//Critical Final�� �¿���
					break;
				}
				case CL_SET_ERROR_CODE:
				{
					LOGT(LOG_TARGET, "NEED CONFIG : HAVE TO SET CONFIG {%s}", resp.data);
					process_cmd(resp.data);
					break;
				}
				default:;
			}
			break;
		}
		else
		{
			sleep(60);
			continue;
		}
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

static int _setting_network_param(void)
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


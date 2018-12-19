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
#include "cl_mdt_pkt.h"
#include "config.h"
#include "command.h"

#include "ext/rfid/cl_rfid_tools.h"


// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static int _setting_network_param(void);

transferSetting_t gSetting_report;
transferSetting_t gSetting_request;
transferSetting_t gRFID_request;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
    
	switch(op)
	{
		/* not used?!?!
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
		*/
		
		case PACKET_TYPE_REPORT:
		{
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_REPORT\r\n");
			res = make_report_packet(packet_buf, packet_len, *((int *)param));
			break;
		}

		case PACKET_TYPE_RFID:
		{
			locationData_t temp_loc;

			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_RFID\r\n");
			
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
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_MSI\r\n");
			res = make_msi_packet(packet_buf, packet_len, ((paramMsi_t *)param)->ip, ((paramMsi_t *)param)->port);
			break;
		}

		case PACKET_TYPE_MIT:
		{
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_MIT\r\n");
			res = make_mit_packet(packet_buf, packet_len, ((paramMit_t *)param)->interval, ((paramMit_t *)param)->max_packet);
			break;
		}

		case PACKET_TYPE_CSS:
		{
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_CSS\r\n");
			res = make_css_packet(packet_buf, packet_len, *((int *)param));
			break;
		}

		case PACKET_TYPE_RAW:
		{
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_RAW\r\n");
			res = make_raw_packet(packet_buf, packet_len, (bufData_t *)param);
			break;
		}
		case PACKET_TYPE_HTTP_GET_PASSENGER_LIST:
		{
            char tmp_buff[512] = {0,};

            strcpy(tmp_buff, "0");
            LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_HTTP_GET_PASSENGER_LIST\r\n");

            if ( param != NULL )
                strcpy(tmp_buff, ((char *)param));

            rfid_tool__set_senario_stat(e_RFID_DOWNLOAD_START);
            res = make_clrfid_pkt__req_passenger(packet_buf, packet_len,tmp_buff);
            break;
		}
		case PACKET_TYPE_HTTP_SET_BOARDING_LIST:
		{
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_HTTP_SET_BOARDING_LIST\r\n");
			res = make_clrfid_pkt__set_boarding(packet_buf, packet_len, (RFID_BOARDING_MGR_T *)param);
			break;
		}
        case PACKET_TYPE_ADAS_EVENT:
        {
            gpsData_t gpsdata;
            int adas_evt = 0;
            char adas_opt[128] = {0,};
            
            gps_get_curr_data(&gpsdata);
			LOGD(LOG_TARGET, "[MAKE PKT] PACKET_TYPE_ADAS_EVENT\r\n");
            
			res = make_mpa_packet(packet_buf, packet_len, &gpsdata, (clAdasData_t *)param);
			break;
		}

		default:
			res = -1;
	}
	
	return res;
}

unsigned char g_html_recv_buff[128*MAX_RFID_USER_SAVE];

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res = 0;
	int ret = 0;
	CL_RESP_PACKET resp =
	{
		.data  = {0}
	};


	if ( op == PACKET_TYPE_HTTP_GET_PASSENGER_LIST )
	{
		static int fail_retry_cnt = 0;
		int recv_buff_len = 0;
		int recv_ret = 0;

		LOGT(LOG_TARGET, "[SEND PKT] PACKET_TYPE_HTTP_GET_PASSENGER_LIST\r\n");

		watchdog_set_cur_ktime(eWdNet1);
		
		_setting_network_param();
		
		recv_buff_len = sizeof(g_html_recv_buff);
		memset(&g_html_recv_buff, 0x00, recv_buff_len);

		recv_ret = transfer_packet_recv(&gRFID_request, packet_buf, packet_len, (unsigned char *)&g_html_recv_buff, recv_buff_len);

		res = parse_clrfid_pkt__req_passenger(g_html_recv_buff, recv_ret);
		
		if ( res < 0 )
			rfid_tool__set_senario_stat(e_RFID_INIT); // err fix
		else
			rfid_tool__set_senario_stat(e_RFID_DOWNLOAD_END);

		return 0;
	}

	if ( op == PACKET_TYPE_HTTP_SET_BOARDING_LIST )
	{
		static int fail_retry_cnt = 0;
		int recv_buff_len = 0;
		int recv_ret = 0;

		LOGT(LOG_TARGET, "[SEND PKT] PACKET_TYPE_HTTP_SET_BOARDING_LIST\r\n");
		watchdog_set_cur_ktime(eWdNet1);
		
		_setting_network_param();
		
		recv_buff_len = sizeof(g_html_recv_buff);
		memset(&g_html_recv_buff, 0x00, recv_buff_len);

		recv_ret = transfer_packet_recv(&gRFID_request, packet_buf, packet_len, (unsigned char *)&g_html_recv_buff, recv_buff_len);

		res = parse_clrfid_pkt__set_boarding(g_html_recv_buff, recv_ret);
		

		return res;
	}


	while(1)
	{
		LOGT(LOG_TARGET, "[SEND PKT] NORMAL PKT [%d] \r\n", op);
		watchdog_set_cur_ktime(eWdNet1);
	
		_setting_network_param();
		
		ret = transfer_packet_recv_etx(&gSetting_report, packet_buf, packet_len, (unsigned char *)&resp, sizeof(resp), ']');
		if(ret == 0)
		{
			//ret = cl_resp_packet_check(resp.error_code);
			LOGT(LOG_TARGET, "  >> recv form server:: Responce code %x \r\n", ret);
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
					LOGE(LOG_TARGET, "AUTH ERROR : ALL OPERATION IS STOPPED!!!\r\n");
					error_critical(eERROR_FINAL, "AUTH ERROR : ALL OPERATION IS STOPPED!!!\r\n");
					//�ܸ��� ��ü ���� ����
					//Critical Final�� �¿���
					break;
				}
				case CL_SET_ERROR_CODE:
				{
					LOGT(LOG_TARGET, "NEED CONFIG : HAVE TO SET CONFIG {%s}\r\n", resp.data);
					process_cmd(resp.data);
                    res = 0;
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

	gSetting_request.retry_count_connect = conf->model.tcp_connect_retry_count;
	gSetting_request.retry_count_send = conf->model.tcp_send_retry_count;
	gSetting_request.retry_count_receive = conf->model.tcp_receive_retry_count;
	gSetting_request.timeout_secs = conf->model.tcp_timeout_secs;
	
	strncpy(gRFID_request.ip, conf->model.request_rfid, 40);
	gRFID_request.port = conf->model.request_rfid_port;
	gRFID_request.retry_count_connect = 2;
	gRFID_request.retry_count_send = 2;
	gRFID_request.retry_count_receive = 0;
	gRFID_request.timeout_secs = 1;
	
	return 0;
}

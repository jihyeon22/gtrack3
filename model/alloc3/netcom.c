#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
//#include <base/at.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <config.h>
#include <callback.h>
#include "netcom.h"

#include "alloc_packet.h"
#include "alloc_packet_tool.h"
#include "tagging.h"
#include "geofence.h"

// jhcho test 
#include "color_printf.h"
// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL
transferSetting_t gSetting_report;
//transferSetting_t gSetting_request;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	char phone[AT_LEN_PHONENUM_BUFF] = {0};

	at_get_phonenum(phone, sizeof(phone));
	
	switch(op)
	{
		case PACKET_TYPE_EVENT:
		{
			alloc_evt_pkt_info_t evtpkt_data;
			int pkt_len = 0;
			
			evtpkt_data = *(alloc_evt_pkt_info_t *)param;

			pkt_len = mkpkt_report_data(	packet_buf, 
											evtpkt_data.evt_code,
											"",
											phone,
											PKT_PRI_DEF_TEMP_SENSOR_OPEN,
											PKT_PRI_DEF_TEMP_SENSOR_SHORT,
											PKT_PRI_DEF_TEMP_SENSOR_NONE,
											evtpkt_data.gpsdata,
											"0",
											0,
											0,
											evtpkt_data.diff_distance);
			* packet_len = pkt_len;
			
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			break;
		}
		case PACKET_TYPE_GET_RFID:
		{
			alloc_get_rfid_pkt_info_t get_rfid_data;
			int pkt_len = 0;
			
			get_rfid_data = *(alloc_get_rfid_pkt_info_t *)param;
			
			pkt_len = mkpkt_get_passenger(	packet_buf, 
											phone,
											get_rfid_data.version);
			
			* packet_len = pkt_len;
			
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			break;
		}
		
		case PACKET_TYPE_REPORT:
		{
			alloc_evt_pkt_info_t reportpkt_data;
			int pkt_len = 0;
			
			reportpkt_data = *(alloc_evt_pkt_info_t *)param;
			//devel_webdm_send_log("PKT_REPORT dist:%d", reportpkt_data.diff_distance);
			//printf(" reportpkt_data.gpsdata.lat [%f]\r\n", reportpkt_data.gpsdata.lat);
			//printf(" reportpkt_data.gpsdata.lon [%f]\r\n", reportpkt_data.gpsdata.lon);
			
			pkt_len = mkpkt_report_data(	packet_buf, 
											eEVT_CODE_PERIOD_REPORT,
											"",
											phone,
											PKT_PRI_DEF_TEMP_SENSOR_OPEN,
											PKT_PRI_DEF_TEMP_SENSOR_SHORT,
											PKT_PRI_DEF_TEMP_SENSOR_NONE,
											reportpkt_data.gpsdata,
											"0",
											0,
											0,
											reportpkt_data.diff_distance);
			
			* packet_len = pkt_len;
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			//printf("packet len is [%d]\r\n", pkt_len);
			
			break;
		}
		case PACKET_TYPE_TAGGING:
		{
			int pkt_len = 0;
			char temp_fence_id[12] = {0};

			taggingData_t *td = (taggingData_t *)param;

			if(td->idx_geo_fence == -1)
			{
//				td->idx_geo_fence = get_first_geo_fence();
				temp_fence_id[0] = '0';
			}
			else
			{
				get_geo_fence_id(td->idx_geo_fence, temp_fence_id);
			}
			//devel_webdm_send_log("PKT_TAGGING cnt:%d ntag:%d zid:%s", td->count, td->tagging_num, temp_fence_id);
			pkt_len = mkpkt_tag_data( packet_buf,
										phone,
										td->count,
										temp_fence_id,
										td->date,
										td->tagging_num,
										td->tagging_data);
										
			* packet_len = pkt_len;
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			//printf("packet len is [%d]\r\n", pkt_len);
			
			break;			
		}
		case PACKET_TYPE_GET_GEOFENCE:
		{
			alloc_evt_pkt_info_t reportpkt_data;
			int pkt_len = 0;
			
			reportpkt_data = *(alloc_evt_pkt_info_t *)param;
			
			//printf(" reportpkt_data.gpsdata.lat [%f]\r\n", reportpkt_data.gpsdata.lat);
			//printf(" reportpkt_data.gpsdata.lon [%f]\r\n", reportpkt_data.gpsdata.lon);
			
			pkt_len = mkpkt_report_data(	packet_buf, 
											eEVT_CODE_PERIOD_REPORT,
											"",
											phone,
											PKT_PRI_DEF_TEMP_DEV_GET_STOP,
											PKT_PRI_DEF_TEMP_SENSOR_SHORT,
											PKT_PRI_DEF_TEMP_SENSOR_NONE,
											reportpkt_data.gpsdata,
											"0",
											0,
											0,
											reportpkt_data.diff_distance);
			
			* packet_len = pkt_len;
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			//printf("packet len is [%d]\r\n", pkt_len);
			break;
		}
		case PACKET_TYPE_GEO_FENCE_IN:
		case PACKET_TYPE_GEO_FENCE_OUT:
		{
			alloc_evt_pkt_info_t evt_data;
			int pkt_len = 0;
			
			evt_data = *(alloc_evt_pkt_info_t *)param;
			printf("dddd-1\r\n");
			//devel_webdm_send_log("PKT_GEOFENCE zid:%s st:%d dist:%d", evt_data.zone_id, evt_data.zone_stat, evt_data.diff_distance);
			pkt_len = mkpkt_report_data(	packet_buf, 
											eEVT_CODE_PERIOD_REPORT,
											"",
											phone,
											PKT_PRI_DEF_TEMP_SENSOR_OPEN,
											PKT_PRI_DEF_TEMP_SENSOR_SHORT,
											PKT_PRI_DEF_TEMP_SENSOR_NONE,
											evt_data.gpsdata,
											evt_data.zone_id,
											evt_data.zone_idx,
											evt_data.zone_stat,
											evt_data.diff_distance);
			
			* packet_len = pkt_len;
			printf("dddd-2\r\n");
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			//printf("packet len is [%d]\r\n", pkt_len);
			
			break;
		}
		case PACKET_TYPE_RESP_SMS:
		{
			int pkt_len = 0;
			
			pkt_len = mkpkt_sms_resp_tcp_data( packet_buf,
										phone);
										
			* packet_len = pkt_len;
			//printf("send packet_buf is [%s]\r\n", *packet_buf);
			//printf("packet len is [%d]\r\n", pkt_len);
			
			break;	
		}
		//jwrho ++
		case PACKET_TYPE_REQUEST_BUS_STOP_INFO:
		{
			int pkt_len = 0;
			
			pkt_len = mkpkt_bus_stop_info_tcp_data( packet_buf, phone);
										
			* packet_len = pkt_len;
			break;
		}
		//jwrho --
		default:
		break;
		
	}
	
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


int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	transferSetting_t network_param = {0,};
	int ret = 0;
	 
	LOGT(LOG_TARGET, "send packet\n");
	
	/*
	switch(op)
	{
		case PACKET_TYPE_REPORT:
		break;
	}
	*/
	
	//printf("send pcket?!?!?!?!\r\n");
	
	//while(1)
	{
		char dbg_format[64] = {0};
		
		get_report_network_param(&network_param);

		LOGI(LOG_TARGET,"send packet - server is2 [%s]:[%d] \n",network_param.ip, network_param.port);

		sprintf(dbg_format, "send_packet2 [%%.%ds] / [%%d]\r\n",packet_len);
		//printf("dbg_format %s\n", dbg_format);
		printf(dbg_format, packet_buf, packet_len);
		
		ret = transfer_packet_recv_call(op, &network_param, packet_buf, packet_len);

		if(ret < 0) {
			printf("op[%d] __send_packet error return\n", op);
			//LOGE(LOG_TARGET, "op[%d] __send_packet error return\n", op);
			return -1;
		}

		//jwrho ++
		if(op == PACKET_TYPE_REQUEST_BUS_STOP_INFO)
			return 0; //requesting bus-top packet unconditionally throw away even if fail.
		//jwrho --
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
	gSetting_report.timeout_secs = 10;

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


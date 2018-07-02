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

#include <kt_thermal_mdt800/packet.h>
#include <kt_thermal_mdt800/gpsmng.h>
#include <kt_thermal_mdt800/hdlc_async.h>
#include <kt_thermal_mdt800/file_mileage.h>

#include "ktth_adas_mgr.h"

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
	lotte_packet2_t *p_packet2 = NULL;

	int list_count = 0;

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

	ret = create_report2_divert_buffer(&p_encbuf, list_count);

	if(ret < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;

	while(packet_count < list_count)
	{
		data_length = 0;
		crc = 0;

			
		if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
			break;

		data_length = sizeof(lotte_packet2_t) - 100 + p_packet2->record_leng;

		crc = crc8(crc, (unsigned char *)p_packet2, data_length);
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet2, data_length);
		free(p_packet2);
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
		p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;
	

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
	lotte_packet2_t packet;
	int org_size = 0;
	char record[100] = {0};
	int rec_len = 0;

	gps_get_curr_data(&gpsdata);

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report2 divert buffer fail\n", __func__);
		return -1;
	}

#if 0
	int cur_car_volt = battery_get_battlevel_car() / 1000;
	rec_len = sprintf(record, ">BATVOLT:%d<", cur_car_volt % 100);
#else
	rec_len = make_record_thermal(record);
#endif

	org_size = create_report2_data(eventCode, &packet, gpsdata, record, rec_len);
	LOGI(LOG_TARGET, "%s> report size %d\n", __func__, org_size);

	crc = crc8(crc, (unsigned char *)&packet, org_size);
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, org_size);
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}


int make_event2_packet_adas(unsigned char **pbuf, unsigned short *packet_len, ktthAdasData_t* adas_data)
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	lotte_packet2_t packet;
	int org_size = 0;
	char record[100] = {0};
	int rec_len = 0;


    // event code is normal report code
    int eventCode = adas_data->event_code;

	gps_get_curr_data(&gpsdata);

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report2 divert buffer fail\n", __func__);
		return -1;
	}


    rec_len = sprintf(record, ">%s<", adas_data->adas_data_str);
	//rec_len = make_record_thermal(record);

	org_size = create_report2_data(eventCode, &packet, gpsdata, record, rec_len);
	LOGI(LOG_TARGET, "%s> report size %d\n", __func__, org_size);

	crc = crc8(crc, (unsigned char *)&packet, org_size);
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, org_size);
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

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
        case eMDS_CUSTOM_KT_ADAS_EVC:
            res = make_event2_packet_adas(packet_buf, packet_len, (ktthAdasData_t *)param);
            break;
		default:
			if(op > 0 && op < eMAX_EVENT_CODE)
			{
				{
					res = make_event2_packet(packet_buf, packet_len, op);
				}
			}
			break;
	}
	return res;
}

#define SERVER_RESP_MAX_LEN 1024
int server_resp_proc(char* resp_buff)
{
    int buff_len = strlen(resp_buff);
    char resp_str[SERVER_RESP_MAX_LEN] = {0,};

    LOGI(LOG_TARGET, "%s> resp proc :: [%s]\r\n", __func__, resp_buff);
    
    // check valid resp
    if ( ( resp_buff[0] != '[') || ( resp_buff[buff_len-1] != ']') )
    {
        LOGE(LOG_TARGET, "%s> resp proc :: error invalid data [%s]\r\n", __func__, resp_buff);
        return -1;
    }

    strncpy(resp_str, resp_buff+1, buff_len-2);

    // &12,1,0,2,37.4756163,126.8818914,60,1,2,00.0000000,000.0000000,0,2,2,00.0000000,000.0000000,0,1

    if ( strcmp(resp_str, "0") == 0 )
    {
        LOGI(LOG_TARGET, "%s> resp proc :: normal resp return.\n", __func__);
        return 1;
    }

    return -1;

}

int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int wait_count;
	char svr_resp[SERVER_RESP_MAX_LEN] = {0,};
	int ret_val = -1;
	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);
	
	while(1) 
	{
		memset(&svr_resp, 0x00, sizeof(svr_resp));
		res = transfer_packet_recv_etx(&gSetting_report, packet_buf, packet_len, svr_resp, sizeof(svr_resp), ']');

		if(res == 0) //send and recv success
		{
			LOGI(LOG_TARGET, "%s> send success : get data >> \"%s\"\n", __func__, svr_resp);
			ret_val = server_resp_proc(svr_resp);
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

	return ret_val;
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
    gps_valid_data_write();
    save_mileage_file(get_server_mileage() + get_gps_mileage());

	printf("===========================================\n");

	if(packet != NULL)
	{
		printf("free_packet ++\n");
		free(packet);
	}
	printf("===========================================\n");
	return 0;
}

int make_record_thermal(char *record)
{
	int rec_len = 0;
	THERMORMETER_DATA tmp_therm;
	int temp_data[5] = {0,};
	int temp_stat[5] = {5,};

	int i = 0;

	for(i = 0 ; i < 5 ; i++)
	{
		temp_stat[i] = 5;
	}

	if(therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		float temper = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
				{
					temp_data[i] = tmp_therm.temper[i].data;
					temp_stat[i] = 1;
					break;
				}
				case eOPEN:
				{
					temp_data[i] = 0;
					temp_stat[i] = 2;
					break;
				}
				case eSHORT:
				{
					temp_data[i] = 0;
					temp_stat[i] = 3;
					break;
				}
				case eUNUSED:
				case eNOK:
				default:
				{
					temp_data[i] = 0;
					temp_stat[i] = 4;
					break;
				}
			}
			LOGI(LOG_TARGET, "%s> CH-%d : %d C\n", __func__, i, temp_data[i]);
		}
	}

	rec_len = sprintf(record, ">%d,%d,%d,%d<",
        temp_stat[0], temp_data[0], temp_stat[1], temp_data[1]);
    
    // debug 

    //devel_webdm_send_log(record);

	return rec_len;
}

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

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/hdlc_async.h>
#include <mdt800/file_mileage.h>

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
	lotte_packet_t *p_packet = NULL;
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

	if(conf->model.packet_type == 64 || conf->model.packet_type == 65)
		ret = create_report_divert_buffer(&p_encbuf, list_count);
	else
		ret = create_report2_divert_buffer(&p_encbuf, list_count);

	if(ret < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;
	extend_pack_idx = 0;

	if( !(conf->model.packet_type == 64 || conf->model.packet_type == 81))
	{

		p_etr_pack = (etrace_packet_t *)malloc(sizeof(etrace_packet_t) * list_count);
		printf("etrace extend packet size = [%d]\n", sizeof(etrace_packet_t) * list_count);
		if(p_etr_pack == NULL)
		{
			LOGE(LOG_TARGET, "%s> etrace extend packet malloc fail\n", __func__);
			return -1;
		}

		p_utc_time_list = (time_t *)malloc(sizeof(time_t) * list_count);
		printf("etrace extend utc time size = [%d]\n", sizeof(time_t) * list_count);
		if(p_utc_time_list == NULL)
		{
			LOGE(LOG_TARGET, "%s> utc time_t malloc fail\n", __func__);
			return -1;
		}
	}

	while(packet_count < list_count)
	{
		data_length = 0;
		crc = 0;

		if(conf->model.packet_type == 64)
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet) < 0)
				break;

			data_length = sizeof(lotte_packet_t);
			crc = crc8(crc, (unsigned char *)p_packet, data_length);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet, data_length);
			free(p_packet);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
			p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

		}
		else if(conf->model.packet_type == 65)
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet) < 0)
				break;

			if(packet_count != list_count-1)
			{
				if(p_packet->gps_pos.latitude == 0 || p_packet->gps_pos.longitude == 0)
				{
					packet_count += 1; //skip packet
					free(p_packet);
					continue;
				}
				else
				{
					printf("p_packet->date.year = [%04d/%02d/%02d]\n", p_packet->date.year, p_packet->date.mon, p_packet->date.day);
					gps_time.tm_year   = p_packet->date.year - 1900;   // ���� :�⵵�� 1900����� ����
					gps_time.tm_mon    = p_packet->date.mon - 1;      // ���� :���� 0���� ����
					gps_time.tm_mday   = p_packet->date.day;
					gps_time.tm_hour   = p_packet->date.hour;
					gps_time.tm_min    = p_packet->date.min;
					gps_time.tm_sec    = p_packet->date.sec;
					gps_time.tm_isdst  = 0;           // ��� Ÿ�� ��� ����

					p_utc_time_list[extend_pack_idx] = mktime(&gps_time);
					p_etr_pack[extend_pack_idx].time = 0;
					//printf("UTC = [%ld]\n", p_utc_time_list[extend_pack_idx]);
					memcpy(&p_etr_pack[extend_pack_idx].gps_pos, &p_packet->gps_pos, sizeof(gps_pos_t));
					extend_pack_idx += 1;
					free(p_packet);
				}
			}
			else //packet_count == list_count-1, meaning is last packet
			{
				p_packet->msg_type = 0x65;
				data_length = sizeof(lotte_packet_t);
				crc = crc8(crc, (unsigned char *)p_packet, data_length);

				gps_time.tm_year   = p_packet->date.year - 1900;   // ���� :�⵵�� 1900����� ����
				gps_time.tm_mon    = p_packet->date.mon - 1;      // ���� :���� 0���� ����
				gps_time.tm_mday   = p_packet->date.day;
				gps_time.tm_hour   = p_packet->date.hour;
				gps_time.tm_min    = p_packet->date.min;
				gps_time.tm_sec    = p_packet->date.sec;
				last_time = mktime(&gps_time);
				for(i = 0; i < extend_pack_idx; i++) {
					p_etr_pack[i].time = last_time - p_utc_time_list[i];
				}
				crc = crc8(crc, (unsigned char *)&extend_pack_idx, sizeof(extend_pack_idx));
				crc = crc8(crc, (unsigned char *)p_etr_pack, sizeof(etrace_packet_t)*extend_pack_idx);

				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet, data_length);
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&extend_pack_idx, sizeof(extend_pack_idx));
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_etr_pack, sizeof(etrace_packet_t)*extend_pack_idx);
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
				p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;
				free(p_packet);
				free(p_etr_pack);
				free(p_utc_time_list);
				
			}
		}
		else if(conf->model.packet_type == 81)
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
				break;

			data_length = sizeof(lotte_packet2_t) - 100 + p_packet2->record_leng;

			crc = crc8(crc, (unsigned char *)p_packet2, data_length);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet2, data_length);
			free(p_packet2);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
			p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

		}
		else
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
				break;

			if(packet_count != list_count-1)
			{
				if(p_packet2->gps_pos.latitude == 0 || p_packet2->gps_pos.longitude == 0)
				{
					packet_count += 1; //skip packet
					free(p_packet2);
					continue;
				}
				else
				{
					//printf("p_packet->date.year = [%04d/%02d/%02d]\n", p_packet2->date.year, p_packet2->date.mon, p_packet2->date.day);
					gps_time.tm_year   = p_packet2->date.year - 1900;   // ���� :�⵵�� 1900����� ����
					gps_time.tm_mon    = p_packet2->date.mon - 1;      // ���� :���� 0���� ����
					gps_time.tm_mday   = p_packet2->date.day;
					gps_time.tm_hour   = p_packet2->date.hour;
					gps_time.tm_min    = p_packet2->date.min;
					gps_time.tm_sec    = p_packet2->date.sec;
					gps_time.tm_isdst  = 0;           // ��� Ÿ�� ��� ����

					p_utc_time_list[extend_pack_idx] = mktime(&gps_time);
					p_etr_pack[extend_pack_idx].time = 0;
					//printf("UTC = [%ld]\n", p_utc_time_list[extend_pack_idx]);
					memcpy(&p_etr_pack[extend_pack_idx].gps_pos, &p_packet2->gps_pos, sizeof(gps_pos_t));
					extend_pack_idx += 1;
					free(p_packet2);
				}
			}
			else //packet_count == list_count-1, meaning is last packet
			{
				p_packet2->msg_type = 0x82;
				data_length = sizeof(lotte_packet2_t) - sizeof(p_packet2->record) + p_packet2->record_leng;
				crc = crc8(crc, (unsigned char *)p_packet2, data_length);

				gps_time.tm_year   = p_packet2->date.year - 1900;   // ���� :�⵵�� 1900����� ����
				gps_time.tm_mon    = p_packet2->date.mon - 1;      // ���� :���� 0���� ����
				gps_time.tm_mday   = p_packet2->date.day;
				gps_time.tm_hour   = p_packet2->date.hour;
				gps_time.tm_min    = p_packet2->date.min;
				gps_time.tm_sec    = p_packet2->date.sec;
				last_time = mktime(&gps_time);
				for(i = 0; i < extend_pack_idx; i++) {
					p_etr_pack[i].time = last_time - p_utc_time_list[i];
				}
				crc = crc8(crc, (unsigned char *)&extend_pack_idx, sizeof(extend_pack_idx));
				crc = crc8(crc, (unsigned char *)p_etr_pack, sizeof(etrace_packet_t)*extend_pack_idx);

				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet2, data_length);
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&extend_pack_idx, sizeof(extend_pack_idx));
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_etr_pack, sizeof(etrace_packet_t)*extend_pack_idx);
				enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
				p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

				//printf("extend pack count = [%d]\n", extend_pack_idx);
				//for(i = 0; i < extend_pack_idx; i++) {
				//	printf("%d, %d, %d\n", p_etr_pack[i].time, p_etr_pack[i].gps_pos.latitude, p_etr_pack[i].gps_pos.longitude);
				//}

				free(p_packet2);
				free(p_etr_pack);
				free(p_utc_time_list);
			}

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

int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	lotte_packet_t packet;

	gps_get_curr_data(&gpsdata);

	if ( ( gpsdata.active != eACTIVE ) || (gpsdata.lat == 0 ) || (gpsdata.lon == 0 ) ) 
	{
		gpsData_t last_gpsdata;
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
        LOGE(LOG_TARGET, "%s> gps invalid : fill last valid gps \n", __func__);
	}

	if(create_report_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	create_report_data(eventCode, &packet, gpsdata);

	crc = crc8(crc, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}

int make_event2_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode, int car_volt)
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

	if ( ( gpsdata.active != eACTIVE ) || (gpsdata.lat == 0 ) || (gpsdata.lon == 0 ) ) 
	{
		gpsData_t last_gpsdata;
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
        LOGE(LOG_TARGET, "%s> gps invalid : fill last valid gps \n", __func__);
	}
    
	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report2 divert buffer fail\n", __func__);
		return -1;
	}

#if 0
	int cur_car_volt = battery_get_battlevel_car() / 1000;
	rec_len = sprintf(record, ">BATVOLT:%d<", cur_car_volt % 100);
#else
	rec_len = make_record(record);
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
		case eREPORT_BATT:
			{
				if(conf->model.packet_type == 64 || conf->model.packet_type == 65)
				{
					res = make_event_packet(packet_buf, packet_len, op);
				}
				else
				{
					if(param == NULL)
					{	
						int cur_car_volt = battery_get_battlevel_car() / 1000;
						res = make_event2_packet(packet_buf, packet_len, op, cur_car_volt);
					}
					else
					{
						res = make_event2_packet(packet_buf, packet_len, op, *((int *)param));
					}
				}
			}
			break;
		default:
			if(op > 0 && op < eMAX_EVENT_CODE)
			{
				if(conf->model.packet_type == 64 || conf->model.packet_type == 65)
				{
					res = make_event_packet(packet_buf, packet_len, op);
				}
				else
				{
					if(param == NULL)
					{	
						int cur_car_volt = battery_get_battlevel_car() / 1000;
						res = make_event2_packet(packet_buf, packet_len, op, cur_car_volt);
					}
					else
					{
						res = make_event2_packet(packet_buf, packet_len, op, *((int *)param));
					}
				}
			}
			break;
	}
	return res;
}

int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int wait_count;

	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);

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

int make_record(char *record)
{
	int rec_len = 0;
	THERMORMETER_DATA tmp_therm;
	float temp1 = -555.5,temp2 = -555.5,temp3 = -555.5;

	int cur_car_volt = battery_get_battlevel_car() / 1000;

	if(therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		float temper = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
					temper =  tmp_therm.temper[i].data * 0.1;
					break;
					
				case eOPEN:
					temper = -555.5;
					break;
					
				case eSHORT:
					temper = -555.5;
					break;
					
				case eUNUSED:
				case eNOK:
				default:
					temper = -555.5;
			}
			LOGI(LOG_TARGET, "%s> CH-%d : %f C\n", __func__, i, temper);

			if(i  == 0)
				temp1 = temper;
			else if(i == 1)
				temp2 = temper;
			else if(i == 2)
				temp3 = temper;
		}
	}

	rec_len = sprintf(record, ">BATVOLT:%d;T1:%.1f;T2:%.1f;T3:%.1f<",
		cur_car_volt % 100, temp1, temp2, temp3);

	return rec_len;
}

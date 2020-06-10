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

#include "debug.h"
#include "netcom.h"
#include "config.h"

#include <kt_flood_mdt800/packet.h>
#include <kt_flood_mdt800/gpsmng.h>
#include <kt_flood_mdt800/hdlc_async.h>
#include <kt_flood_mdt800/file_mileage.h>

#include "leakdata-list.h"

#include "color_printf.h"

#include "transfer_food.h"

#define SEND_MIRRORED_PACKET 0

extern is_list_init;

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

int sever_save_interval = 0;
int is_sever_modify_ip = 0;
char g_sever_ip[40];

int make_flood_event_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
{
	flood_packet_t packet;
	int org_size = 0;

	org_size = create_flood_report_data(&packet);
	
	*packet_len = org_size;
	*pbuf = malloc(org_size);
	
	memcpy(*pbuf, (unsigned char *)&packet, org_size);

	return 0;
}

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{

	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);
	configurationModel_t * conf = get_config_model();
	
	switch(op)
	{
		// case eCYCLE_REPORT_EVC:
		// 	res = make_period_packet(packet_buf, packet_len);
		// 	break;

		default:
			if(op > 0 && op < eMAX_EVENT_CODE)
			{
				res = make_flood_event_packet(packet_buf, packet_len, op);
			}
			break;
	}

	return res;
}

#define SERVER_RESP_MAX_LEN 1024
int server_resp_proc(char* resp_buff)
{
    int buff_len = strlen(resp_buff);

	resp_packet_t *resp_packet;
	int i = 0;
	char p_str_resp_ip[40];
	int  n_resp_port;
	char p_str_resp_port[16];
	int warning_cnt = 0;
	configurationModel_t *conf = get_config_model();

	memset(&resp_packet, 0x00, sizeof(resp_packet_t));	
	resp_packet = (resp_packet_t *)&resp_buff[1];

	buff_len = ntohl(resp_packet->msg_len);

	printf("resp_buff... buff_len : %d \n-------------------------\n", buff_len);
	for(i == 0; i < buff_len; i++)
	{
		printf("%02x ",  resp_buff[i]);
		if(i%29 == 0 && i != 0)
			printf("\n");

	}
	printf("\n-------------------------\n");

	if ( ( resp_buff[0] != FLOOD_PACKET_START_FLAG) || ( resp_buff[buff_len-1] != FLOOD_PACKET_END_FLAG) )
    {
        LOGE(LOG_TARGET, "%s> resp proc :: error invalid data [%s]\r\n", __func__, resp_buff);
		print_yellow ("%s> resp proc :: error invalid data [%s]\r\n", __func__, resp_buff);
        return -1;
    }

	/* INTERVAL */
	printf("report_interval %d\n", ntohs(resp_packet->report_interval));
	printf("conf->model.report_interval_keyon %d\n", conf->model.report_interval_keyon);
	if (ntohs(resp_packet->report_interval) != conf->model.report_interval_keyon)
	{
		char str_interval[16];
		conf->model.report_interval_keyon = ntohs(resp_packet->report_interval);
		

		printf("conf->model.report_interval_keyon3 %d\n", conf->model.report_interval_keyon);
		memset(str_interval, 0x00, sizeof(str_interval));	
		sprintf(str_interval,"%d",ntohs(resp_packet->report_interval));
		printf("str_interval3 %s\n", str_interval);

		if(sever_save_interval == 0 )
		{
			printf("sever_save_interval %d\n", sever_save_interval);
			if(save_config_user("user:report_interval_keyon", (char *)str_interval) < 0)
			{
				LOGE(LOG_TARGET, "<%s> save config error #4\n", __FUNCTION__);
				print_yellow("<%s> save config error #4\n", __FUNCTION__);
				return -1;
			}
			if(save_config_user("user:collect_interval_keyon", (char *)str_interval) < 0)
			{
				LOGE(LOG_TARGET, "<%s> save config error #3\n", __FUNCTION__);
				print_yellow("<%s> save config error #3\n", __FUNCTION__);
				return -1;
			}
			if(save_config_user("user:collect_interval_keyoff", (char *)str_interval) < 0)
			{
				LOGE(LOG_TARGET, "<%s> save config error #5\n", __FUNCTION__);
				print_yellow("<%s> save config error #5\n", __FUNCTION__);
				return -1;
			}
			if(save_config_user("user:report_interval_keyoff", (char *)str_interval) < 0)
			{
				LOGE(LOG_TARGET, "<%s> save config error #6\n", __FUNCTION__);
				print_yellow("<%s> save config error #6\n", __FUNCTION__);
				return -1;
			}
			sever_save_interval = ntohs(resp_packet->report_interval);	
		}
		else
		{	
			printf("sever_save_interval %d\n", sever_save_interval);
			sever_save_interval = ntohs(resp_packet->report_interval);	
		}		

		printf("conf->model.report_interval_keyon2 %d\n", conf->model.report_interval_keyon);
	
	} 	

	/* IP */
	memset(p_str_resp_ip, 0x00, sizeof(p_str_resp_ip));
	sprintf(p_str_resp_ip, "%d.%d.%d.%d", resp_packet->ip1, resp_packet->ip2, resp_packet->ip3, resp_packet->ip4);

	//gSetting_report.ip

	printf("p_str_resp_ip : %s\n", p_str_resp_ip);
	if((strcmp(p_str_resp_ip, conf->model.report_ip) != 0) 
	&& resp_packet->ip1 != 0
	&& resp_packet->ip2 != 0
	&& resp_packet->ip3 != 0
	&& resp_packet->ip4 != 0)
	{
		printf("conf->model.report_ip : %s\n", conf->model.report_ip);
		strcpy(conf->model.report_ip, p_str_resp_ip);
		strcpy(g_sever_ip, p_str_resp_ip);
		printf("p_str_resp_ip2 : %s\n", p_str_resp_ip);
		is_sever_modify_ip++;

		if(is_sever_modify_ip < 2)
		{
			if(save_config_user("user:report_ip", (char *)p_str_resp_ip) < 0)
			{
				LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
				return -1;
			}
			//printf("p_str_resp_ip2 : %s\n", p_str_resp_ip);
			conf = load_config_user();
			printf("conf->model.report_ip : %s\n", conf->model.report_ip);
			usleep(100);
			system("sync &");
		}
		else
		{
			printf("is_sever_modify_ip : %d\n", is_sever_modify_ip);
			strcpy(g_sever_ip, p_str_resp_ip);
		}		
	}
	else if (resp_packet->ip1 == 0
	&& resp_packet->ip2 == 0
	&& resp_packet->ip3 == 0
	&& resp_packet->ip4 == 0)
	{
		LOGE(LOG_TARGET, "<%s> resp_packet->ip : %d.%d.%d.%d \n", __FUNCTION__, resp_packet->ip1, resp_packet->ip2, resp_packet->ip3, resp_packet->ip4);
		devel_webdm_send_log("resp_packet->ip : %d.%d.%d.%d \n", resp_packet->ip1, resp_packet->ip2, resp_packet->ip3, resp_packet->ip4);
	}

	/* PORT */
	printf("port: %d\n", ntohs(resp_packet->port));
	n_resp_port = ntohs(resp_packet->port);

	if (n_resp_port != conf->model.report_port && n_resp_port != 0)
	{
		printf(" conf->model.report_port %d\n",  conf->model.report_port);

		memset(p_str_resp_port, 0x00, sizeof(p_str_resp_port));	
		sprintf(p_str_resp_port,"%d",n_resp_port);
		if(save_config_user("user:report_port", (char *)p_str_resp_port) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #2\n", __FUNCTION__);
			return -1;
		}
		conf = load_config_user();
		printf("conf->model.report_port : %s\n", conf->model.report_port);
		
	}
	else if (n_resp_port == 0)
	{
		LOGE(LOG_TARGET, "<%s> n_resp_port : %d \n", n_resp_port);
		devel_webdm_send_log("resp_packet->port : %d\n", n_resp_port);
	}	

	/* WARNNING DEVICE */
	if(resp_packet->warning_num > 0)
	{
		int idx = FLOOD_PACKET_WARRING_IDX;
		int list_count = 0;
		list_count = list_get_num(&leak_data_list);

		//print_yellow("list_count: %d\n", list_count);
		if(list_count > 0)
			list_del_all(&leak_data_list);

		warning_cnt = resp_packet->warning_num;
		
		printf("warning_data->warning_num : %d\n", resp_packet->warning_num);
		LOGI(LOG_TARGET, "warrning list num : %d\n", resp_packet->warning_num);
		devel_webdm_send_log("warrning list: %d\n", resp_packet->warning_num);
		
		for(i = 0; i < warning_cnt; i++)
		{			
			char device_id[MAX_DEVICE_ID];
			char deviceid_len;
			unsigned char * item; 

			item = malloc(sizeof(unsigned char) * MAX_DEVICE_ID);
			if(item == NULL)
			{
				sleep(1);
				printf("item is null \n");
			}
			memset(item, 0x0, sizeof(unsigned char) * MAX_DEVICE_ID);			

			deviceid_len = resp_buff[idx];
			if(deviceid_len > MAX_DEVICE_ID)
				deviceid_len = MAX_DEVICE_ID;
			
			memset(device_id, 0x30, sizeof(char)*MAX_DEVICE_ID);
			strncpy(device_id, &resp_buff[idx + 1], deviceid_len);
			sprintf(item, "%s", device_id);

			printf("device_id  : %s/%d\n", device_id, deviceid_len);
			printf("item  : %s\n", item );
			
			if(list_add(&leak_data_list, item)< 0)
			{
				LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
				print_yellow( "%s : list add fail\n", __FUNCTION__);
			}

			idx = idx + deviceid_len + 1;

		}

	}
	else
	{
		is_list_init = 1;

	}

	return 1;

}

int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res = 0;
	int wait_count;
	char svr_resp[SERVER_RESP_MAX_LEN] = {0,};
	int ret_val = -1;
	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);
	
	while(1) 
	{
		memset(&svr_resp, 0x00, sizeof(svr_resp));

		res = transfer_packet_recv_etxflood(&gSetting_report, packet_buf, packet_len, svr_resp, sizeof(svr_resp), 3);

		if(res == 0) //send and recv success
		{
			LOGI(LOG_TARGET, "%s> send success : get data >> \"%s\"\n", __func__, svr_resp);

			// // jhcho afterawards
			// svr_resp[0] = 0x02;
			// svr_resp[1] = 0x1F;
			// svr_resp[2] = 0x00;
			// svr_resp[3] = 0x00;
			// svr_resp[4] = 0x00;
			// svr_resp[5] = 0x00;
			// svr_resp[6] = 0x1D;
			// svr_resp[7] = 0x00;
			// svr_resp[8] = 0x3C;
			// svr_resp[9] = 0x70;
			// svr_resp[10] = 0xDC;
			// svr_resp[11] = 0x11;
			// svr_resp[12] = 0x22;
			// svr_resp[13] = 0x0F;
			// svr_resp[14] = 0xA1;
			// svr_resp[15] = 0x01;
			
			// svr_resp[16] = 0x00;
			// svr_resp[17] = 0x03;

			// svr_resp[16] = 0x02;
			// svr_resp[17] = 0x0B;
			// svr_resp[18] = 0x41;
			// svr_resp[19] = 0x42;
			// svr_resp[20] = 0x43;
			// svr_resp[21] = 0x44;
			// svr_resp[22] = 0x45;
			// svr_resp[23] = 0x46;
			// svr_resp[24] = 0x47;
			// svr_resp[25] = 0x48;
			// svr_resp[26] = 0x49;
			// svr_resp[27] = 0x4A;
			// svr_resp[28] = 0x4B;

			// svr_resp[29] = 0x0B;
			// svr_resp[30] = 0x4B;
			// svr_resp[31] = 0x4C;
			// svr_resp[32] = 0x4D;
			// svr_resp[33] = 0x4E;
			// svr_resp[34] = 0x45;
			// svr_resp[35] = 0x46;
			// svr_resp[36] = 0x47;
			// svr_resp[37] = 0x48;
			// svr_resp[38] = 0x49;
			// svr_resp[39] = 0x4A;
			// svr_resp[40] = 0x4B;		

			// svr_resp[41] = 0x03;
			
			ret_val = server_resp_proc(svr_resp);
			break;
		}

		LOGE(LOG_TARGET, "%s> Fail to send packet [%d]\n", __func__, op);

		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		
		// if(op == eCYCLE_REPORT_EVC)
		// {
		// 	watchdog_set_cur_ktime(eWdNet1);
		// }
		// else
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

	if(is_sever_modify_ip > 2)
		strncpy(gSetting_report.ip, g_sever_ip, 40);
	else
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

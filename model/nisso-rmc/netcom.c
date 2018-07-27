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

#include "server_resp.h"

#include <nisso_mdt800/packet.h>
#include <nisso_mdt800/gpsmng.h>
#include <nisso_mdt800/hdlc_async.h>
#include <nisso_mdt800/file_mileage.h>

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
	nisso_packet2_t *p_packet2 = NULL;
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

	create_report2_divert_buffer(&p_encbuf, list_count);

	if(ret < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;
	extend_pack_idx = 0;


	while(packet_count < list_count)
	{
		data_length = 0;
		crc = 0;

		//if(conf->model.packet_type == 81)
		{
			if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
				break;
            
            if ( p_packet2 == NULL )
                break;
            
			data_length = sizeof(nisso_packet2_t);

			crc = crc8(crc, (unsigned char *)p_packet2, data_length);
			enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet2, data_length);
			free(p_packet2);
			//enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
			//p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

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

int make_event2_packet(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	gpsData_t last_gpsdata;
	nisso_packet2_t packet;
	int org_size = 0;

	gps_get_curr_data(&gpsdata);
    active_gps_process_force_routine(gpsdata);
    
	if ( gpsdata.active != eACTIVE ) 
	{
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
	}

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report2 divert buffer fail\n", __func__);
		return -1;
	}

	org_size = create_report2_data(eventCode, &packet, gpsdata, NULL, 0);
	LOGI(LOG_TARGET, "%s> report size %d\n", __func__, org_size);

	crc = crc8(crc, (unsigned char *)&packet, org_size);
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, org_size);
	//enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	//p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

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
		default:
			res = make_event2_packet(packet_buf, packet_len, op);
			printf("%s() -> %d\r\n", __func__, __LINE__);
			break;
	}

    write_vaild_data();

	return res;
}

//#define TEST_CODE_TEST_TEST__TEST
int __send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int wait_count;
	char svr_resp[SERVER_RESP_MAX_LEN] = {0,};
    char svr_resp_2[SERVER_RESP_MAX_LEN] = {0,};
	int ret_val = -1;
    int fail_count = 0;

    int conn_api_ret_val = 0;

    static int last_success_server_port = 0;
    static char last_success_server_ip[40] = {0,};

	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);
	
	while(1) 
	{
        watchdog_set_cur_ktime(eWdNet1);
        watchdog_set_cur_ktime(eWdNet2);

        setting_network_param();

		memset(&svr_resp, 0x00, sizeof(svr_resp));
		//res = transfer_packet_recv_etx(&gSetting_report, packet_buf, packet_len, svr_resp, sizeof(svr_resp), ']');
#ifdef TEST_CODE_TEST_TEST__TEST
        static int test_cnt = 0;
        if ( test_cnt++ == 0 )
            strcpy(svr_resp, "[0][&12,2,3,2,37.4836196,126.8798599,150,4,2,37.4836196,126.8798599,70,1][&12,1, 0,2,37.4756163,126.8818914,60 ,1,2,00.0000000,000.0000000,0 ,2,2,00.0000000,000.0000000,0,3,2,37.399693, 127.100937,10,1]");
        else
            strcpy(svr_resp, "[2]");
#else
        conn_api_ret_val = transfer_packet_recv_nisso(&gSetting_report, packet_buf, packet_len, (unsigned char *)&svr_resp, SERVER_RESP_MAX_LEN);
#endif
        mds_api_remove_etc_char(svr_resp, svr_resp_2, sizeof(svr_resp_2));

        if ( strlen(svr_resp_2) )
        {
            if ( strlen(svr_resp_2) > 3 )
            {
                //devel_webdm_send_log("pkt send recv \"%s\"", svr_resp_2);
            }

            LOGI(LOG_TARGET, "%s> send success : get data 2 >> \"%s\"\n", __func__, svr_resp_2);
            ret_val = server_resp_proc(svr_resp_2);

            if ( ret_val < 0)
                devel_webdm_send_log("[%s:%d] op[%d] -> PARSE FAIL \"%s\"", gSetting_report.ip, gSetting_report.port, op, svr_resp_2);
        }
        else
        {
            devel_webdm_send_log("[%s:%d] op[%d] -> RECV FAIL [%d], [%d]  ", gSetting_report.ip, gSetting_report.port, op, fail_count, conn_api_ret_val);
        }

        if (ret_val >= 0)
        {
            /*
            memset(last_success_server_ip, 0x00, sizeof(last_success_server_ip));
            last_success_server_port = gSetting_report.port;
            strncpy(last_success_server_ip, gSetting_report.ip, 40);
            LOGE(LOG_TARGET, "%s> recovery ip [%s][%d]\n", __func__, last_success_server_ip, last_success_server_port);
            */

            if (fail_count > 0)
                devel_webdm_send_log("[%s:%d] op[%d] -> RECV SUCCESS CASE 1 [%d], [%d]  ", gSetting_report.ip, gSetting_report.port, op, fail_count, conn_api_ret_val);

            break;
        }

        LOGE(LOG_TARGET, "%s> Fail to send packet [%d]\n", __func__, op);

        //wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
        //while(wait_count-- > 0) {
        //	LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
#if 0
        if ( fail_count > 5 )
        {
            if ( last_success_server_port > 0 )
            {
                /*
                LOGE(LOG_TARGET, "%s> recovery ip [%s][%d]\n", __func__, last_success_server_ip, last_success_server_port);
                devel_webdm_send_log("recovery ip [%s][%d]", __func__, last_success_server_ip, last_success_server_port);

                configurationModel_t * conf = get_config_model();
                strncpy(conf->model.report_ip, last_success_server_ip, sizeof(conf->model.report_ip));
                conf->model.report_port = last_success_server_port;
                save_config_user("user:report_ip", last_success_server_ip);
                save_config_user("user:report_port", last_success_server_port);
                */
            }
        }
#endif

        fail_count ++;
        sleep(WAIT_INTERVAL_TIME);
        if ( fail_count > PACKET_RETRY_COUNT )
            break;
        //}

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

#define MAX_SEND_RETRY_CNT_NISSO	10
int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int ret;
	int max_send_try_cnt = MAX_SEND_RETRY_CNT_NISSO;

//	setting_network_param(); //real-time ip/port change

	LOGI(LOG_TARGET, "ip %s : %d send packet!!\n", gSetting_report.ip, gSetting_report.port);

	while(max_send_try_cnt --)
	{
		ret = __send_packet(op, packet_buf, packet_len);
		if ( ret >= 0 )
			break;
        devel_webdm_send_log("send pkt RETRY CASE 0");
	}

	if(ret < 0) {
		LOGE(LOG_TARGET, "op[%d] __send_packet error return\n", op);
        devel_webdm_send_log("send pkt FAIL CASE 0");
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
	gSetting_report.retry_count_connect = 1;
	gSetting_report.retry_count_send = 1;
	gSetting_report.retry_count_receive = 7;
	gSetting_report.timeout_secs = 30;
// ����
/*
	configurationModel_t *conf = get_config_model();
	strncpy(gSetting_report.ip, conf->model.report_ip, 40);
	gSetting_report.port = conf->model.report_port;
	gSetting_report.retry_count_connect = 3;
	gSetting_report.retry_count_send = 3;
	gSetting_report.retry_count_receive = 3;
	gSetting_report.timeout_secs = 30;
*/
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

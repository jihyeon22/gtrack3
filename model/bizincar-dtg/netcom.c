#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <base/config.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
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
#include <base/sender.h>
#include "thread-keypad.h"
#include <disc_dtg_pkt.h>

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/hdlc_async.h>
#include <mdt800/file_mileage.h>


#include "dtg_pkt_senario.h"
#include "mdt_pkt_senario.h"
#include "dvr_pkt_senario.h"
//#define SEND_MIRRORED_PACKET 1

transferSetting_t gSetting_mdt_report;
transferSetting_t gSetting_dtg_report;
transferSetting_t gSetting_dvr_report;

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

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{

	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);
	    
	switch(op)
	{
		case eCYCLE_REPORT_EVC:
			res = bizincar_mdt__make_period_pkt(packet_buf, packet_len);
			break;
        case eDTG_CUSTOM_EVT__DTG_KEY_ON:
            res = bizincar_dtg__make_evt_pkt(packet_buf, packet_len, eKeyOn_Event);
            break;
        case eDTG_CUSTOM_EVT__DTG_KEY_OFF:
            res = bizincar_dtg__make_evt_pkt(packet_buf, packet_len, eKeyOff_Event);
            break;
        case eDTG_CUSTOM_EVT__DTG_POWER_ON:
            res = -1;
            break;
        case eDTG_CUSTOM_EVT__DTG_POWER_OFF:
            res = bizincar_dtg__make_evt_pkt(packet_buf, packet_len, ePower_Event);
            break;
        case eDTG_CUSTOM_EVT__DTG_REPORT:
            res = bizincar_dtg__make_period_pkt(packet_buf, packet_len);
            break;
        case eDVR_CUSTOM_EVT__DVR_REPORT:
        {
            bizincar_dvr__dev_data_t* dvr_data = (bizincar_dvr__dev_data_t *) param;
            res = bizincar_dvr__make_evt_pkt(packet_buf, packet_len, dvr_data->buff, dvr_data->buff_len );
            break;
        }
		default:
			if(op > 0 && op < eMAX_EVENT_CODE) {
				res = bizincar_mdt__make_event_pkt(packet_buf, packet_len, op);
			}
			break;
	}

	return res;
}

int mdt__send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int retry = PACKET_RETRY_COUNT;
	int wait_count;

    bizincar_mdt_response_t resp = {0,};
    
	printf("\n\n send_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);

	while(retry-- > 0) 
	{
		res = transfer_packet_recv(&gSetting_mdt_report, packet_buf, packet_len, &resp, sizeof(bizincar_mdt_response_t));
		if(res == 0) //send and recv success
        {
            res = bizincar_mdt__parse_resp(&resp);
            LOGI(LOG_TARGET, "mdt send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            LOGI(LOG_TARGET, "mdt send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            LOGI(LOG_TARGET, "mdt send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
			break;
        }

		sleep(WAIT_INTERVAL_TIME);
	}

#if SEND_MIRRORED_PACKET
	int retry_mirrored = PACKET_RETRY_COUNT;
	while(retry_mirrored-- > 0) 
	{
		res = transfer_packet(&gSetting_report_mds_mdt800, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(5);
	}
#endif

	if(retry <= 0)
	{
		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		LOGI(LOG_TARGET, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		return -1; //
	}

	return res;
}



int dtg__send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int retry = PACKET_RETRY_COUNT;
	int wait_count;

    bizincar_dtg_respose_t resp = {0,};

    printf(" ----------------------------------------- dtg send pkt \r\n");

	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);

	while(retry-- > 0) 
	{
		res = transfer_packet_recv(&gSetting_dtg_report, packet_buf, packet_len, &resp, sizeof(bizincar_dtg_respose_t));
		if(res == 0) //send and recv success
        {
            //rintf("dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            res = bizincar_dtg__parse_pkt(&resp);
            LOGI(LOG_TARGET, "dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            LOGI(LOG_TARGET, "dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            LOGI(LOG_TARGET, "dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);

			break;
        }

		sleep(WAIT_INTERVAL_TIME);
	}

#if SEND_MIRRORED_PACKET
	int retry_mirrored = PACKET_RETRY_COUNT;
	while(retry_mirrored-- > 0) 
	{
		res = transfer_packet(&gSetting_report_mds_mdt800, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(5);
	}
#endif

	if(retry <= 0)
	{
		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		LOGI(LOG_TARGET, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		return -1; //
	}

	return res;
}


int dvr__send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int retry = PACKET_RETRY_COUNT;
	int wait_count;

    bizincar_dvr__server_resp_t resp = {0,};

    printf(" ----------------------------------------- dvr send pkt \r\n");

	printf("\n\nsend_packet : op[%d]============>\n", op);
	hdlc_async_print_data(packet_buf, packet_len);

	while(retry-- > 0) 
	{
		res = transfer_packet_recv(&gSetting_dvr_report, packet_buf, packet_len, &resp, sizeof(bizincar_dtg_respose_t));
		if(res == 0) //send and recv success
        {
            //printf("dvr send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            res = bizincar_dvr__parse_resp(&resp);
            //printf("dvr send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
			break;
        }

		sleep(WAIT_INTERVAL_TIME);
	}

#if SEND_MIRRORED_PACKET
	int retry_mirrored = PACKET_RETRY_COUNT;
	while(retry_mirrored-- > 0) 
	{
		res = transfer_packet(&gSetting_report_mds_mdt800, packet_buf, packet_len);
		if(res == 0) //send and recv success
			break;

		sleep(5);
	}
#endif

	if(retry <= 0)
	{
		wait_count = MAX_WAIT_RETRY_TIME/WAIT_INTERVAL_TIME;
		while(wait_count-- > 0) {
			LOGI(LOG_TARGET, "%s> wait time count [%d]\n", __func__, wait_count);
			sleep(WAIT_INTERVAL_TIME);
		}
		LOGI(LOG_TARGET, "%s> keep packet, but to move end of buffer. %d\n", __func__, wait_count);
		return -1; //
	}

	return res;
}


#define MAX_RETRY_CNT   5

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int ret;
    int retry_cnt = MAX_RETRY_CNT;

	setting_network_param(); //real-time ip/port change

    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    printf(" call send pkt [%d]\r\n", op);
    
    while(retry_cnt--)
    {
        switch(op)
        {
            case eDTG_CUSTOM_EVT__DTG_KEY_ON:
            case eDTG_CUSTOM_EVT__DTG_KEY_OFF:
            case eDTG_CUSTOM_EVT__DTG_POWER_ON:
            case eDTG_CUSTOM_EVT__DTG_POWER_OFF:
            case eDTG_CUSTOM_EVT__DTG_REPORT:
                ret = dtg__send_packet(op, packet_buf, packet_len);
                break;
            case eDVR_CUSTOM_EVT__DVR_REPORT:
                ret = dvr__send_packet(op, packet_buf, packet_len);
                break;
            default :
                LOGI(LOG_TARGET, "ip %s : %d send packet!!\n", gSetting_mdt_report.ip, gSetting_mdt_report.port);
                ret = mdt__send_packet(op, packet_buf, packet_len);
                break;
        }

        if(ret < 0) 
        {
            LOGE(LOG_TARGET, "op[%d] mdt__send_packet error return [%d]\n", op, retry_cnt);
            sleep(3);
        }
        else
        {
            LOGI(LOG_TARGET, "op[%d] mdt__send_packet success return [%d]\n", op, retry_cnt);
            break;
        }
    }

	keypad_server_result__set_result(op, KEY_RESULT_TRUE);
	LOGI(LOG_TARGET, "send_packet op[%d] send success\n", op);

	return 0;
}


int setting_network_param(void)
{
	configurationModel_t *conf = get_config_model();
	strncpy(gSetting_mdt_report.ip, conf->model.report_ip, 40);
	gSetting_mdt_report.port = conf->model.report_port;
	gSetting_mdt_report.retry_count_connect = 3;
	gSetting_mdt_report.retry_count_send = 3;
	gSetting_mdt_report.retry_count_receive = 3;
	gSetting_mdt_report.timeout_secs = 30;

	strncpy(gSetting_dtg_report.ip, conf->model.dtg_report_ip, 40);
	gSetting_dtg_report.port = conf->model.dtg_report_port;
	gSetting_dtg_report.retry_count_connect = 3;
	gSetting_dtg_report.retry_count_send = 3;
	gSetting_dtg_report.retry_count_receive = 3;
	gSetting_dtg_report.timeout_secs = 30;

	strncpy(gSetting_dvr_report.ip, conf->model.dvr_report_ip, 40);
	gSetting_dvr_report.port = conf->model.dvr_report_port;
	gSetting_dvr_report.retry_count_connect = 3;
	gSetting_dvr_report.retry_count_send = 3;
	gSetting_dvr_report.retry_count_receive = 3;
	gSetting_dvr_report.timeout_secs = 30;
    
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

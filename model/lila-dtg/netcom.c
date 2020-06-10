#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <base/watchdog.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <logd_rpc.h>
#include "netcom.h"
#include <base/sender.h>

#include <callback.h>
#include "lila_transfer.h"
#include "debug.h"
#include "netcom.h"
#include "config.h"

#define MAX_WAIT_RETRY_TIME		180	//unit : sec
#define WAIT_INTERVAL_TIME		5	//unit : sec
#define PACKET_RETRY_COUNT		10
#define MAX_RETRY_CNT   5

transferSetting_t gSetting_report;
transferSetting_t gSetting_mdt_report;
transferSetting_t gSetting_dtg_report;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = -1;
	LOGI(LOG_TARGET, "%s> op[%d]\n", __func__, op);

	switch(op)
	{
		case ePKT_TRANSFER_ID__DTG_INFO:
			res = lila_dtg__make_dtg_header(packet_buf, packet_len);
			break;
		case ePKT_TRANSFER_ID__DTG_DATA:
			res = lila_dtg__make_dtg_data_dummy(packet_buf, packet_len);
			break;
		default:
			break;
	}

	return res;
	return 0;
}


int dtg__send_packet_info(char op, unsigned char *packet_buf, int packet_len)
{
	int res;
	int retry = PACKET_RETRY_COUNT;
//	int wait_count;
	
    LILA_PKT_RESP__DTST_T resp;
	memset(&resp, 0x00, sizeof(resp));

    printf(" ----------------------------------------- dtg info pkt \r\n");

	printf("\n\nsend_packet : op[%d]============>\n", op);

	while(retry-- > 0) 
	{

		res = transfer_packet_recv(&gSetting_dtg_report, packet_buf, packet_len, &resp, sizeof(resp));

        watchdog_set_cur_ktime(eWdNet1);
        watchdog_set_cur_ktime(eWdNet2);

		if(res == 0) //send and recv success
        {
            //rintf("dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
            res = lila_dtg__parse_dtg_header(&resp);

			break;
        }
		sleep(WAIT_INTERVAL_TIME);

	}

#ifdef DTG_PKT_FAIL_WRITE_TO_FILE
    if ( resp.packet_ret_code != 1 )
    {
        char log_msgbuff[1024] = {0,};
        sprintf(log_msgbuff, "server get ret fail [%d] start ++ ",  resp.packet_ret_code);
        mds_api_write_time_and_log(DEBUG_DTG_PKT_LOG, log_msgbuff);
        mds_api_debug_hexdump_to_log(packet_buf,packet_len);
    }
#endif

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

	return res;
}

int _dtg__send_packet_data(unsigned char *packet_buf, unsigned short packet_len)
{
	int retry = PACKET_RETRY_COUNT;
	int res = 0;

    LILA_PKT_RESP__DTST_T resp;
	memset(&resp, 0x00, sizeof(resp));

	while(retry-- > 0) 
	{
		res = transfer_packet_recv_lila(&gSetting_dtg_report, packet_buf, packet_len, &resp, sizeof(resp));

		watchdog_set_cur_ktime(eWdNet1);
		watchdog_set_cur_ktime(eWdNet2);

		if(res == 0) //send and recv success
		{
			//rintf("dtg send success?@!?!?!?!?!?!?! [%d] [%d]\r\n", resp.packet_ret_code, res);
			res = lila_dtg__parse_dtg_header(&resp);

			break;
		}
		else
			transfer_close_socket();
		sleep(WAIT_INTERVAL_TIME);
	}
	return res;
}

int dtg__send_packet_data(char op, unsigned char *packet_buf, int packet_len)
{
	int res = 0;

	unsigned char *tmp_packet_buf = NULL;
	unsigned short tmp_packet_len = 0;

	while( lila_dtg__make_dtg_data(&tmp_packet_buf, &tmp_packet_len) > 0 )
	{
		_dtg__send_packet_data(tmp_packet_buf, tmp_packet_len);
		if ( tmp_packet_buf != NULL)
			free(tmp_packet_buf);
	}

	transfer_close_socket(); // close hsocket
	return res;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	int ret;
//    int retry_cnt = MAX_RETRY_CNT;

	setting_network_param(); //real-time ip/port change
	switch(op)
	{
		case ePKT_TRANSFER_ID__DTG_INFO:
			ret = dtg__send_packet_info(op, packet_buf, packet_len);
			break;
		case ePKT_TRANSFER_ID__DTG_DATA:
			ret = dtg__send_packet_data(op, packet_buf, packet_len);
			break;
		default :
			break;

	}

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
    
	return 0;
}
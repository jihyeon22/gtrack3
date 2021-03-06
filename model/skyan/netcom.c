#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <base/watchdog.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <logd_rpc.h>


#include <callback.h>
#include "netcom.h"
#include "config.h"

#include "skyan_tools.h"
#include "skyan_senario.h"
#include "skyan_resp.h"
#include "packet.h"

transferSetting_t gSetting_report;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
    switch(op)
    {
        case e_skyan_pkt__evt:
        case e_skyan_pkt__geofence_evt:
        case e_skyan_pkt__normal_period:
        case e_skyan_pkt__req_geofence_info:
        {
            create_sky_autonet_report_pkt((SKY_AUTONET__PKT_ARG_T *)param, packet_buf, packet_len);
            break;
        }
        
        default:
            LOGE(eSVC_MODEL, "[SKYAN NETCOMM] mk pkt [0x%x] - No suport PKT TYPE\r\n", op);
            return -1;
    }
    /*
        e_skyan_pkt__evt,
    e_skyan_pkt__normal_period,
    */
	return 0;
}


int _send_packet(char op, unsigned char *packet_buf, int packet_len)
{
    int ret = -1;
    transferSetting_t network_setting_info = {
        0,
    };

    network_setting_info.retry_count_connect = 2;
    network_setting_info.retry_count_send = 2;
    network_setting_info.retry_count_receive = 0;
    network_setting_info.timeout_secs = 1;
    
    get_user_cfg_report_ip(network_setting_info.ip);
    get_user_cfg_report_port(&network_setting_info.port);

    switch(op)
    {
        case e_skyan_pkt__evt:
        case e_skyan_pkt__geofence_evt:
        case e_skyan_pkt__normal_period:
        case e_skyan_pkt__req_geofence_info:
        {
            int recv_ret = -1;
            int parse_ret = -1;
            int recv_buff_len = 0;

            char recv_buff[2048] = {0,};
            char recv_buff2[2048] = {0,};
            
            recv_buff_len = sizeof(recv_buff);
            recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);

            // etc char remove.. : only string data..
            mds_api_remove_etc_char(recv_buff, recv_buff2, sizeof(recv_buff2));

            if ( strlen(recv_buff2) > 10)
                ret = skyan_resp__parse(recv_buff);

            break;
        }
    }

    /*
        e_skyan_pkt__evt,
    e_skyan_pkt__normal_period,
    */
	return ret;
}


int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
    int ret = -1;
    int retry_cnt = MAX_SEND_RETRY_CNT;

    while(retry_cnt--)
    {
        ret = _send_packet(op, packet_buf, packet_len);
        if ( ret == SKYAN_JSON_PARSE_SUCCESS )
        {
            LOGI(eSVC_MODEL, "[SKYAN PKT] SEND PKT SUCCESS : op [%d]\r\n", op);
            break;
        }
        watchdog_set_cur_ktime(eWdNet1);
        watchdog_set_cur_ktime(eWdNet2);
        LOGE(eSVC_MODEL, "[SKYAN PKT] SEND PKT FAIL : op [%d]\r\n", op);
        sleep(5);
    }

    return 0;
}
int free_packet(void *packet)
{
    gps_valid_data_write();
    mileage_write();

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
	gSetting_report.retry_count_connect = 3;
	gSetting_report.retry_count_send = 3;
	gSetting_report.retry_count_receive = 3;
	gSetting_report.timeout_secs = 30;

	return 0;
}
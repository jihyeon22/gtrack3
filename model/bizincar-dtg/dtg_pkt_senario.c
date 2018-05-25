#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/thermtool.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <board/modem-time.h>
#include <board/power.h>
#include <board/battery.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/nettool.h>
#include <util/stackdump.h>
#include "logd/logd_rpc.h"

#include "callback.h"
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"
#include "thread-keypad.h"

#include <board/board_system.h>

#include "dtg_pkt_senario.h"
#include "dtg_gtrack_tool.h"
#include "disc_dtg_pkt.h"
#include "tacom_choyoung_protocol.h"

static int _char_mbtoi(char *srcptr, int size)
{
	char tmp_buf[128];
	int result;
	
	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, srcptr, size);
	result = atoi(tmp_buf);
	return result;
}

static double _char_mbtod(char *srcptr, int size)
{
	char tmp_buf[128];
	int result;

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, srcptr, size);
	result = strtod(tmp_buf, NULL);
	return result;
}


static float _char_mbtof(char *srcptr, int size)
{
	char tmp_buf[128];
	float result;

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, srcptr, size);
	result = atof(tmp_buf);
	return result;
}


int bizincar_dtg_init()
{
    char* tmp_buff;
    int taco_year = 0;
    int max_init_wait = 60;

    int ret_bufflen = 0;

    while(max_init_wait --)
    {
        cy_send_cmd_force_send(CY_DTG_CMD__DTG_DEV_INFO);

        ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_buff);

        if ( ret_bufflen <= 0 )
        {
            LOGI(LOG_TARGET, "bizincar_dtg_init is fail... 1 retry...\r\n");
            sleep(1);
            continue;
        }

        if (set_current_dtg_data_2(tmp_buff, ret_bufflen) > 0)
        {
            LOGI(LOG_TARGET, "bizincar_dtg_init is fail... 2 retry...\r\n");
            free(tmp_buff);
            break;
        }
        
        free(tmp_buff);

        sleep(1);
    }
    
    if ( max_init_wait < 0 )
        devel_webdm_send_log("DTG INIT CONN FAIL");

    return 0;
}


// -------------------------------------------------------------------------------
// call every 1sec
// -------------------------------------------------------------------------------
int bizincar_dtg_pkt_sernaio()
{
	int report_interval = 180; //3 min
	int collect_interval = 10;

    int condition_send = 0;

    static int key_on_pkt = 0;
    static int key_off_pkt = 0;

    report_interval = get_dtg_report_period();
    collect_interval = get_dtg_create_period();
    
    condition_send = report_interval/collect_interval;

    if(condition_send <= 0)
        condition_send = 1;

    // printf("condition_send is [%d]\r\n", condition_send);
    
    LOGI(LOG_TARGET, "[DTG PKT KEY] on [%d] / off [%d]\r\n", key_on_pkt, key_off_pkt);

    if (( key_off_pkt > 1 ) && (get_fake_ignition_key_stat() == 0) && (taco_gtrack_tool__get_remain_cnt() > 0))
    {
        // KEY OFF : 
		//  - clear dtg memory buffer 
		//  - no send pkt
        bizincar_dtg__clr_dtg_taco_buff();
        return 0;
    }

    if ( taco_gtrack_tool__get_remain_cnt() >= (condition_send) )
    {
        if ( get_fake_ignition_key_stat() == 1  )
        {
            key_on_pkt ++;
            key_off_pkt = 0;
        }
        else
        {
            key_on_pkt = 0;
            key_off_pkt++;
        }

        
        sender_add_data_to_buffer(eDTG_CUSTOM_EVT__DTG_REPORT, NULL, ePIPE_1);
        
    }

    return 0;
}

int bizincar_dtg__clr_dtg_taco_buff()
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_cur_stream_buff = NULL;
    char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int ret_bufflen = 0;
    
    LOGI(LOG_TARGET, "[DTG PKT KEY] CLEAR DTG TACO BUFF\r\n");

    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(10, &tmp_bluk_stream_buff)) > 0 )
    {
        // first current data set
        if (( ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_cur_stream_buff) ) > 0 )
        {
            set_current_dtg_data((unsigned char*)tmp_cur_stream_buff, ret_bufflen);
            free(tmp_cur_stream_buff);
        }

        free(tmp_bluk_stream_buff);
    }
    
    return 0;
}

// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int bizincar_dtg__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len)
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_cur_stream_buff = NULL;
    char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int disc_pkt_size = 0;
    int ret_bufflen = 0;
    
    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(10, &tmp_bluk_stream_buff)) > 0 )
    {
        // first current data set
        if (( ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_cur_stream_buff) ) > 0 )
        {
            set_current_dtg_data((unsigned char*)tmp_cur_stream_buff, ret_bufflen);
            free(tmp_cur_stream_buff);
        }

        disc_pkt_size = dtg_dsic__make_bulk_pkt(tmp_bluk_stream_buff, taco_data_size, &tmp_mk_pkt_buff);

        *pbuf = (unsigned char *)tmp_mk_pkt_buff;
        *packet_len = disc_pkt_size;

        free(tmp_bluk_stream_buff);

    }
    
    return 0;
}


int bizincar_dtg__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, int evt)
{
//    char * tmp_buff = NULL;
    char * tmp_buff2 = NULL;

//    int taco_data_size = 0;
    int disc_pkt_size = 0;
//    int ret_bufflen = 0;
    
    disc_pkt_size = dtg_dsic__make_evt_pkt(NULL, 0, &tmp_buff2, evt);
    
    *pbuf = (unsigned char *)tmp_buff2;
    *packet_len = disc_pkt_size;
    
    return 0;
}


int bizincar_dtg__parse_pkt(bizincar_dtg_respose_t* resp)
{
    int resp_code = -1;
    int ret_val = -1;

    resp_code = resp->packet_ret_code;

    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    
    switch (resp_code)
    {
        case 0 : 
            ret_val = -1;
            break;
        case 1 : // success
            ret_val = 0;
            break;
        case 2 : // success
            ret_val = 0;
            cy_send_cmd_force_send(CY_DTG_CMD__LOC_LOOKUP);
            break;
        default : 
            ret_val = -1;
            break;
    }   

    return ret_val;
}


int bizincar_dtg__vehicle_speed()
{
    tacom_std_data_t std_taco_data = {0,};
    int vehicle_speed = 0;
    taco_gtrack_tool__get_cur_std_data(&std_taco_data);
    vehicle_speed = _char_mbtoi(std_taco_data.speed, 3);
    printf(" bizincar_dtg__vehicle_speed() => [%d]\r\n", vehicle_speed);
    LOGI(LOG_TARGET, "%s > dtg speed [%d]\n", __func__, vehicle_speed);
    return vehicle_speed;
}



int bizincar_dtg__vehicle_odo()
{
    tacom_std_data_t std_taco_data = {0,};
    int vehicle_odo = 0;
    taco_gtrack_tool__get_cur_std_data(&std_taco_data);
    vehicle_odo = _char_mbtoi(std_taco_data.cumulative_run_distance, 7);
    printf(" bizincar_dtg__vehicle_odo() => [%d]\r\n", vehicle_odo);
    LOGI(LOG_TARGET, "%s > dtg odo [%d]\n", __func__, vehicle_odo);
    return vehicle_odo;
}


int bizincar_dtg__vehicle_odo_diff_mdt()
{
    static int last_odo = -1;
    int current_odo = bizincar_dtg__vehicle_odo();
    
    int vehicle_odo = 0;

    if ( last_odo == -1 )
        last_odo = current_odo;
    
    if ( current_odo > last_odo )
    {
        vehicle_odo = current_odo - last_odo;
        last_odo = current_odo;
    }

    printf(" bizincar_dtg__vehicle_odo_diff_mdt() => [%d]\r\n", vehicle_odo);
    LOGI(LOG_TARGET, "%s > dtg odo [%d]\n", __func__, vehicle_odo);

    return vehicle_odo;
}


int bizincar_dtg__vehicle_odo_diff_dvr()
{
    static int last_odo = -1;
    int current_odo = bizincar_dtg__vehicle_odo();
    
    int vehicle_odo = 0;

    if ( last_odo == -1 )
        last_odo = current_odo;

    if ( current_odo > last_odo )
    {
        vehicle_odo = current_odo - last_odo;
        last_odo = current_odo;
    }

    printf(" bizincar_dtg__vehicle_odo_diff_dvr() => [%d]\r\n", vehicle_odo);
    LOGI(LOG_TARGET, "%s > dtg odo [%d]\n", __func__, vehicle_odo);

    return vehicle_odo;
}


int bizincar_dtg__key_stat()
{
    tacom_std_data_t std_taco_data = {0,};
    int key_stat = 0;
    taco_gtrack_tool__get_cur_std_data(&std_taco_data);
    key_stat = std_taco_data.key_stat;;
    // printf(" bizincar_dtg__key_stat() => [%d]\r\n", key_stat);
    LOGI(LOG_TARGET, "%s > dtg key_stat [%d]\n", __func__, key_stat);
    return key_stat;
}


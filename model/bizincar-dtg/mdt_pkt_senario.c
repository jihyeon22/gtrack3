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
#include <base/dmmgr.h>

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


#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/gps_utill.h>
#include <mdt800/geofence.h>
#include <mdt800/file_mileage.h>

#include <tacom/tacom_std_protocol.h>
#include "mdt_pkt_senario.h"

extern int is_run_ignition_off;

// ------------------------------------------------------------------------
// gps call back ::: fake...
// ------------------------------------------------------------------------
void gps_parse_one_context_callback_fake(void)
{
	gps_condition_t ret = eUNKNOWN_GPS_DATA;
	gpsData_t cur_gpsdata;
	gpsData_t gpsdata;
    gpsData_t last_gpsdata;
    
	lotte_packet_t *p_packet;
	time_t modem_time; //jwrho 2016.01.04 (gsp time jump up patch)

    static int gps_fail_cnt = 0;

	////////////////////////////////////////////////////////////////////////
	// 1. gps packet gathering and filtering
	////////////////////////////////////////////////////////////////////////
	gps_get_curr_data(&cur_gpsdata);
	
    if ( is_run_ignition_off == 1 )
        gps_fail_cnt = 0;
    
	switch(cur_gpsdata.active)
	{
		case eINACTIVE:
            
            gps_valid_data_get(&last_gpsdata);

            cur_gpsdata.lat = last_gpsdata.lat;
		    cur_gpsdata.lon = last_gpsdata.lon;

			ret = inactive_gps_process(cur_gpsdata, &gpsdata);
            // gps deactivate 10 min ==> cold reset..
            /*
            if ( gps_fail_cnt++ > 600 )
            {
                devel_webdm_send_log("gps active fail.. cold boot");
                gps_reset_immediately(GPS_BOOT_COLD);
                gps_fail_cnt = 0;
            }
            */
			break;
		case eACTIVE:
            gps_fail_cnt = 0;
			modem_time = get_modem_time_utc_sec();


			ret = active_gps_process(cur_gpsdata, &gpsdata);
			break;
	}

	if(ret != eUSE_GPS_DATA) {
		print_skip_gps_reason(ret, cur_gpsdata);
		return;
	}

	LOGD(LOG_TARGET, "cur_gpsdata.active = [%d], this gps condition[%d]\n", cur_gpsdata.active, ret);
	
	LOGD(LOG_TARGET, "gps local time and date: %s\n", asctime(localtime(&gpsdata.utc_sec)) );
	static int show_mileage = 0;
	if(show_mileage++ >= 5)
	{
		show_mileage = 0;
		LOGI(LOG_TARGET, "\n-----------------------------------------\nserver_mileage[%d], gps_mileage = [%d]\n-----------------------------------------", get_server_mileage(), get_gps_mileage());
	}
	

	////////////////////////////////////////////////////////////////////////
	// 2. create packet and add packet to buffer
	////////////////////////////////////////////////////////////////////////
	p_packet = (lotte_packet_t *)malloc(sizeof(lotte_packet_t));
	if(p_packet == NULL) {
		LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
		return;
	}

    if ( bizincar_dtg__key_stat() == 1)
    {
        printf("make key on report pkt\r\n");
	    create_report_data(eCYCLE_REPORT_EVC, p_packet, gpsdata);
    }
    else
    {
        printf("make key off report pkt\r\n");
        create_report_data(eIGN_OFF_EVT, p_packet, gpsdata);
    }

    // init odo and fill dtg data..
    p_packet->vehicle_odo = bizincar_dtg__vehicle_odo_diff_mdt();

	if(list_add(&gps_buffer_list, p_packet) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
		free(p_packet);
	}

	LOGI(LOG_TARGET, "current report packet buffering count[%d]\n", get_gps_data_count());
	return;
}

void fake_ignition_on_callback_mdt(void)
{
    // ------------------------------------------------------
    // mdt pkt
    // ------------------------------------------------------
    dmmgr_send(eEVENT_KEY_ON, NULL, 0); 
	sender_add_data_to_buffer(eIGN_ON_EVT, NULL, ePIPE_2);
}


void fake_ignition_off_callback_mdt(void) // do not use
{
    // ------------------------------------------------------
    // mdt pkt
    // ------------------------------------------------------
    /*
    sender_add_data_to_buffer(eIGN_OFF_EVT, NULL, ePIPE_2);
	sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
	save_mileage_file(get_server_mileage() + get_gps_mileage());
    */
    dmmgr_send(eEVENT_KEY_OFF, NULL, 0); 
}

// ----------------------------------------------------------------
// bizincar mdt main senario :: call 1sec
// ----------------------------------------------------------------
int bizincar_mdt_pkt_sernaio()
{
	int report_interval = 180; //3 min
	int collect_interval = 10;
    
	int condition_send = 0;
    static int msg_print_cnt = 0;

    static int last_key_stat = -1;
    int cur_key_stat = 0;

    gps_parse_one_context_callback_fake();

    // -----------------------------------------
    // fake key sernaio......
    // -----------------------------------------
    cur_key_stat = bizincar_dtg__key_stat();

    if (( last_key_stat != cur_key_stat ) && ( cur_key_stat == 1 )) // key on sernaio.
    {
        devel_webdm_send_log("FAKE KEY ON");
        LOGI(LOG_TARGET, "FAKE KEY ON SENARIO\r\n");
        fake_ignition_on_callback_mdt();
    }
    else if ( ( last_key_stat != cur_key_stat ) && ( cur_key_stat == 0 )) // key off sernaio.
    {
        devel_webdm_send_log("FAKE KEY OFF");
        LOGI(LOG_TARGET, "FAKE KEY OFF SENARIO\r\n");
        fake_ignition_off_callback_mdt();
        // do nothing..
    }

    last_key_stat = cur_key_stat;

    // ---------------------------------------------
    // mdt report data sernaio
    // ---------------------------------------------
    report_interval = get_report_interval();
    collect_interval = get_collection_interval();
    if(msg_print_cnt++ > 5) {
        LOGI(LOG_TARGET, "report_interval[%d]/collect_interval[%d]\n", report_interval, collect_interval);
        msg_print_cnt = 0;
    }

    condition_send = report_interval/collect_interval;
    if(condition_send <= 0)
    {
        condition_send = 1;
    }				

    if(get_gps_data_count() >= (condition_send))
    {
#if(FEATURE_DONT_USE_MERGED_PACKET)
        int n_try = condition_send;

        while(get_gps_data_count() >= (condition_send) && n_try-->0)
#endif
        {
            sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
        }
    }

}


// ----------------------------------------------------------------
// mdt pkt tool
// ----------------------------------------------------------------

int bizincar_mdt__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len)
{
	unsigned short crc = 0;
	int enclen = 0;
	int packet_count = 0;
	unsigned char *p_encbuf;
	lotte_packet_t *p_packet;
	int list_count = 0;

	list_count = list_get_num(&gps_buffer_list);
	if(list_count <= 0)
	{
		LOGE(LOG_TARGET, "%s> list count error %d\n", __func__, list_count);
		return -1;
	}

	if(list_count > LIMIT_TRANSFER_PACKET_COUNT)
		list_count = LIMIT_TRANSFER_PACKET_COUNT;

	if(create_report_divert_buffer(&p_encbuf, list_count) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	enclen = 0;
	packet_count = 0;
	
	while(packet_count < list_count)
	{
		crc = 0;
		if(list_pop(&gps_buffer_list, (void *)&p_packet) < 0)
			break;

		crc = crc8(crc, (unsigned char *)p_packet, sizeof(lotte_packet_t));
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)p_packet, sizeof(lotte_packet_t));
		free(p_packet);
		enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
		p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

		packet_count += 1;
		//if(is_available_report_divert_buffer(packet_count++) == 0) {
		//	LOGT(LOG_TARGET, "report divert buffer full...\n");
		//	break;
		//}
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


int bizincar_mdt__make_event_pkt(unsigned char **pbuf, unsigned short *packet_len, int eventCode)
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
	}
	
    if ( gpsdata.year < 2016)
    {
//        configurationBase_t *conf = get_config_base();
        struct tm loc_time;
//        gpsdata.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
//       _gps_utc_sec_localtime(gpsdata.utc_sec, &loc_time, conf->gps.gps_time_zone);
        gpsdata.year = loc_time.tm_year + 1900;
        gpsdata.mon = loc_time.tm_mon + 1;
        gpsdata.day = loc_time.tm_mday;
        gpsdata.hour = loc_time.tm_hour;
        gpsdata.min = loc_time.tm_min;
        gpsdata.sec = loc_time.tm_sec;
    }

	if(create_report_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	create_report_data(eventCode, &packet, gpsdata);

     // init odo and fill dtg data..
    packet.vehicle_odo = bizincar_dtg__vehicle_odo_diff_mdt();

    //print_report_data(packet);

	crc = crc8(crc, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, sizeof(lotte_packet_t));
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}



int bizincar_mdt__parse_resp(bizincar_mdt_response_t* resp)
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
        default : 
            ret_val = -1;
            break;
    }   

    return ret_val;
}
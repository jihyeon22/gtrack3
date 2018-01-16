#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include "logd/logd_rpc.h"

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "katech-tools.h"
#include "katech-data-calc.h"
#include "katech-packet.h"
#include "seco_obd_1.h"
#include "seco_obd_mgr.h"



static int flag_run_thread_main = 1;
#define LOG_TARGET eSVC_MODEL

#define TRIP_NONE			0
#define TRIP_START			1
#define TRIP_CALC			2
#define TRIP_END			3

static int trip_status = 0;

gpsData_t g_gpsdata;

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	init_seco_obd_mgr("/dev/ttyHSL1", 115200, NULL);	

	thread_model = thread_katech_obd;
	exit_thread_model = exit_thread_katech_obd;

	katech_obd_mgr__set_ta1_interval_sec(KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC);
	katech_obd_mgr__set_ta2_interval_sec(KATECH_OBD_TA2_INTERVAL_DEFAULT_SEC);

	tripdata__set_car_vin("KMHSU81UBGU678413");
	katech_tools__set_svr_stat(KATECH_SVR_STAT_NONE);

    init_dev_boot_time();
}

void network_on_callback(void)
{
	/*
    if ( katech_tools__get_svr_stat() == KATECH_SVR_STAT_NONE )
	{
		printf("send auth pkt...\r\n");
		katech_pkt_auth_send();
	}
	*/
}

void button1_callback(void)
{

}

void button2_callback(void)
{

}

void ignition_on_callback(void)
{
	printf("ignition on callback!!!!\r\n");
	printf("ignition on callback!!!!\r\n");
	printf("ignition on callback!!!!\r\n");

	start_tripdata();
}

void ignition_off_callback(void)
{
	printf("ignition off callback!!!!\r\n");
	printf("ignition off callback!!!!\r\n");
	printf("ignition off callback!!!!\r\n");

    end_tripdata();

    gpsData_t cur_gpsdata = {0,};
    
    gps_get_curr_data(&cur_gpsdata);
    katech_pkt_1_insert_and_send(&cur_gpsdata, KATECH_PKT_IMMEDIATELY_SEND);
    //katech_pkt_2_insert_and_send(&cur_gpsdata, KATECH_PKT_IMMEDIATELY_SEND);
}

void power_on_callback(void)
{
	set_obd_auto_poweroff_sec(60);
}

void power_off_callback(void)
{
	poweroff(__FUNCTION__, sizeof(__FUNCTION__));
}

#define GET_OBD_CMD_TA2_INTERVAL_SEC    600

SECO_CMD_DATA_SRR_TA1_T g_obd_ta1_buff_cur;
SECO_CMD_DATA_SRR_TA2_T g_obd_ta2_buff_cur;

void gps_parse_one_context_callback(void)
{
//	int res = 0;
//    static int time_cnt = 0;
//	static int gps_cnt = 0;
//	static int init_pkt_res = 0;

    gpsData_t cur_gpsdata = {0,};
    gpsData_t last_gpsdata = {0,};

	// 초당 1번이라고 가정하고 코딩한다.
    gps_get_curr_data(&cur_gpsdata);
    
	if (( trip_status == TRIP_START) || ( trip_status == TRIP_CALC ) )
	{
	}
	
	// 시간체크한다.
	if ( gps_chk_valid_time(&cur_gpsdata) <= 0 )
        return;
    
    // 잡혔을때..
    if ( cur_gpsdata.active == 1 ) 
	{
        mileage_process(&cur_gpsdata);
    	katech_pkt_1_insert_and_send(&cur_gpsdata, KATECH_PKT_INTERVAL_SEND);
	}
    else // 안잡혔을때..
    {
        gps_valid_data_get(&last_gpsdata);
        last_gpsdata.satellite = 0;
        katech_pkt_1_insert_and_send(&last_gpsdata, KATECH_PKT_INTERVAL_SEND);
    }

}

void main_loop_callback(void)
{
	int time_cnt = 0;
	int auth_fail_chk_cnt = 0;

	katech_obd_mgr__timeserise_calc_init();
	
	while(flag_run_thread_main)
	{

        // -------------------------------------------------------
        // server auth senario
        // -------------------------------------------------------
        // 서버인증 체크 인증용 패킷 날린다.
  		LOGI(LOG_TARGET, "MAIN :: SERVER STAT [%d]\n", katech_tools__get_svr_stat());

        if ( katech_tools__get_svr_stat() == KATECH_SVR_STAT_NONE )
        {
            LOGI(LOG_TARGET, "MAIN :: SEND AUTH PKT [%d]\n", katech_tools__get_svr_stat());
            katech_pkt_auth_send();
        }
        
        calc_tripdata();

		time_cnt++;
		sleep(1);
	}
}

void terminate_app_callback(void)
{
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;

	// 60 초
}

void network_fail_emergency_reset_callback(void)
{

}
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/stackdump.h>
#include <logd_rpc.h>
#include <board/board_system.h>
#include <netcom.h>
#include <callback.h>
#include <config.h>


#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"


static int flag_run_thread_main = 1;
int first_key_send_evt = 0;

void abort_callback(void)
{
	
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");
	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
	stackdump_abort_callback = abort_callback;	

    skyan_senario__pre_init();

}

void network_on_callback(void)
{
	printf("gtrack calback ::: network_on_callback !!!\r\n");
}

void button1_callback(void)
{
	printf("gtrack calback ::: button1_callback !!!\r\n");
}

void button2_callback(void)
{
	printf("gtrack calback ::: button2_callback !!!\r\n");
}

void ignition_on_callback(void)
{
    printf("gtrack calback ::: ignition_on_callback !!!\r\n");
    skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_DEVIDE);
    skyan_tools__set_key_stat(SKYAN_KEY_STAT_ON);
    
    if ( skyan_tools__get_nostart_flag() == SKYAN_TOOLS__NORMAL_MODE )
        send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__KEYON);
    else
        send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__NOSTART);

    clear_init_all_geo_fence_v2();
    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__GEOFENCE__GET_FROM_SVR);

    first_key_send_evt = 1;
    // send to genfence req
}

void ignition_off_callback(void)
{
    printf("gtrack calback ::: ignition_off_callback !!!\r\n");
    skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_DEVIDE);
    skyan_tools__set_key_stat(SKYAN_KEY_STAT_OFF);

    // if ( skyan_tools__get_nostart_flag() == SKYAN_TOOLS__NORMAL_MODE )
        send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__KEYOFF);

    first_key_send_evt = 1;
    // clear geofence clear
}

void power_on_callback(void)
{	
    skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_DEVIDE);
	printf("gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	printf("gtrack calback ::: power_off_callback !!!\r\n");

    skyan_tools__set_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_DEVIDE);
    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__REMOVE_BATT);
    skyan_senario__poweroff();

}

void gps_parse_one_context_callback(void)
{
	printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}

void gps_ant_ok_callback(void)
{
    // clear bit
    skyan_tools__clear_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__GPS_CONN);

    if ( skyan_tools__get_gps_ant_stat() != SKYAN_TOOLS__GPS_ANT_STAT_OK)
    {
        skyan_tools__set_gps_ant_stat(SKYAN_TOOLS__GPS_ANT_STAT_OK);

        // if ( skyan_tools__get_key_stat() == SKYAN_KEY_STAT_ON )
    }
    printf("gtrack calback ::: gps_ant_ok_callback !!!\r\n");
}

void gps_ant_nok_callback(void)
{
    // set bit
    skyan_tools__set_devstat(SKY_AUTONET_PKT__DEV_STAT_BIT__GPS_CONN);

    if ( skyan_tools__get_gps_ant_stat() != SKYAN_TOOLS__GPS_ANT_STAT_NOK)
    {
        skyan_tools__set_gps_ant_stat(SKYAN_TOOLS__GPS_ANT_STAT_NOK);

        if ( skyan_tools__get_key_stat() == SKYAN_KEY_STAT_ON )
            send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__GPS_ANT_DISCONN);
    }
    printf("gtrack calback ::: gps_ant_nok_callback !!!\r\n");
}


void main_loop_callback(void)
{
    // wait for first key stat..
    while(first_key_send_evt == 1)
        sleep(1);

    skyan_senario__init();

	while(flag_run_thread_main)
	{
		printf("gtrack calback ::: main_loop_callback !!!\r\n");
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		sleep(1);

        if ( skyan_tools__get_nostart_flag() == SKYAN_TOOLS__NORMAL_MODE )
            skyan_senario__start_callback();

        if ( skyan_tools__get_nostart_flag() == SKYAN_TOOLS__NOSTART_MODE )
            skyan_senario__nostart_callback();
	}
}

void terminate_app_callback(void)
{
	printf("gtrack calback ::: terminate_app_callback !!!\r\n");
}

void exit_main_loop(void)
{
	printf("gtrack calback ::: exit_main_loop !!!\r\n");

	flag_run_thread_main = 0;
}

void network_fail_emergency_reset_callback(void)
{
    skyan_senario__poweroff();
}
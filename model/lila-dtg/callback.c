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

#include "dtg_pkt_senario.h"

static int flag_run_thread_main = 1;


void abort_callback(void)
{
	
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");
	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
	stackdump_abort_callback = abort_callback;	
}

void init_dtg_callback(void)
{
    lila_dtg_init(); //dtg senario func
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
}

void ignition_off_callback(void)
{
	printf("gtrack calback ::: ignition_off_callback !!!\r\n");

}

void power_on_callback(void)
{	
	printf("gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	printf("gtrack calback ::: power_off_callback !!!\r\n");

}

void gps_parse_one_context_callback(void)
{
	//printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}

void main_loop_callback(void)
{
	lila_adas_mgr__init();
	lila_dtg__send_dtg_header();
	
	//
	while(flag_run_thread_main)
	{
		//printf("gtrack calback ::: main_loop_callback !!!\r\n");
		// lila_dtg__korean_test();
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		lila_dtg__pkt_sernaio();
		
		sleep(1);
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

}


void gps_ant_ok_callback(void)
{

}

void gps_ant_nok_callback(void)
{

}
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
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>
#include "logd/logd_rpc.h"

#include <base/thermtool.h>

#include "http_thermal_pkt.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL


static int flag_run_thread_main = 1;
static int _g_cur_power_flag = 0;
static int _process_poweroff(char * log);

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");
	// thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	therm_set_sense_cycle(conf->model.thermal_sensor_interval);
	therm_set_dev(conf->model.thermal_sensor_dev, strlen(conf->model.thermal_sensor_dev));
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
	_g_cur_power_flag = 1;
	printf("gtrack calback ::: ignition_on_callback !!!\r\n");
}

void ignition_off_callback(void)
{
	_g_cur_power_flag = 0;
	printf("gtrack calback ::: ignition_off_callback !!!\r\n");

}

void power_on_callback(void)
{	
	printf("gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	printf("gtrack calback ::: power_off_callback !!!\r\n");
	_process_poweroff(NULL);

}

void gps_parse_one_context_callback(void)
{
	gpsData_t gpsdata;
	gps_get_curr_data(&gpsdata);

	if ( gps_chk_valid_time(&gpsdata) <= 0 )
		return; 
	//printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}


void main_loop_callback(void)
{
	int main_cnt_send_http_thermal = 0 ;
	configurationModel_t *conf = get_config_model();

	while(flag_run_thread_main)
	{
		int http_thermal_send_interval = 0;

		//printf("gtrack calback ::: main_loop_callback !!!\r\n");
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();

		if ( _g_cur_power_flag == 1 )
		{
			http_thermal_send_interval =  conf->model.http_send_interval_keyon;
		}
		else
		{
			http_thermal_send_interval =  conf->model.http_send_interval_keyon;
		}

		LOGI(LOG_TARGET, "[MAIN] thermal send time [%d]/[%d] \n", main_cnt_send_http_thermal, http_thermal_send_interval);

		if ( main_cnt_send_http_thermal > http_thermal_send_interval)
		{
			thermaldata_t sensor_val = {0,};
			main_cnt_send_http_thermal = 0;
			ds_get_thermal_val(&sensor_val);
			sender_add_data_to_buffer(PACKET_TYPE_THERMAL_HTTP, &sensor_val, ePIPE_1);
		}


		main_cnt_send_http_thermal++;
		therm_sense();
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

static int _process_poweroff(char * log)
{
//	save_geo_fence_status_info();

	devel_webdm_send_log("Accumulate distance : %um at the end\n", mileage_get_m());
	
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	
	poweroff(log, strlen(log));

	return 0;
}


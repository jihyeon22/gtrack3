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
#include <base/dmmgr.h>
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


#include "btn_key_mgr.h"

#include "mdt_pkt_senario.h"
#include "dvr_pkt_senario.h"
#include "dtg_pkt_senario.h"

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/gps_utill.h>
#include <mdt800/geofence.h>
#include <mdt800/file_mileage.h>


void abort_callback(void);

static int _process_poweroff(int now_poweroff_flag, char *log);
static int flag_run_thread_main = 1;

int is_run_ignition_off = 0;
//static time_t prev_gps_active_time = 0;

void wait_time_sync()
{
	gpsData_t cur_gpsdata;

	while(1) {
		gps_get_curr_data(&cur_gpsdata);
		if(cur_gpsdata.year > 2013)
			break;
		
		sleep(5);

		printf("wait_time_sync : year[%d]\n", cur_gpsdata.year);
		LOGI(LOG_TARGET, "wait_time_sync : year[%d]\n", cur_gpsdata.year);
	}
}

void abort_callback(void)
{
	save_mileage_file(get_server_mileage() + get_gps_mileage());
}

void init_model_callback(void)
{
	int mileage = 0;
	configurationModel_t *conf = get_config_model();

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
	printf("%s ++\n", __func__);
	stackdump_abort_callback = abort_callback; 

	//thread_model = thread_keypad;
	//exit_thread_model = exit_thread_keypad;

	LOGI(LOG_TARGET, "init_model_callback\n");
	load_mileage_file(&mileage);
	set_server_mileage(mileage);

	if(conf->model.tempature_enable == 1)
	{
		therm_set_sense_cycle(conf->model.tempature_cycle);
		therm_set_dev(conf->model.tempature_device, strlen(conf->model.tempature_device));
	}

}

void init_dtg_callback(void)
{
    bizincar_dtg_init(); //dtg senario func
}

void network_on_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "network_on_callback ++\n");
	/*
	char smsmsg[100] = {0,};
	sprintf(smsmsg, "start %u %u\n", mileage_get_m(), (unsigned int)tools_get_kerneltime());
	devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	*/
}

void test_gps_func();

#ifdef FEATURE_GEO_FENCE_SIMULATION
	int geo_test_flag = 0;
#endif
void button2_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "button2_callback ++\n");
	
	sender_add_data_to_buffer(eKIT_DEFINE_2_EVT, NULL, ePIPE_2);
	
#ifdef FEATURE_GEO_FENCE_SIMULATION
	geo_test_flag = 1; //geo in
#endif
	//test_gps_func();
}
void button1_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "button1_callback ++\n");
	
	sender_add_data_to_buffer(eKIT_DEFINE_1_EVT, NULL, ePIPE_2);

#ifdef FEATURE_GEO_FENCE_SIMULATION
	geo_test_flag = 0; //geo out
#endif
	//test_gps_func();
}

void ignition_on_callback(void)
{
//	configurationModel_t *conf = get_config_model();

	is_run_ignition_off = 0;

	LOGI(LOG_TARGET, "ignition_on_callback ++\n");
	
	// init_gps_manager(); //jwrho

	wait_time_sync();

    // ------------------------------------------------------
    // mdt pkt
    // ------------------------------------------------------
    // mdt key on sernaio is moved
    //  --> fake_ignition_on_callback_mdt();

    // ------------------------------------------------------
    // dvr pkt
    // ------------------------------------------------------
    sender_add_data_to_buffer(eDTG_CUSTOM_EVT__DTG_KEY_ON, NULL, ePIPE_2);
}


void ignition_off_callback(void)
{
//	configurationModel_t *conf = get_config_model();
    
	is_run_ignition_off = 1;
	
	LOGI(LOG_TARGET, "ignition_off_callback ++\n");

	wait_time_sync();

	//TODO : last gps pos file save for correcting more gen-fence
    // ------------------------------------------------------
    // mdt pkt
    // ------------------------------------------------------
    // mdt key off sernaio is moved
       //  --> fake_ignition_off_callback_mdt();
	sender_add_data_to_buffer(eIGN_OFF_EVT, NULL, ePIPE_2);
	sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
	save_mileage_file(get_server_mileage() + get_gps_mileage());

    // ------------------------------------------------------
    // dtg pkt
    // ------------------------------------------------------
    sender_add_data_to_buffer(eDTG_CUSTOM_EVT__DTG_KEY_OFF, NULL, ePIPE_2);
}

void power_on_callback(void)
{
	LOGI(LOG_TARGET, "power_on_callback ++\n");

	// sender_add_data_to_buffer(ePOWER_SOURCE_CHANGE_EVT, NULL, ePIPE_2);

    // ------------------------------------------------------
    // dtg pkt
    // ------------------------------------------------------
    sender_add_data_to_buffer(eDTG_CUSTOM_EVT__DTG_POWER_ON, NULL, ePIPE_2);
}

void power_off_callback(void)
{
    return;
	int batt = 0;

	printf("%s ++\n", __func__);

	if(is_run_ignition_off == 0 && power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		ignition_off_callback();
	}

	LOGI(LOG_TARGET, "power_off_callback ++\n");
	sender_add_data_to_buffer(ePOWER_SOURCE_CHANGE_EVT, NULL, ePIPE_2);
	
    // dtg pkt
    sender_add_data_to_buffer(eDTG_CUSTOM_EVT__DTG_POWER_OFF, NULL, ePIPE_2);
    
	batt = battery_get_battlevel_internal();
	if(batt > 0 && batt < 3600)
	{
		devel_webdm_send_log("Warn : Internal battery is low - %d(mV)\n", batt);
	}
	
	_process_poweroff(1, "power_off_callback");
}



void main_loop_callback(void)
{
#ifdef KEYOFF_RESET_SERNAIO
	time_t system_on_time = 0;
	time_t current_time = 0;
	int ign_status;

	prev_gps_active_time = tools_get_kerneltime();
	system_on_time = tools_get_kerneltime();
#endif

	init_gps_manager(); //jwrho
	init_geo_fence();
	
	setting_network_param();

	while(flag_run_thread_main && nettool_get_state() != DEFINES_MDS_OK) {
		LOGI(LOG_TARGET, "%s : first time wating untill network enable..\n", __func__);
		sleep(3);
	}

	wait_time_sync();

    // ------------------------------------------------------
    // dvr read thread..
    // ------------------------------------------------------
    bizincar_dvr__mgr_init();

    // ------------------------------------------------------
    // force send to key on evt;
    // ------------------------------------------------------
    // dmmgr_send(eEVENT_KEY_ON, NULL, 0); // move to fake_ignition_on_callback_mdt();
    ignition_on_callback();

    // ------------------------------------------------------
    // force send to power on evt;
    // ------------------------------------------------------
    dmmgr_send(eEVENT_PWR_ON, NULL, 0);
    power_on_callback();

	while(flag_run_thread_main)
	{
        bizincar_mdt_pkt_sernaio();
        bizincar_dtg_pkt_sernaio();
        
        chk_runtime_network_chk();
		watchdog_set_cur_ktime(eWdMain);

        sleep(1);

#ifdef KEYOFF_RESET_SERNAIO
		current_time = tools_get_kerneltime();
		ign_status = power_get_ignition_status();

		// if system working keep while 48 hours, system will reset in key off status.
		if(ign_status == POWER_IGNITION_OFF || tools_get_kerneltime() - prev_gps_active_time >= 1800)
		{
			int remain_mem = 0;
			
			remain_mem = tools_get_available_memory();
			if(remain_mem >= 0 && remain_mem < 5000) //5000KB(5MB) 
			{
				devel_webdm_send_log("Warn : Lack of Available memory.");
				_process_poweroff(1, "main_loop_callback:leak mem");
			}

			LOGI(LOG_TARGET, "check auto reset %d:%d = [%d]sec > [%d] sec\n", current_time, system_on_time, (current_time-system_on_time), (24 * 3600));
//			if(current_time - system_on_time > (48 * 3600) ) 
			if(current_time - system_on_time > (24 * 3600) ) 
			{
				system_on_time = tools_get_kerneltime();
				_process_poweroff(0, "main_loop_callback:regular poweroff");
			}
		}
#endif
		watchdog_process();

	}
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}

void terminate_app_callback(void)
{
	save_mileage_file(get_server_mileage() + get_gps_mileage());
}

static int _process_poweroff(int now_poweroff_flag, char *log)
{
//	static int poweroff_count = 0;
	char smsmsg[100] = {0,};

//	if(poweroff_count++ > 5 || now_poweroff_flag == 1)
	{
		gps_valid_data_write();

		sprintf(smsmsg, "Total Accumulate distance : %um at the end\n", get_gps_mileage() + get_server_mileage());
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
						
		//if(poweroff_count > 5)
		if(now_poweroff_flag == 0)
		{
			//devel_webdm_send_log("regular poweroff %d", poweroff_count);
			devel_webdm_send_log("regular poweroff");
		}

		sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
		poweroff(log, strlen(log));
	}
//	else
//	{
//		system("echo 3 > /proc/sys/vm/drop_caches &");
//	}

	return 0;
}

void network_fail_emergency_reset_callback(void)
{
    _process_poweroff(1, "network_fail_reset");
}
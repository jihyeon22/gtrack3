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

#include <netcom.h>
#include <callback.h>
#include <config.h>
#include <data-list.h>

#include "lotte_packet.h"
#include "lotte_gpsmng.h"
#include "lotte_gps_utill.h"
#include "lotte_geofence.h"
#include "debug.h"
#include "lotte_file_mileage.h"
#include "thread-model.h"
#include "gps_ant.h"

void abort_callback(void);

static int _process_poweroff(int now_poweroff_flag, char *log);
static void _check_poweroff(int power_status, time_t current_time);

static int flag_run_thread_main = 1;
static int power_status = 1;
static int is_run_power_off = 0;
static time_t prev_gps_active_time = 0;

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

void hang_power_off()
{
	while(is_run_power_off)
	{
		sleep(10);
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

	//thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	thread_network_set_warn_timeout(3600);
	
	printf("%s ++\n", __func__);
	stackdump_abort_callback = abort_callback; 
	thread_model = thread_keypad;
	exit_thread_model = exit_thread_keypad;
	LOGI(LOG_TARGET, "init_model_callback\n");
	load_mileage_file(&mileage);
	set_server_mileage(mileage);

	if(conf->model.tempature_enable == 1)
	{
		therm_set_sense_cycle(conf->model.tempature_cycle);
		therm_set_dev(conf->model.tempature_device, strlen(conf->model.tempature_device));
	}
}

void network_on_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "lotte:network_on_callback ++\n");
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
	LOGI(LOG_TARGET, "lotte:button2_callback ++\n");
	
#ifdef FEATURE_GEO_FENCE_SIMULATION
	geo_test_flag = 1; //geo in
#endif
	//test_gps_func();
}
void button1_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "lotte:button1_callback ++\n");
	
	sender_add_data_to_buffer(eGPIO_NUM0_EVT, NULL, ePIPE_1);

#ifdef FEATURE_GEO_FENCE_SIMULATION
	geo_test_flag = 0; //geo out
#endif
	//test_gps_func();
}

void ignition_on_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "lotte:ignition_on_callback ++\n");
	
	init_gps_manager(); //jwrho

	if(conf->model.tempature_enable == 1)
	{
		therm_clear_fault();
		therm_set_sense_cycle(conf->model.tempature_cycle);
	}

	wait_time_sync();

	hang_power_off();

	sender_add_data_to_buffer(eIGN_ON_EVT, NULL, ePIPE_1);
	
	devel_webdm_send_log("MILEAGE : %d(m) at ignition-on.\n", get_server_mileage() + get_gps_mileage());
}

void ignition_off_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "lotte:ignition_off_callback ++\n");

	if(conf->model.tempature_enable == 1)
	{
		therm_set_sense_cycle(conf->model.tempature_cycle*3);
	}

	wait_time_sync();

	hang_power_off();

	//TODO : last gps pos file save for correcting more gen-fence
	sender_add_data_to_buffer(eIGN_OFF_EVT, NULL, ePIPE_1);
	sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
	save_mileage_file(get_server_mileage() + get_gps_mileage());

	devel_webdm_send_log("MILEAGE : %d(m) at ignition-off.\n", get_server_mileage() + get_gps_mileage());
}

void power_on_callback(void)
{
	power_status = 1;

	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "lotte:power_on_callback ++\n");

	hang_power_off();
}

void power_off_callback(void)
{
	power_status = 0;

	LOGI(LOG_TARGET, "lotte:power_off_callback ++\n");

	hang_power_off();

	sender_add_data_to_buffer(ePOWER_SOURCE_CHANGE_EVT, NULL, ePIPE_1);
}

void check_mileage_status(gpsData_t *cur_gpsdata)
{
	static int prev_mileage = -1;
	static int err_count = 0;
	static int sended_error_noti = 0;
	
	if(prev_mileage == -1)
	{
		prev_mileage = get_gps_mileage();

		return;
	}

	if(power_get_ignition_status() == POWER_IGNITION_ON)
	{
		if(cur_gpsdata->active && cur_gpsdata->speed > 0)
		{
			if(prev_mileage == get_gps_mileage())
			{
				err_count++;
				LOGT(LOG_TARGET, "MILEAGE : mileage is not chaged. CNT:%d", err_count);
			}
			else
			{
				err_count = 0;
				prev_mileage = get_gps_mileage();
			}
		}
	}

	if(err_count < 60)
	{
		return;
	}

	if(sended_error_noti == 0)
	{
		sended_error_noti = 1;
		devel_webdm_send_log("Mileage : Couldn't change mileage.");
	}
}

void gps_parse_one_context_callback(void)
{
	gps_condition_t ret = eUNKNOWN_GPS_DATA;
	gpsData_t cur_gpsdata;
	gpsData_t gpsdata;
	lotte_packet_t *p_packet;
	////////////////////////////////////////////////////////////////////////
	// 1. gps packet gathering and filtering
	////////////////////////////////////////////////////////////////////////
#ifdef TEST_CODE_ENABLE //for test
	if(gps_get_curr_data(&cur_gpsdata) < 0)
		return -1;
#else
	gps_get_curr_data(&cur_gpsdata);
#endif

	static int show_mileage = 0;
	if(show_mileage++ >= 5)
	{
		show_mileage = 0;
		LOGI(LOG_TARGET, "\n-----------------------------------------\nserver_mileage[%d], gps_mileage = [%d]\n-----------------------------------------", get_server_mileage(), get_gps_mileage());
	}

	switch(cur_gpsdata.active)
	{
		case eINACTIVE:
			ret = inactive_gps_process(cur_gpsdata, &gpsdata);
			break;
		case eACTIVE:
			if(cur_gpsdata.speed > 0)
				prev_gps_active_time = tools_get_kerneltime();
			ret = active_gps_process(cur_gpsdata, &gpsdata);
			break;
	}

	check_mileage_status(&cur_gpsdata);

	if(ret != eUSE_GPS_DATA) {
		print_skip_gps_reason(ret, cur_gpsdata);
		return;
	}

	LOGD(LOG_TARGET, "cur_gpsdata.active = [%d], this gps condition[%d]\n", cur_gpsdata.active, ret);

#ifdef TEST_CODE_ENABLE //for test
	printf("gps data use : idx[%d]\n", cur_gpsdata.num);
	printf("num[%d], active[%d], pos[%d]\n", gpsdata.num, gpsdata.active, gpsdata.lat);
#endif
	
	LOGD(LOG_TARGET, "gps local time and date: %s\n", asctime(localtime(&gpsdata.utc_sec)) );	

	////////////////////////////////////////////////////////////////////////
	// 2. create packet and add packet to buffer
	////////////////////////////////////////////////////////////////////////
	p_packet = (lotte_packet_t *)malloc(sizeof(lotte_packet_t));
	if(p_packet == NULL) {
		LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
		return;
	}

	create_report_data(eCYCLE_REPORT_EVC, p_packet, gpsdata);

	if(list_add(&gps_buffer_list, p_packet) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
		free(p_packet);
	}

	LOGI(LOG_TARGET, "current report packet buffering count[%d]\n", get_gps_data_count());
	return;
}

void main_loop_callback(void)
{
	int at_recov_cnt = 1024;
	int msg_print_cnt = 0;
	int report_interval = 180; //3 min
	int collect_interval = 10;
	time_t system_on_time = 0;
	time_t current_time = 0;
	int ign_status;
	configurationModel_t *conf = get_config_model();

	init_gps_manager(); //jwrho
	init_geo_fence();
	
	setting_network_param();

	while(flag_run_thread_main && nettool_get_state() != DEFINES_MDS_OK) {
		LOGI(LOG_TARGET, "%s : first time wating untill network enable..\n", __func__);
		_check_poweroff(power_status, tools_get_kerneltime());
		sleep(3);
	}

	//at_channel_recovery();

	wait_time_sync();

	prev_gps_active_time = tools_get_kerneltime();

	system_on_time = tools_get_kerneltime();
	msg_print_cnt = 0;

	if(conf->model.tempature_enable == 1)
	{
		therm_sense();
	}

	while(flag_run_thread_main)
	{
		int condition_send = 0;
		
		if(at_recov_cnt-- < 0) {
			//at_channel_recovery();
			at_recov_cnt = 1024;
		}

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

		if(conf->model.tempature_enable == 1)
		{
			therm_sense();
		}

		gps_ant_check();
		
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

		_check_poweroff(power_status, current_time);
		watchdog_process();
		watchdog_set_cur_ktime(eWdMain);

		sleep(5);
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

#define POWEROFF_STANDBY_SECS 3600
static void _check_poweroff(int power_status, time_t current_time)
{
	static time_t ktime_key_off = 0;

	//Check Power
	if(power_status == 0)
	{
		int batt = 0;
	
		if(ktime_key_off == 0)
		{
			ktime_key_off = current_time;
		}

		if(current_time - ktime_key_off > POWEROFF_STANDBY_SECS)
		{
			LOGT(LOG_TARGET, "Poweroff standby time expired! : Do poweroff...");
			_process_poweroff(1, "_check_poweroff:standby");
		}

		batt = battery_get_battlevel_internal();
		if(batt > 0 && batt < 3700)
		{
			LOGT(LOG_TARGET, "batt internal %d mv\n", batt);
			devel_webdm_send_log("Warn : Internal battery is low - %d(mV)\n", batt);
			_process_poweroff(1, "_check_poweroff:low battery");
		}
	}
	else
	{
		ktime_key_off = 0;
	}
}

static int _process_poweroff(int now_poweroff_flag, char *log)
{
//	static int poweroff_count = 0;
	char smsmsg[100] = {0,};

//	if(poweroff_count++ > 5 || now_poweroff_flag == 1)
	{
		is_run_power_off = 1;
	
		sprintf(smsmsg, "Accumulate distance : %um at the end\n", get_gps_mileage());
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		devel_webdm_send_log(smsmsg);

		gps_valid_data_write();
		
//		if(poweroff_count > 5)
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


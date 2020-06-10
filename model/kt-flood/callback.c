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
#include "debug.h"
#include "netcom.h"

#include <board/board_system.h>

#include <kt_flood_mdt800/packet.h>
#include <kt_flood_mdt800/gpsmng.h>
#include <kt_flood_mdt800/gps_utill.h>
#include <kt_flood_mdt800/geofence.h>
#include <kt_flood_mdt800/file_mileage.h>

#include "kt_flood_unit.h"

#include "color_printf.h"

#include <signal.h>

void abort_callback(void);

static int _process_poweroff(int now_poweroff_flag, char *log);
static int flag_run_thread_main = 1;

static time_t prev_gps_active_time = 0;
static unsigned int cycle_report = 0;

static int is_run_ignition_off = 0;
int is_list_init = 0;
extern int sever_save_interval;
extern int is_sever_modify_ip;
extern char g_sever_ip[40];

#define INTERVAL 1

void wait_time_sync()
{

}
void report_timer(int signo)
{
	int interval_time = get_report_interval();
	int ign_status = power_get_ignition_status();

	if(sever_save_interval > 0)
		interval_time = sever_save_interval;

    //printf("mytimer called........ %d \n", interval_time);

	if(interval_time >0 && ++cycle_report >= interval_time && ign_status == POWER_IGNITION_ON)
	{
		//print_red("cycle_report\n");
		sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
		cycle_report = 0;
	}

    alarm(INTERVAL); 

}

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

void network_on_callback(void)
{
	printf("gtrack calback ::: network_on_callback !!!\r\n");
}

void button1_callback(void)
{
	printf("gtrack callback ::: button1_callback !!!\r\n");

	// interval TEST
	// char str_interval[16];
	// int test_interval = 20;
	// configurationModel_t *conf = get_config_model();

	// printf("conf->model.report_interval_keyon3 %d\n", conf->model.report_interval_keyon);
	// memset(str_interval, 0x00, sizeof(str_interval));	
	// sprintf(str_interval,"%d",test_interval);

	// if(save_config_user("user:collect_interval_keyon", (char *)str_interval) < 0)
	// {
	// 	LOGE(LOG_TARGET, "<%s> save config error #3\n", __FUNCTION__);
	// 	return -1;
	// }
	// if(save_config_user("user:report_interval_keyon", (char *)str_interval) < 0)
	// {
	// 	LOGE(LOG_TARGET, "<%s> save config error #4\n", __FUNCTION__);
	// 	return -1;
	// }
	// if(save_config_user("user:collect_interval_keyoff", (char *)str_interval) < 0)
	// {
	// 	LOGE(LOG_TARGET, "<%s> save config error #5\n", __FUNCTION__);
	// 	return -1;
	// }
	// if(save_config_user("user:report_interval_keyoff", (char *)str_interval) < 0)
	// {
	// 	LOGE(LOG_TARGET, "<%s> save config error #6\n", __FUNCTION__);
	// 	return -1;
	// }

	// conf = load_config_user();
	// printf("conf->model.report_interval_keyon2 %d\n", conf->model.report_interval_keyon);
	
	// sever_save_interval = conf->model.report_interval_keyon;	
}

void button2_callback(void)
{
	printf("gtrack callback ::: button2_callback !!!\r\n");

	LOGD(LOG_TARGET, "%s : button2_callback\n", __func__);

}

void ignition_on_callback(void)
{
	printf("gtrack callback ::: ignition_on_callback !!!\r\n");

	is_run_ignition_off = 0;
	is_list_init = 0;
	sever_save_interval = 0;
	is_sever_modify_ip = 0;
	cycle_report = 50;
	
	memset(g_sensor_state, 0x00, sizeof(g_sensor_state));
	memset(g_sever_ip, 0x00, sizeof(g_sever_ip));
	

	LOGD(LOG_TARGET, "%s : ignition_on_callback\n", __func__);
	//sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
}

void ignition_off_callback(void)
{
	printf("gtrack callback ::: ignition_off_callback !!!\r\n");
	LOGD(LOG_TARGET, "%s : ignition_off_callback\n", __func__);
	is_run_ignition_off = 1;

	if(sever_save_interval > 0) 
	{
		char str_interval[16];

		sprintf(str_interval,"%d",sever_save_interval);
		printf("str_interval3 %s\n", str_interval);

		if(save_config_user("user:report_interval_keyon", (char *)str_interval) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #4\n", __FUNCTION__);
			print_yellow("<%s> save config error #4\n", __FUNCTION__);
			return -1;
		}
		if(save_config_user("user:collect_interval_keyon", (char *)str_interval) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #3\n", __FUNCTION__);
			print_yellow("<%s> save config error #3\n", __FUNCTION__);
			return -1;
		}
		if(save_config_user("user:collect_interval_keyoff", (char *)str_interval) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #5\n", __FUNCTION__);
			print_yellow("<%s> save config error #5\n", __FUNCTION__);
			return -1;
		}
		if(save_config_user("user:report_interval_keyoff", (char *)str_interval) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #6\n", __FUNCTION__);
			print_yellow("<%s> save config error #6\n", __FUNCTION__);
			return -1;
		}

	}
	if(is_sever_modify_ip > 1)
	{
		if(save_config_user("user:report_ip", (char *)g_sever_ip) < 0)
		{
			LOGE(LOG_TARGET, "<%s> save config error #1\n", __FUNCTION__);
			return -1;
		}
	}

}

void power_on_callback(void)
{	
	printf("gtrack callback ::: power_on_callback !!!\r\n");
	LOGD(LOG_TARGET, "%s : power_on_callback\n", __func__);

}

void power_off_callback(void)
{
	int batt = 0;
	printf("gtrack callback ::: power_off_callback !!!\r\n");
	LOGD(LOG_TARGET, "%s : power_off_callback\n", __func__);

	if(is_run_ignition_off == 0 && power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		ignition_off_callback();
	}

	// LOGI(LOG_TARGET, "power_off_callback ++\n");
	// sender_add_data_to_buffer(ePOWER_SOURCE_CHANGE_EVT, NULL, ePIPE_2);
	kt_flood_unit_close();

	batt = battery_get_battlevel_internal();
	if(batt > 0 && batt < 3600)
	{
		devel_webdm_send_log("Warn : Internal battery is low - %d(mV)\n", batt);
	}

	_process_poweroff(1, "power_off_callback");


	
}

void gps_parse_one_context_callback(void)
{
	int interval_time = get_report_interval();

	// 우선 GPS 를 사용하지 않음. 
	//printf("gtrack callback ::: gps_parse_one_context_callback !!!\r\n");

	// printf("interval_time : %d\n", interval_time);
	// if(interval_time >0 && ++cycle_report >= interval_time )
	// {
	// 	printf("cycle_report\n");
	// 	sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
	// 	cycle_report = 0;
	// }

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


	init_gps_manager(); 
	init_geo_fence();
	
	// jhcho_kt_flood
	kt_flood_unit_init();
    
	setting_network_param();

	while(flag_run_thread_main && nettool_get_state() != DEFINES_MDS_OK) {
		LOGI(LOG_TARGET, "%s : first time wating untill network enable..\n", __func__);
		sleep(3);
	}

	//at_channel_recovery();

	wait_time_sync();

	prev_gps_active_time = tools_get_kerneltime();

	system_on_time = tools_get_kerneltime();
	msg_print_cnt = 0;

    struct sigaction act;
    act.sa_handler = report_timer;
    //SIGALRM이란 시그널이 발생할 경우에 동작시킬 핸들러 함수를 등록하는 구문
    //SIGALRM이 발생하면 mytimer를 호출 - mytimer에서 원하는 작업을 추가하면 됩니다.
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
 
    sigaction(SIGALRM, &act, 0);
    alarm(INTERVAL);


	while(flag_run_thread_main)
	{
		int condition_send = 0;
		

		watchdog_set_cur_ktime(eWdMain);
		
		if(at_recov_cnt-- < 0) {
			//at_channel_recovery();
			at_recov_cnt = 1024;
		}

		if ( power_get_power_source() == POWER_SRC_DC)
			kt_flood_unit_state_write();

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

		watchdog_process();

		sleep(1);
	}
}

void exit_main_loop(void)
{
	printf("gtrack calback ::: exit_main_loop !!!\r\n");

	flag_run_thread_main = 0;
}

void terminate_app_callback(void)
{

}

static int _process_poweroff(int now_poweroff_flag, char *log)
{
//	static int poweroff_count = 0;
	char smsmsg[100] = {0,};

//	if(poweroff_count++ > 5 || now_poweroff_flag == 1)
	{

		//sprintf(smsmsg, "Accumulate distance : %um at the end\n", get_gps_mileage());
		sprintf(smsmsg, "_process_poweroff  at the end\n");
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		devel_webdm_send_log(smsmsg);
		
		if(now_poweroff_flag == 0)
		{
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
    _process_poweroff(1, "netfail");
}
void gps_ant_ok_callback(void)
{

}

void gps_ant_nok_callback(void)
{

}


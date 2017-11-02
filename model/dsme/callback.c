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

#include <board/modem-time.h>
#include <board/board_system.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "custom.h"
#include "welding_machine.h"
#include "data-list.h"

#define LOG_TARGET eSVC_MODEL

static int flag_run_thread_main = 1;
static int _process_poweroff(int now_poweroff_flag, char *log);

void wait_time_sync()
{
	time_t system_time;
	struct tm *timeinfo = NULL;
	struct tm cur_time;

	while(1) {
		if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
			time(&system_time);
			timeinfo = localtime ( &system_time );
		}
		else {
			timeinfo = (struct tm *)&cur_time;
		}

		if(timeinfo == NULL) {
			LOGE(eSVC_MODEL, "%s> get time info is NULL error\n", __func__);
		}

		if(timeinfo->tm_year+1900 > 2016)
			break;

		sleep(5);

		LOGI(LOG_TARGET, "wait_time_sync : year[%d]\n", timeinfo->tm_year+1900);
	}
}
void abort_callback(void)
{
	
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");
	thread_network_set_warn_timeout(3600 * 12); //12 hour no network error

	stackdump_abort_callback = abort_callback;

	welding_machine_config_setup();
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
	wait_time_sync();
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
	_process_poweroff(1, "power_off_callback");

}

void gps_parse_one_context_callback(void)
{
	printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}

int sms_test_idx = 0;
void main_loop_callback(void)
{
	configurationModel_t *conf = get_config_model();
	time_t currnet_time;
	time_t prev_report_time;
	setting_network_param();
	int ivs_report_time_debug = 0;

	int rand_num = 0;
	srand(time(NULL));
	rand_num = rand();
	printf("rand_num = [%d]\n", rand_num);

	prev_report_time = currnet_time = get_modem_time_utc_sec();

	//tools_get_kerneltime();
	while(flag_run_thread_main)
	{
		//printf("rand_num = [%d]\n", rand_num);

		conf = get_config_model();

		currnet_time = get_modem_time_utc_sec();

		//printf("gtrack calback ::: main_loop_callback !!!\r\n");
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		
		welding_machine_process(rand_num);

		if( (currnet_time - prev_report_time) >= conf->model.report_interval) {
			prev_report_time = currnet_time;
			printf("==================================================\n");
			printf("sender_add_data_to_buffer call #1\n");
			printf("==================================================\n");
			sender_add_data_to_buffer(eREPORT_USER_DATA, NULL, ePIPE_1);

			while(1) {
				if(get_data_count() < conf->model.report_interval) {
					break;
				}
				sender_add_data_to_buffer(eREPORT_USER_DATA, NULL, ePIPE_1);
				sleep(1);
			}
		}
		else {

			if(get_data_count() > conf->model.report_interval * 2) {
				prev_report_time = currnet_time;
				printf("==================================================\n");
				printf("sender_add_data_to_buffer call #2\n");
				printf("==================================================\n");
				sender_add_data_to_buffer(eREPORT_USER_DATA, NULL, ePIPE_1);
			}
			else {
				if(ivs_report_time_debug++ > 20) {
					ivs_report_time_debug = 0;
					LOGI(LOG_TARGET, "ivs report time : [%d/%d]\n", (currnet_time-prev_report_time), conf->model.report_interval);

					if(get_welding_machine_id() != NULL) {
						LOGI(LOG_TARGET, "[%d] welding_machine_id : %s\n", rand_num, get_welding_machine_id());
					}
				}
			}
		}

		//report_interval=60
		//board_rate=38400
		//InV_T_collection_interval=0

/*
		printf("sms_test_idx = [%d]\n", sms_test_idx);
		if(sms_test_idx == 0) {
			parse_model_sms(NULL, NULL, "&IP,[192.168.0.1],[12345]");
			sms_test_idx = 1;
		} else if(sms_test_idx == 1) {
			parse_model_sms(NULL, NULL, "&ip, [192 .168. 0.3], [12345]");
			sms_test_idx = 2;
		} else if(sms_test_idx == 2) {
			parse_model_sms(NULL, NULL, "[IVSCLT,0,600,60]");
			sms_test_idx = 3;
		} else if(sms_test_idx == 3) {
			parse_model_sms(NULL, NULL, "[IVSSTP,0]");
			sms_test_idx = 4;
		} else if(sms_test_idx == 4) {
			parse_model_sms(NULL, NULL, "[ADDCLT,SETID,T0001/SETID]");
			sms_test_idx = 5;
		} else if(sms_test_idx == 5) {
			parse_model_sms(NULL, NULL, "[ADDCLT,IVS/IVS]");
			sms_test_idx = 6;
		} else if(sms_test_idx == 6) {
			parse_model_sms(NULL, NULL, "[RBRSET,9600");
			sms_test_idx = 7;
		} else if(sms_test_idx == 7) {
			parse_model_sms(NULL, NULL, "[RESET]");
			sms_test_idx = 0;
		}
		usleep(1000*2000); //2000ms
*/
		//usleep(1000*500); //500ms
		usleep(1000*500); //500ms

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


static int _process_poweroff(int now_poweroff_flag, char *log)
{

	char smsmsg[100] = {0,};
	if(now_poweroff_flag == 0)
	{
		devel_webdm_send_log("regular poweroff");
	}

	power_off_collect_stop();
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	poweroff(log, strlen(log));
	

	return 0;
}
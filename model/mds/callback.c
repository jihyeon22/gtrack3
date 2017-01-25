#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <base/sender.h>
#include <base/devel.h>
#include <base/mileage.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/stackdump.h>
#include "logd/logd_rpc.h"

#include <netcom.h>
#include <callback.h>
#include <config.h>
#include <mds.h>
#include <data-list.h>

#include <at/at_util.h>
#include <at/at_log.h>
#include <at/watchdog.h>

#include "board/modem-time.h"
#include "board/power.h"
#include "board/board_system.h"

#include "packet.h"
#include "mds_ctx.h"
#include "mds.h"


// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static int _process_poweroff(int now_poweroff_flag, char *log);
static int flag_run_thread_main = 1;

int mds_init_complete = 0;

void abort_callback(void)
{
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
	stackdump_abort_callback = abort_callback;
}

void button1_callback(void)
{
}

void button2_callback(void)
{
}

void network_on_callback(void)
{
	char smsmsg[100] = {0,};
	sprintf(smsmsg,"start %u\n", (unsigned int)tools_get_kerneltime());
	devel_send_sms_noti(smsmsg, sizeof(smsmsg), 3);
	set_ctx_network(1);
}

void ignition_on_callback(void)
{
	int res;
	struct tm time = {0,};
	
	LOGE(LOG_TARGET, "######## ignition on callback \r\n");
	
	if ( get_modem_time_tm(&time) != MODEM_TIME_RET_SUCCESS ) 
	{
		time_t tm_time;
		struct tm* ptime;
		
		LOGI(LOG_TARGET, "get modem time : use atd \r\n");
		
		at_get_modemtime(&tm_time,0);
		ptime = localtime(&tm_time);
		
		memcpy(&time, ptime, sizeof(time));
	}
	
	set_pkt_ctx_keyon_time(time.tm_year + 1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	
	LOGE(LOG_TARGET, "ignition_on_callback : settime ret [%d] \r\n", res);
	
	set_ctx_power(PACKET_POWER_STATUS_ON);

	// 기존에 쌓아두었던 모든 OFF 패킷을 모두 전송한다.
	make_packet_keyoff_data_done();
	send_keyoff_data(0);
}

void ignition_off_callback(void)
{
	int res;
	struct tm time = {0,};
	
	LOGE(LOG_TARGET, "######## ignition off callback \r\n");

	if ( get_modem_time_tm(&time) != MODEM_TIME_RET_SUCCESS ) 
	{
		time_t tm_time;
		struct tm* ptime;
		
		LOGI(LOG_TARGET, "get modem time : use atd \r\n");
		
		at_get_modemtime(&tm_time,0);
		ptime = localtime(&tm_time);
		
		memcpy(&time, ptime, sizeof(time));
	}
		
	set_pkt_ctx_keyoff_time(time.tm_year + 1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

	LOGE(LOG_TARGET, "ignition_off_callback : settime ret [%d] \r\n", res);
	
	set_ctx_power(PACKET_POWER_STATUS_OFF);
		
	// 그동안에 쌓아두었던 모든 ON 패킷을 전송한다.
	make_packet_keyon_data_done();
	send_keyon_data(0);
}

void power_on_callback(void)
{
	LOGE(LOG_TARGET, "######## power_on_callback\n");
}

void power_off_callback(void)
{
	LOGE(LOG_TARGET, "######## power_off_callback\n");

	// battery only mode : turn off immediately
	_process_poweroff(1, "power_off_callback");
}

void gps_parse_one_context_callback(void)
{	
	static unsigned int main_time=0;
	gpsData_t gpsdata;
	//gpsData_t* pdata;
	
	gps_get_curr_data(&gpsdata);

	printf("[kksworks gps]\t%d\t%f\t%f\t%d\t%f\t\r\n",gpsdata.satellite, gpsdata.lat, gpsdata.lon, gpsdata.speed, gpsdata.hdop); 

	if (mds_init_complete)
	{
	
		// Warning!!!!
		// time is not correct. (more and more slow...)
		
		if ( get_ctx_power() == PACKET_POWER_STATUS_ON )
		{
			LOGT(LOG_TARGET, "PowerOn Routine (%d,%d) / %d ------ \r\n", get_ctx_keyon_gather_data_interval(),get_ctx_keyon_send_to_data_interval(), main_time);
			
			model_mds_make_poweron_packet(gpsdata);
			
			model_mds_send_poweron_packet();
		
		}
		else if ( get_ctx_power() == PACKET_POWER_STATUS_OFF )
		{
			LOGT(LOG_TARGET, "PowerOff Routine (%d,%d) / %d ------ \r\n", get_ctx_keyoff_gather_data_interval(),get_ctx_keyoff_send_to_data_interval(), main_time);
			
			model_mds_make_poweroff_packet(gpsdata);
			
			model_mds_send_poweroff_packet();
			
			if (get_ctx_power_is_changed() == PACKET_RET_SUCCESS)
			{
			
				printf("power is changed!!!\r\n");
				printf("power is changed!!!\r\n");
				printf("power is changed!!!\r\n");
				printf("power is changed!!!\r\n");
				printf("power is changed!!!\r\n");
				printf("power is changed!!!\r\n");
			
				model_mds_send_poweroff_event_packet(gpsdata);
			}

		}
		
		model_mds_check_server();
		main_time++;
	}
	
	return;
	
}

int ___utc_localtime(struct tm *utc, struct tm *gmt, int adj_hour)
{
	if(utc == NULL) {
		return GPS_PARSE_FALSE;
	}

	time_t utc_sec = 0;
	utc_sec = mktime(utc);
	utc_sec += adj_hour * 60 * 60;
	localtime_r(&utc_sec, gmt);
	return GPS_PARSE_TRUE;
}

void main_loop_callback(void)
{
	configurationModel_t *conf = NULL;

	int sleep_sec = 10;
	int run_sec = 0;
	
	int ign_status;
	
//	gpsData_t gpsdata = {0,};

//	int res;
//	int time_sec = 1;
	
	mds_init_complete = 0;
	
	conf = get_config_model();

	mds_init();
	setting_network_param();

	set_ctx_keyon_gather_data_interval(conf->model.collect_interval_keyon);
	set_ctx_keyon_send_to_data_interval(conf->model.report_interval_keyon);
	set_ctx_keyoff_gather_data_interval(conf->model.collect_interval_keyoff);
	set_ctx_keyoff_send_to_data_interval(conf->model.report_interval_keyoff);
	
	set_ctx_server_sleep(0);
	
	mds_init_complete = 1;
	

	
	/*
	if (system_on_time == 0)
	{
		struct tm time = {0,};
		struct timespec = {0,};
		
		get_modem_time_tm(&time);

		 time  = mktime( &time);
		user_time = localtime(&time)
		
	}*/

	// ----------------------------------------------
	// 실제 루틴 시작
	// ----------------------------------------------

	while(flag_run_thread_main)
	{

		/*
		printf("run_ctx.keyon_gather_data_interval_sec = [%d]\r\n",run_ctx.keyon_gather_data_interval_sec );
		printf("run_ctx.keyon_send_to_data_interval_sec = [%d]\r\n",run_ctx.keyon_send_to_data_interval_sec);
		printf("run_ctx.keyoff_gather_data_interval_sec = [%d]\r\n",run_ctx.keyoff_gather_data_interval_sec);
		printf("run_ctx.keyoff_send_to_data_interval_sec = [%d]\r\n",run_ctx.keyoff_send_to_data_interval_sec);
		*/
		
		// get gps data..
		// if cannot get gps data then fail. And nothing to do
		
		ign_status = power_get_ignition_status();
		
		// if system working keep while 24 hours, system will reset in key off status.
 
		if(ign_status == POWER_IGNITION_OFF)
		{
			run_sec += sleep_sec;
			
			if( run_sec > (24 * 3600) )
			{
				run_sec = 0;
				printf (" _process_poweroff() \r\n");
				_process_poweroff(0, "regular poweroff");
			}
		}
		
		watchdog_process();
		watchdog_set_cur_ktime(eWdMain);
		
		sleep(sleep_sec);
	}
}

void terminate_app_callback(void)
{
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}



static int _process_poweroff(int now_poweroff_flag, char *log)
{
//	static int poweroff_count = 0;
	char smsmsg[100] = {0,};

//	if(poweroff_count++ > 5 || now_poweroff_flag == 1)
	{
		//LOGT(LOG_TARGET, "_process_poweroff : reset [%d] \r\n",poweroff_count );
		LOGT(LOG_TARGET, "_process_poweroff : reset flag %d\r\n", now_poweroff_flag);

        snprintf(smsmsg, sizeof(smsmsg)-1, "poweroff %u\n", (unsigned int)tools_get_kerneltime());
        devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		
		make_packet_keyoff_data_done();
		send_keyoff_data(0);
		
		make_packet_keyon_data_done();
		send_keyon_data(0);
		
		gps_valid_data_write();
		
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
//		LOGT(LOG_TARGET, "_process_poweroff : drop chache [%d] \r\n",poweroff_count );
//        system("echo 3 > /proc/sys/vm/drop_caches &");
//	}

	return 0;
}



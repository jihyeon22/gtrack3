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
#include <base/watchdog.h>
#include <board/power.h>
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

#include "debug.h"
#include "alloc_packet_tool.h"
#include "geofence.h"
#include "buzzer.h"
#include "alloc_util.h"
#include "alloc_rfid.h"
#include "tagging.h"

// jhcho test
#include "color_printf.h"
#include "Ftp_ClientCmd.h"


static int _process_poweroff(char *log);

int g_rfid_request_flag = 0;
int g_rfid_complelte_flag = 0;
int g_tl500_state = 0; 
int g_rfid_requestdb = 0;
int g_tl500_geofence_reset = 0;
		
static int flag_run_thread_main = 1;

static time_t prev_gps_active_time = 0;

BOOL g_geofencedown = false;
// feature check
// #define GEO_TEST 0
// #ifndef USE_EXTGPIO_EVT
// #error "allocation must set USE_EXTGPIO_EVT feature"
// #endif

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

void wait_rfid_request()
{
	while(!g_rfid_request_flag) {
		LOGI(LOG_TARGET, "wait_rfid_request\n");		
		sleep(1);
	}
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	thread_network_set_warn_timeout(3600);
	
	init_geo_fence(eGEN_FENCE_DEBUG_MODE);

#ifdef USE_EXTGPIO_EVT
	if (init_gpioinput_module("14,15,16,18") == -1)
		LOGE(LOG_TARGET, "GPIO input MODULE init fail!!!!!!!!!!!!!!\n");
#endif
//	alloc_geo_fence_info_init();
// 	alloc_geo_fence_info_load();
	
}

void network_on_callback(void)
{
	printf("%s ++\n", __func__);
	LOGI(LOG_TARGET, "alloc:network_on_callback ++\n");
}

void button1_callback(void)
{
	configurationModel_t *conf = get_config_model();

	print_yellow("gtrack calback ::: button1_callback !!!\r\n");

	// jhcho test [[ 
	g_geofencedown = true;

	struct timeval tv;
	struct tm ttm;

	time_t system_time;
	struct tm *timeinfo;
	char cmd[128] = {0,};

	localtime_r(&tv.tv_sec, &ttm);
	
	time(&system_time);
	timeinfo = localtime ( &system_time );

	sprintf(cmd, "%04d%02d%02d%02d%02d%02d", 
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);


	// sprintf(cmd, "%04d%02d%02d%02d%02d%02d", 
	// 		ttm.tm_year + 1900, ttm.tm_mon + 1, ttm.tm_mday, 
	// 		ttm.tm_hour, ttm.tm_min, ttm.tm_sec);
	print_yellow("cmd rfid reader2!!!! [%s]\r\n", cmd);

	//set_circulating_bus(0);
	load_circulating_bus_info();
	// jhcho test ]]


#if GEO_TEST
	set_recent_geo_fence(get_recent_geo_fence()+1);
	tagging_add_list();
	LOGT(LOG_TARGET, "geocence++ : %d", get_recent_geo_fence());
#endif
}

void button2_callback(void)
{
	printf("gtrack calback ::: button2_callback !!!\r\n");

	set_circulating_bus(1);
#if GEO_TEST
	set_recent_geo_fence(get_recent_geo_fence()-1);
	tagging_add_list();
	LOGT(LOG_TARGET, "geocence-- : %d", get_recent_geo_fence());
#endif
}

#ifdef USE_EXTGPIO_EVT
void button_ext1_callback(void)
{
	LOGI(LOG_TARGET, "alloc : btn ext1\n");
}

void button_ext2_callback(void)
{
	LOGI(LOG_TARGET, "alloc : btn ext2\n");
}

void button_ext3_callback(void)
{
	LOGI(LOG_TARGET, "alloc: btn event :: add passenger count\n");
	tagging_add_count(get_recent_geo_fence());
}

void button_ext4_callback(void)
{
	LOGI(LOG_TARGET, "alloc: btn event :: passenger full\n");
	wait_rfid_request();
	pkt_send_btn_passenger_full();

}
void button_ext5_callback(void)
{
	LOGI(LOG_TARGET, "alloc: btn event :: end drive\n");
	wait_rfid_request();
	pkt_send_end_drive();
}
void button_ext6_callback(void)
{
	LOGI(LOG_TARGET, "alloc: btn event :: emergency \n");
	wait_rfid_request();
	pkt_send_btn_emergency();

}
#endif

void ignition_on_callback(void)
{
	wait_time_sync();
	wait_rfid_request();
	pkt_send_start_drive();
}

void ignition_off_callback(void)
{
	wait_time_sync();
	wait_rfid_request();
	pkt_send_end_drive();
}

void power_on_callback(void)
{
}

void power_off_callback(void)
{
	_process_poweroff("power_off_callback");
}

static unsigned int cycle_report = 0;
#define CALC_DISTANCE_INTERVAL_SEC	10
 
//jwrho ++
static int g_geofence_chk_cnt = 0; 
static int g_geofence_max_wait_count = 30;
//jwrho --
void gps_parse_one_context_callback(void)
{
	int interval_time = get_report_interval();
	
	
	gpsData_t gpsdata;
	fence_notification_t fnoti;
	int fence_num = -1;

	gps_get_curr_data(&gpsdata);

	if(gpsdata.year < 2015)
	{
		return;
	}
	
	//LOGT(LOG_TARGET, "read mileage [%d m]\n", mileage_get_m());
	
	if (cycle_report % CALC_DISTANCE_INTERVAL_SEC == 0 )
	{
		//printf("calc distance!!\r\n");
		mileage_process(&gpsdata);
	}
	
	if(gpsdata.active == 1)
	{
		if(gpsdata.speed > 0)
			prev_gps_active_time = tools_get_kerneltime();

	// -----------------------------------------------------
	// geo fense...
	// -----------------------------------------------------	
		fnoti = get_geofence_notification(&fence_num, gpsdata);
		if(fnoti != eFENCE_NONE_NOTIFICATION)
		{
			if(fnoti == eFENCE_IN_NOTIFICATION)
			{
				//devel_webdm_send_log("debug : geofence in noti [%d]", fence_num);
				if(get_geofence_status() == eGET_GEOFENCE_STAT_COMPLETE) //jwrho
				{
					pkt_send_geofence_in(gpsdata,fence_num);
					print_red (" pkt_send_geofence_in fence_num : %d \r\n", fence_num);

					g_tl500_geofence_reset = 0;
				}
				
			}
			else if(fnoti == eFENCE_OUT_NOTIFICATION)
			{
				//devel_webdm_send_log("debug : geofence out noti [%d]", fence_num);
				if(get_geofence_status() == eGET_GEOFENCE_STAT_COMPLETE) //jwrho
				{
					pkt_send_geofence_out(gpsdata,fence_num);
					print_red ("pkt_send_geofence_out fence_num : %d \r\n", fence_num);
				}
			}
		}
	}

	//printf("packet time [%d]/[%d]\r\n", cycle_report, interval_time);
	//print_yellow("packet time [%d]/[%d]\r\n", cycle_report, interval_time);
	
	if(interval_time >0 && ++cycle_report >= interval_time)
	{
		LOGI(LOG_TARGET, "get_geofence_status() ==> [%d]\r\n", get_geofence_status()); //jwrho
		pkt_send_period_report(gpsdata);
		tagging_add_list();
		pkt_send_tagging();
		cycle_report = 0;
	}

	//jwrho ++
	if(get_geofence_status() == eGET_GEOFENCE_STAT_NULL)
	{
		LOGE(LOG_TARGET, "genfence null stat, [%d/%d]\r\n", g_geofence_chk_cnt, g_geofence_max_wait_count);
		g_geofence_chk_cnt += 1;
		if(g_geofence_chk_cnt > g_geofence_max_wait_count)
		{
			g_geofence_chk_cnt = 0;

			if(g_geofence_max_wait_count < 60) { //1 min
				g_geofence_max_wait_count = 60;
			} else if(g_geofence_max_wait_count < 180) { //3 min
				g_geofence_max_wait_count = 180;
			} else if(g_geofence_max_wait_count < 300) { //5 min
				g_geofence_max_wait_count = 300;
			} else if(g_geofence_max_wait_count < 600) { //10 min
				g_geofence_max_wait_count = 600;
			} else if(g_geofence_max_wait_count < 1800) { //30 min
				g_geofence_max_wait_count = 1800;
			} else { //1 hour
				g_geofence_max_wait_count = 3600;
			}
			sender_add_data_to_buffer(PACKET_TYPE_REQUEST_BUS_STOP_INFO, NULL, ePIPE_2);
		}
	}
	else
	{
		g_geofence_max_wait_count = 30;
		//LOGI(LOG_TARGET, "get_geofence_status() = [%d]\n", get_geofence_status());
	}
	//jwrho --
		

}

void main_loop_callback(void)
{
	int at_recov_cnt = 1024;
	time_t system_on_time = tools_get_kerneltime();

	// 대우 조선 해양에서 APN 을 lte.ktfwing.com 으로 변경 요청함.
	at_set_apn_form_cgdcont(1, AT_APN_IP_TYPE_IPV4, "lte.ktfwing.com");

	init_gps_manager(); //jwrho
	init_geo_fence(eGEN_FENCE_DEBUG_MODE);

	setting_network_param();

	//at_channel_recovery();
	
	//setting_network_param();
	wait_time_sync();

	// 1. 부팅하면, 기존의 RFID 정보를 무조건 초기화 한다.
	init_passenger_info();
	
	printf("main_loop_callback\r\n");
	// 2. 네트워크가 연결될때까지 기다린다.
	while(flag_run_thread_main && nettool_get_state() != DEFINES_MDS_OK) {
		LOGI(LOG_TARGET, "%s : first time wating untill network enable..\n", __func__);
		sleep(3);
	}
	
	prev_gps_active_time = tools_get_kerneltime();
	
	// 3. 기존의 geofence data 를 open 한다.
	//  - geofence data 는 자동으로 저장하기 때문에 geofence data 를 insert 하지 않는다.
	//  - open 하는 이유는 geofence id 와, total 정보등을 위해서.
	load_file_geofence_info();
	
	// 4. rfid 리더기 uart 를 open 한다.
	init_alloc_rfid_reader();
	
	// 5. rfid 의 정보를 최신화한다.
	//  - 일단, 파일에 저장되어있던 정보를 갖고와서, 서버에 해당 정보가 최신버젼인지 물어본다.
	//  - 최신정보가 아니라면, 서버에서 자동으로 내려준다.
	pkt_send_get_rfid();
	g_rfid_request_flag = 1;

	//pkt_send_get_geofence();
	// TL500 초기화 중
	g_tl500_state = 1;

	buzzer_init();

	while(flag_run_thread_main)
	{
		static time_t ktime_prev_check = 0;
		int ret = get_passenger_info_stat();
		

		printf("flag_run_thread_main\r\n");

		char rfid_id[16] = {0,};
		int rfid_len = 0;

		char command[16] = {0,};
		char buff[32] = {0,};
		int data_len = 0;
		
		if ( (ret == eGET_PASSENGER_STAT_COMPLETE) && (g_rfid_complelte_flag == 0) )
		{
			// print_passenger_info();
			g_rfid_complelte_flag = 1;
		}
		
		// get_alloc_rfid_reader_version(NULL);
		// rfid_len = get_alloc_rfid_reader(rfid_id);
		
		// if ( rfid_len > 0 )
		// 	find_rfid(rfid_id, rfid_len);

		data_len = get_alloc_rfid_reader(command, buff);
		if(data_len > 0)
		{
			if(strcmp(command, "H1") == 0 )
			{
				printf("Version command: %s buff:%s /rfid_len : %d]\n", command, buff, data_len);

				get_alloc_rfid_alivecheck(buff);
				set_alloc_rfid_alivecheck(3);
			}
			else if (strcmp(command, "H5") == 0 )
			{
				get_alloc_rfid_circulating_bus(buff);
				set_alloc_rfid_circulating_bus(3);
			}
			else if (strcmp(command, "H2") == 0 )
			{
				get_alloc_rfid_download_DBAck(buff);
			}
			else if (strcmp(command, "H3") == 0 )
			{
				get_alloc_rfid_tagging(buff, data_len);
				// find_rfid(buff, data_len);
			}
			else if (strcmp(command, "H4") == 0 )
			{
				g_rfid_requestdb++;
				print_red("command : %s, g_rfid_requestdb : %d\n", command,  g_rfid_requestdb);
				set_alloc_rfid_request_DBAck(3);
				pkt_send_get_rfid();
				
			}

		}
		if(at_recov_cnt-- < 0) {
			// jhcho_compile
			//at_channel_recovery();
			at_recov_cnt = 1024;
		}

		time_t current_ktime = tools_get_kerneltime();
		// if system working keep while 48 hours, system will reset in key off status.
		if(current_ktime - ktime_prev_check > 30 &&
			(power_get_ignition_status() == POWER_IGNITION_OFF || current_ktime - prev_gps_active_time >= 1800))
		{
			int remain_mem = 0;

			ktime_prev_check = current_ktime;
			
			remain_mem = tools_get_available_memory();
			if(remain_mem >= 0 && remain_mem < 4000) //4000KB(4MB)
			{
				devel_webdm_send_log("main_loop_callback:Lack of Available memory. %d", remain_mem);
				_process_poweroff("main_loop_callback:Lack of Available memory.");
			}

			LOGI(LOG_TARGET, "check auto reset %d:%d = [%d]sec > [%d] sec\n", current_ktime, system_on_time, (current_ktime-system_on_time), (24 * 3600));

			if(current_ktime - system_on_time > (24 * 3600) ) 
			{
				devel_webdm_send_log("regular poweroff");
				_process_poweroff("main_loop_callback:regular poweroff");
			}
		}
		
		watchdog_process();
		watchdog_set_cur_ktime(eWdMain);
	}
}


void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}
void terminate_app_callback(void)
{
}

static int _process_poweroff(char *log)
{
	tagging_add_list();
	pkt_send_tagging();
	gps_valid_data_write();

	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	poweroff(log, strlen(log));
	
	return 0;
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

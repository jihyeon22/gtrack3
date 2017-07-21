#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <base/config.h>
#include <base/gpstool.h>

#include <base/mileage.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <board/power.h>
#include <board/rfidtool.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/debug.h>
#include <util/stackdump.h>
#include "logd/logd_rpc.h"

#include <netcom.h>
#include <callback.h>
#include <config.h>
#include "cl_mdt_pkt.h"
#include "data-list.h"
#include "geofence.h"
#include "section.h"
#include "routetool.h"

#include <mdsapi/mds_api.h>
#include "board/board_system.h"

#include <at/at_util.h>


#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#include "cl_rfid_tools.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

#define MAKE_PACKET 1
#define ONLY_SEND 0

static int _idle_check_every_sec(int speed, int triger_secs);
static int _make_location_data(gpsData_t *gpsdata, int code);
static int _send_location_data(void);
void _wait_time_sync();
static int _check_device_poweroff();
static int _process_poweroff();

static int flag_run_thread_main = 1;
static int stat_key_on = 0;

static unsigned int avg_speed_total_val = 0;
static unsigned int avg_speed_total_num = 0;

static int system_on_time = 0;
static time_t prev_gps_active_time = 0;

static pthread_mutex_t avg_speed_mutex = PTHREAD_MUTEX_INITIALIZER ;

// ---------------------------------------------
RFID_DEV_INFO_T g_rfid_dev_info;
// ---------------------------------------------
// #define USE_CL_THERMAL_SENSOR

#ifdef USE_CL_THERMAL_SENSOR
#include "base/thermtool.h"
#define THERMAL_SENSING_CYCLE 	1
#define THERMAL_DEVICE			"/dev/ttyHSL2"
#endif

#define CALC_DISTANCE_INTERVAL_SEC  10

void abort_callback(void)
{
	save_geo_fence_status_info();
}

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();
	thread_network_set_warn_timeout(conf->model.interval_time * 2);

	stackdump_abort_callback = abort_callback;
	init_geo_fence(eGEN_FENCE_DEBUG_MODE);
	section_setup_from_config();

	// set_rout_table_port_forward("smd29","ppp1");

#ifdef USE_CL_THERMAL_SENSOR
	// thermal senser // not use thermal sensor
	therm_set_sense_cycle(THERMAL_SENSING_CYCLE);
	therm_set_dev(THERMAL_DEVICE, strlen(THERMAL_DEVICE));
#endif

	init_kjtec_rfid();
	
	rfid_tool__set_senario_stat(e_RFID_INIT); 
	//rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_START);
}

void network_on_callback(void)
{
	devel_webdm_send_log("start %u %u\n", mileage_get_m(), (unsigned int)tools_get_kerneltime());
}

void button1_callback(void)
{
	char code= CL_CAR_ACCIDENT_BTN_EVENT_CODE;
	gpsData_t gpsdata;
	
	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

	//kjtec_rfid_mgr__clr_all_user_data();
	//rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_START);
}

void button2_callback(void)
{
	char code= CL_CAR_BROKEN_BTN_EVENT_CODE;
	gpsData_t gpsdata;
	
	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

	//kjtec_rfid_mgr__clr_all_user_data();
	//rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_START);

}

void ignition_on_callback(void)
{
	char code= CL_ACC_ON_EVENT_CODE;
	gpsData_t gpsdata;
	
	stat_key_on = 1;

	_wait_time_sync();

	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

//	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);
}

void ignition_off_callback(void)
{
	char code= CL_ACC_OFF_EVENT_CODE;
	gpsData_t gpsdata;
	
	stat_key_on = 0;

	_wait_time_sync();

	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

//	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);

}

void power_on_callback(void)
{
	char code= CL_MODEM_ON_EVENT_CODE;
	gpsData_t gpsdata;

	_wait_time_sync();

	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();
	
//	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);
}

void power_off_callback(void)
{
	char code = CL_MODEM_OFF_EVENT_CODE;
	gpsData_t gpsdata;

	_wait_time_sync();

	if(stat_key_on == 1 && power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		ignition_off_callback();
	}

	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

//	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);
	_process_poweroff("power_off_callback");
}

static unsigned int cycle_collect = 0;
static unsigned int cycle_report = 0;
void gps_parse_one_context_callback(void)
{
	static int flag_fisrt_gps = 0;

	static time_t last_gps_utc_sec = 0;
	time_t cur_gps_utc_sec = 0;

	configurationModel_t *conf = get_config_model();
	int interval_time = conf->model.interval_time;
	int max_packet = conf->model.max_packet;
	gpsData_t gpsdata;
	int speed = 0;
	fence_notification_t fnoti;
	int fence_num = -1;
	fence_notification_t fence_status = eFENCE_NONE_NOTIFICATION;

	gps_get_curr_data(&gpsdata);

	// debug info
	cur_gps_utc_sec = gpsdata.utc_sec;

	if ( cur_gps_utc_sec - last_gps_utc_sec >= 2)
	{
		char debug_msg_buff[128] = {0,};
		sprintf(debug_msg_buff,"GPS TIME DIFF [%d]sec", (cur_gps_utc_sec - last_gps_utc_sec));
		LOGE(LOG_TARGET, "%s\n", debug_msg_buff);
		//mds_api_write_time_and_log("/system/mds/log/gps_time.log", debug_msg_buff);
	}
	
	last_gps_utc_sec = cur_gps_utc_sec;

	if(gpsdata.year < 2014)
	{
		return;
	}

	if(gpsdata.active == 1)
	{
		if(gpsdata.speed > 0)
		{
			prev_gps_active_time = tools_get_kerneltime();
		}
	
		if(flag_fisrt_gps == 0)
		{
			int code = CL_GPS_ON_EVENT_CODE;
			
			flag_fisrt_gps = 1;

			_make_location_data(&gpsdata, code);
			_send_location_data();
			//sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);		

			devel_webdm_send_log("GPS ACTIVATE SUCCESS!");

		}
		speed = gpsdata.speed;

		fnoti = get_geofence_notification(&fence_num, gpsdata);
		if(fnoti != eFENCE_NONE_NOTIFICATION)
		{
			if(fnoti == eFENCE_IN_NOTIFICATION)
			{
				fence_status = eFENCE_IN_NOTIFICATION;
				_make_location_data(&gpsdata, CL_ZONE_IN_EVENT_CODE);
				_send_location_data();
			}
			else if(fnoti == eFENCE_OUT_NOTIFICATION)
			{
				fence_status = eFENCE_OUT_NOTIFICATION;
				_make_location_data(&gpsdata, CL_ZONE_OUT_EVENT_CODE);
				_send_location_data();
			}
		}
	}
	else
	{
		speed = -1;
	}

	if(_idle_check_every_sec(speed, conf->model.stop_time) == 1)
	{
		_make_location_data(&gpsdata, CL_STOP_WRN_EVENT_CODE);
		
		if(fence_status != eFENCE_IN_NOTIFICATION){
			_send_location_data();
		}
	}

	if(section_check(&gpsdata) == 1)
	{
		_make_location_data(&gpsdata, CL_AREA_EVENT_CODE);
		_send_location_data();
		return;
	}

	if(max_packet > 0 && interval_time >0 && ++cycle_collect >= (interval_time / max_packet))
	{
		_make_location_data(&gpsdata, CL_CYCLE_EVENT_CODE);
	}
	
	if(++cycle_report >= interval_time)
	{
		_send_location_data();
	}

	// mileage calc test
	if (cycle_report % CALC_DISTANCE_INTERVAL_SEC == 0 )
	{
		//printf("calc distance!!\r\n");
		mileage_process(&gpsdata);
	}

}

#define GPS_ANT_CHK_INTERVAL_SEC		10

#define GET_THERMAL_SENSOR_INTERVAL_SEC	60
#define GET_RFID_USER_INTERVAL_SEC		60

#define KJTEC_CONN_DISCONN_CHK_CNT		3
#define RFID_CHK_INTERVAL				60
#define MAX_CHK_RFID_WRITE_FAIL_CNT 	3

#define MAIN_STAT_MSG_PRINT_INTERVAL 	5

void main_loop_callback(void)
{

	static int main_loop_cnt = 0;
	int main_rfid_chk_cnt = 0;

	system_on_time = tools_get_kerneltime();
	
	static int last_gps_ant_stat = -1;

	int rfid_read_fail_cnt = 0;
	int rfid_write_user_data_fail_cnt = 0;
	// rfid_init();
	int need_to_rfid_info = 0;

	memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );

	if (0)
	{
		RFID_FIRMWARE_VER_T ver_info;
		if ( kjtec_rfid__firmware_ver_info(&ver_info) == KJTEC_RFID_RET_SUCCESS )
			LOGI(LOG_TARGET, "[MAIN] kjtec version info [%s]\n", ver_info.data_result  );
		else
			LOGE(LOG_TARGET, "[MAIN] kjtec version info fail\n");
	}

	while(flag_run_thread_main)
	{
		static int count_run_watchdog = 0;
		if(count_run_watchdog++ > 60)
		{
			count_run_watchdog = 0;
			watchdog_process();
			watchdog_set_cur_ktime(eWdMain);
			_check_device_poweroff();			
		}

		if ( ( main_loop_cnt % GPS_ANT_CHK_INTERVAL_SEC ) == 0 )
		{
			int cur_gps_ant_stat = mds_api_gps_util_get_gps_ant();

			if ( cur_gps_ant_stat != last_gps_ant_stat)
			{
				devel_webdm_send_log("GPS ANT STAT [%d]", cur_gps_ant_stat);
			}
			last_gps_ant_stat = cur_gps_ant_stat;
		}

#ifdef USE_CL_THERMAL_SENSOR
		if ( ( main_loop_cnt % GET_THERMAL_SENSOR_INTERVAL_SEC ) == 0 )
			therm_sense();
#endif

		// ------------------------------------------------------
		// 시나리오시작
		// -----------------------------------------------------
		if ( ( main_loop_cnt % MAIN_STAT_MSG_PRINT_INTERVAL ) == 0 )
			LOGI(LOG_TARGET, "[MAIN] pkt_stat [%s], rfid chk [%d]/[%d] \n", rfid_tool__get_senario_stat_str(), main_rfid_chk_cnt, rfid_tool__env_get_rfid_chk_interval()  );

		// 1. rfid 단말을 확인한다.
		if ( rfid_tool__get_senario_stat() == e_RFID_INIT )
		{
			need_to_rfid_info = 0;

			if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
			{
				rfid_tool__set_senario_stat(e_NEED_TO_RFID_USER_CHK);
				LOGE(LOG_TARGET, "[MAIN] RFID INIT success\r\n");
				devel_webdm_send_log("KJTEC CONN : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
				rfid_read_fail_cnt = 0;
				rfid_write_user_data_fail_cnt = 0;
				rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
			}
			else
			{
				rfid_tool__set_senario_stat(e_RFID_INIT);
				LOGE(LOG_TARGET, "[MAIN] RFID INIT FAIL [%d]/[%d]\r\n",rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
				// 너무 자주보내면안됨.
				if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
					devel_webdm_send_log("KJTEC CONN FAIL!! 1 ");
				rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
			}
		}

		// 2. 승객리스트를 웹에서 받아온다.
		if ( rfid_tool__get_senario_stat() == e_NEED_TO_RFID_USER_CHK )
		{
			rfid_write_user_data_fail_cnt = 0;
			need_to_rfid_info = 0;

			if ( rfid_tool__env_get_all_clear() == 1 )
				sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, "0", ePIPE_2);
			else
				sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, g_rfid_dev_info.saved_timestamp, ePIPE_2);

		}

		// 3. 모두받아왔다면, 승객을 rfid 에 넣는다.
		if ( rfid_tool__get_senario_stat() == e_RFID_DOWNLOAD_END )
		{
			int ret = kjtec_rfid_mgr__write_user_info(); // block! // 한참걸릴수있다.

			// 다운로드 성공했으니, 새로운 정보로 갱신한다.
			if ( ret == KJTEC_RFID_RET_SUCCESS )
			{
				// 새로 얻어와야 하기때문에.
				memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
				
				rfid_write_user_data_fail_cnt = 0;
				if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
				{
					need_to_rfid_info = 0;
					devel_webdm_send_log("[MAIN] USER DOWN OK 1 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
				}
				else
				{
					need_to_rfid_info = 1;
					devel_webdm_send_log("[MAIN] USER DOWN OK 2 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
				}
			}
			else
			{
				LOGE(LOG_TARGET, "[MAIN] RFID WRITE FAIL [%d]/[%d]\r\n",rfid_write_user_data_fail_cnt, MAX_CHK_RFID_WRITE_FAIL_CNT);

				if ( rfid_write_user_data_fail_cnt++ > MAX_CHK_RFID_WRITE_FAIL_CNT ) 
				{
					devel_webdm_send_log("USER DATA WRITE FAIL!!");
					rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_FAIL);
				}
			}
		}

		if ( need_to_rfid_info == 1 )
		{
			if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
			{
				devel_webdm_send_log("[MAIN] RFID DEV INFO : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
				need_to_rfid_info = 0;
			}
		}

		// 4. 모두 write도 성공했으니, 승객정보를 주기적으로 요청한다.
		if ( rfid_tool__get_senario_stat() == e_RFID_USER_INFO_CHK_READY ) 
		{
			if ( main_rfid_chk_cnt > rfid_tool__env_get_rfid_chk_interval() )
			{
				kjtec_rfid__dev_rfid_req();
				main_rfid_chk_cnt = 0;
			}
		}
	
		// rfid 살아있는지, 연결되어있는지 체크한다.
		if ( ( main_loop_cnt % RFID_CHK_INTERVAL ) == 0 )
		{
			// 정상동작중에만 체크한다.
			if ( ( rfid_tool__get_senario_stat() > e_RFID_INIT ) && ( rfid_tool__get_senario_stat() <= e_RFID_USER_INFO_CHK_READY ) )
			{
				int dev_stat = kjtec_rfid_mgr__alive_dev();

				// 연결 해제 됐다가, 다시 연결된경우.
				if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt > KJTEC_CONN_DISCONN_CHK_CNT ) )
				{
					rfid_read_fail_cnt = 0;
					rfid_tool__set_senario_stat(e_RFID_INIT);
					LOGI(LOG_TARGET, "[CONN CHK] KJTEC DISCONN -> CONN !!\r\n");
					devel_webdm_send_log("[CONN CHK] KJTEC DISCONN -> CONN !!");

					rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
				}
				// 정상연결중
				else if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt <= KJTEC_CONN_DISCONN_CHK_CNT ) )
				{
					rfid_read_fail_cnt = 0;
					LOGI(LOG_TARGET, "[CONN CHK] KJTEC CONN -> NORMAL STAT !!\r\n");

					rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
				}
				// 연결이상.
				else
				{
					LOGE(LOG_TARGET, "KJTEC CONN FAIL [%d]/[%d]\n", rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
					if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
						devel_webdm_send_log("[CONN CHK] KJTEC CONN FAIL!! 2 ");
					
					rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
				}
			}
		}
		
		/*
		if ( rfid_tool__get_senario_stat() == e_RFID_FIRMWARE_DOWNLOAD_START )
			kjtec_rfid_mgr__download_fw("/system/rfid_fw.bin");
*/
		main_loop_cnt++;
		main_rfid_chk_cnt++;
		sleep(1);
	}

#ifdef USE_CL_THERMAL_SENSOR
	rfid_deinit();
#endif

}

void terminate_app_callback(void)
{
	save_geo_fence_status_info();
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}

int get_key_stat(void)
{
	return stat_key_on;
}

void avg_speed_add_data(int speed)
{
	pthread_mutex_lock(&avg_speed_mutex);
	
	avg_speed_total_val += speed;
	avg_speed_total_num++;	

	pthread_mutex_unlock(&avg_speed_mutex);
}

void avg_speed_clear(void)
{
	pthread_mutex_lock(&avg_speed_mutex);
	
	avg_speed_total_val = 0;
	avg_speed_total_num = 0;

	pthread_mutex_unlock(&avg_speed_mutex);
}

int avg_speed_get(void)
{
	int res = 0;

	pthread_mutex_lock(&avg_speed_mutex);
	
	if(avg_speed_total_num !=0)
	{
		res = avg_speed_total_val / avg_speed_total_num;
	}

	pthread_mutex_unlock(&avg_speed_mutex);

	return res;
}

static int _idle_check_every_sec(int speed, int triger_secs)
{
	static unsigned int idle_secs = 0;
	static int is_trigered = 0;

	if(triger_secs <= 0)
	{
		return 0;
	}

	if(speed < 0)
	{
		idle_secs = 0;
		return 0;
	}
	
	if(speed > 0)
	{
		idle_secs = 0;
		is_trigered = 0;
		return 0;
	}

	if(is_trigered == 1)
	{
		return 0;
	}

	idle_secs++;

	if(idle_secs < triger_secs)
	{
		return 0;
	}

	is_trigered = 1;
	return 1;
}

static int _make_location_data(gpsData_t *gpsdata, int code)
{
	locationData_t *data = NULL;

	cycle_collect = 0;
	
	data = malloc(sizeof(locationData_t));
	if(data == NULL)
	{
		LOGE(LOG_TARGET, "%s : malloc fail\n", __FUNCTION__);
		return -1;
	}

	memcpy(&(data->gpsdata), gpsdata, sizeof(gpsData_t));
	data->acc_status = stat_key_on;
	data->event_code = code;
	data->mileage_m = 0;
	data->avg_speed = 0;
	
	if(list_add(&packet_list, (void *)data) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
		free(data);
		return -1;
	}

	return 0;
}

static int _send_location_data(void)
{
	int pkt_type = -1;

	cycle_report = 0;
	section_clear_acc_dist();
	
#ifdef USE_CL_THERMAL_SENSOR
	pkt_type = CL_PKT_TYPE_THERMAL_REPORT;
	sender_add_data_to_buffer(PACKET_TYPE_REPORT, &pkt_type, ePIPE_1);
#else
	pkt_type = CL_PKT_TYPE_NORMAL_REPORT;
	sender_add_data_to_buffer(PACKET_TYPE_REPORT, &pkt_type, ePIPE_1);
#endif
	
	return 0;
}


void _wait_time_sync()
{
	gpsData_t cur_gpsdata;

	while(1) {
		gps_get_curr_data(&cur_gpsdata);
		if(cur_gpsdata.year > 2013)
			break;
		
		sleep(1);
		
		if ( power_get_power_source() == POWER_SRC_BATTERY)
			return;

		printf("wait_time_sync : year[%d]\n", cur_gpsdata.year);
		LOGI(LOG_TARGET, "wait_time_sync : year[%d]\n", cur_gpsdata.year);
	}
}

static int _check_device_poweroff(void)
{
	time_t current_time = 0;
	int ign_status = 0;

	current_time = tools_get_kerneltime();
	ign_status = power_get_ignition_status();

	// if system working keep while 48 hours, system will reset in key off status.
	if(ign_status == POWER_IGNITION_OFF || current_time - prev_gps_active_time >= 1800)
	{
		int remain_mem = 0;
		
		remain_mem = tools_get_available_memory();
		if(remain_mem >= 0 && remain_mem < 4000) //4000KB(4MB) 
		{
			devel_webdm_send_log("Warn : Lack of Available memory.");
			_process_poweroff("main_loop_callback:leak mem");
		}

		LOGI(LOG_TARGET, "check auto reset %d:%d = [%d]sec > [%d] sec\n", current_time, system_on_time, (current_time-system_on_time), (24 * 3600));
		if(current_time - system_on_time > (24 * 3600) ) 
		{
			devel_webdm_send_log("regular poweroff");
			_process_poweroff("main_loop_callback:regular poweroff");
		}
	}

	return 0;
}

static int _process_poweroff(char * log)
{
	save_geo_fence_status_info();

	devel_webdm_send_log("Accumulate distance : %um at the end\n", mileage_get_m());
	
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	
	poweroff(log, strlen(log));

	return 0;
}

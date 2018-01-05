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
#include "cl_rfid_senario.h"
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

// #define USE_CL_THERMAL_SENSOR

#ifdef USE_CL_THERMAL_SENSOR
#include "base/thermtool.h"
#define THERMAL_SENSING_CYCLE 	1
#define THERMAL_DEVICE			"/dev/ttyHSL2"
#endif

//#define DEBUG_INFO_ALIVE_SEND_TO_DM
#ifdef DEBUG_INFO_ALIVE_SEND_TO_DM
#define SEND_ALIVE_INTERVAL_SEC		    600
static unsigned int _debug_gps_parse_cnt = 0;
static unsigned int _debug_main_loop_cnt = 0;
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

//	kjtec_rfid_mgr__clr_all_user_data();
	//kjtec_rfid_mgr__download_sms_noti_enable(1, "01086687577");
//	rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_START);

}

void button2_callback(void)
{
	char code= CL_CAR_BROKEN_BTN_EVENT_CODE;
	gpsData_t gpsdata;
	
	gps_get_curr_data(&gpsdata);
	
	_make_location_data(&gpsdata, code);
	_send_location_data();

//	kjtec_rfid_mgr__clr_all_user_data();
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
//	kjtec_rfid_mgr__clr_all_user_data();
//	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &code, ePIPE_2);

	rfid_tool__set_senario_stat(e_RFID_INIT); 
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
    static int gps_time_invalid_cnt = 0;

	gps_get_curr_data(&gpsdata);

    chk_runtime_network_chk();

	if ( gps_chk_valid_time(&gpsdata) <= 0 )
    {
        gps_time_invalid_cnt++;
        if ( gps_time_invalid_cnt > 20 )
        {
            devel_webdm_send_log("gps time time is invalid case 1");
            gps_time_invalid_cnt = 0;
        }
		return;
    }

    gps_time_invalid_cnt = 0;

#ifdef DEBUG_INFO_ALIVE_SEND_TO_DM
		if ( ( _debug_gps_parse_cnt ++ % SEND_ALIVE_INTERVAL_SEC ) == 0 )
		{
			devel_webdm_send_log("I AM ALIVE - 2 [%d] / [%d]", _debug_main_loop_cnt, _debug_gps_parse_cnt);
		}
#endif

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
        devel_webdm_send_log("gps time time is invalid case 2");
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


int clear_main_watchdog()
{
	watchdog_set_cur_ktime(eWdMain);
	return 0;
}

void main_loop_callback(void)
{

	static int main_loop_cnt = 0;
	

	system_on_time = tools_get_kerneltime();
	
	static int last_gps_ant_stat = -1;

	// rfid_init();
	rfid_main_senario_init();


	while(flag_run_thread_main)
	{
		static int count_run_watchdog = 0;
		if(count_run_watchdog++ > 60)
		{
			count_run_watchdog = 0;
			watchdog_process();
			clear_main_watchdog();
			_check_device_poweroff();
		}

		chk_runtime_network_chk();
		
#ifdef DEBUG_INFO_ALIVE_SEND_TO_DM
		if ( ( _debug_main_loop_cnt++ % SEND_ALIVE_INTERVAL_SEC ) == 0 )
		{
			devel_webdm_send_log("I AM ALIVE -1 [%d] / [%d]", _debug_main_loop_cnt, _debug_gps_parse_cnt);
		}
#endif

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

        // main src dc check.
        if ( power_get_power_source() == POWER_SRC_DC )
        {
            rfid_main_senario();
        }
	
		main_loop_cnt++;
		
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
    int max_time_sync_sec = 20;
    // do nothing...

	while(max_time_sync_sec--) {
		gps_get_curr_data(&cur_gpsdata);
		if(cur_gpsdata.year > 2013)
			break;
        sleep(1);
	}

    if ( max_time_sync_sec <= 0 )
        devel_webdm_send_log("_wait_time_sync() err");
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

void network_fail_emergency_reset_callback(void)
{
    //_process_poweroff("network chk fail");
    save_geo_fence_status_info();
    poweroff(NULL,0);
}

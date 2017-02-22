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
#include "packet.h"
#include "data-list.h"
#include "geofence.h"
#include "section.h"
#include "routetool.h"

#include "board/board_system.h"

#include "base/thermtool.h"

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

#define THERMAL_SENSING_CYCLE 	1
#define THERMAL_DEVICE			"/dev/ttyHSL2"

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

	// thermal senser
	therm_set_sense_cycle(THERMAL_SENSING_CYCLE);
	therm_set_dev(THERMAL_DEVICE, strlen(THERMAL_DEVICE));

}

void network_on_callback(void)
{
	devel_webdm_send_log("start %u %u\n", mileage_get_m(), (unsigned int)tools_get_kerneltime());
}

void button1_callback(void)
{
}

void button2_callback(void)
{
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

	configurationModel_t *conf = get_config_model();
	int interval_time = conf->model.interval_time;
	int max_packet = conf->model.max_packet;
	gpsData_t gpsdata;
	int speed = 0;
	fence_notification_t fnoti;
	int fence_num = -1;
	fence_notification_t fence_status = eFENCE_NONE_NOTIFICATION;

	gps_get_curr_data(&gpsdata);

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

void main_loop_callback(void)
{
	static unsigned char uid_prev[10] = {0};
	int len_uid = 0;
	int detach_card = 1;

	system_on_time = tools_get_kerneltime();
	
	// rfid_init();

	therm_sense();

	while(flag_run_thread_main)
	{
		unsigned char uid[10] = {0};

		static int count_run_watchdog = 0;
		if(count_run_watchdog++ > 60)
		{
			count_run_watchdog = 0;
			watchdog_process();
			watchdog_set_cur_ktime(eWdMain);
			_check_device_poweroff();			
		}
		
		#if 0
		therm_sense();
		// get_modem_time_utc_sec();
		{
			int rec_len = 0;
			THERMORMETER_DATA tmp_therm;

			if(therm_get_curr_data(&tmp_therm) == 0)
			{
				int i = 0;

				for(i=0 ; i < tmp_therm.channel; i++)
				{
					switch(tmp_therm.temper[i].status)
					{
						case eOK:
							printf(" tmp data is [%d]\r\n", tmp_therm.temper[i].data);
							break;

						case eOPEN:

							break;

						case eSHORT:

							break;

						case eUNUSED:
							break;
						case eNOK:
							break;
						default:
							break;
					}

				}
			}
			else 
				printf(" tmp data read fail\r\n");

		}
		#endif
		/* // rfid is not support 
		usleep(800000);
		rfid_clear_uart();
		if((len_uid = rfid_find_card(uid)) > 0)
		{
			debug_hexdump_buff(uid, len_uid);
	
			if(memcmp(uid, uid_prev, len_uid) == 0)
			{
				if(detach_card == 0)
					continue;
			}
			else
			{
				memcpy(uid_prev, uid, len_uid);
			}
			detach_card = 0;			

			int res_check_rfid = rfid_check_passenser(uid, len_uid);

			if(res_check_rfid < 0)
			{
				rfid_beep(2,2,100000);
				rfid_beep(2,2,100000);

				continue;
			}
			else if(res_check_rfid == 0)
			{
				continue;
			}
			else
			{
				rfidData_t rfid =
				{
					.uid = {0},
					.len_uid = 0,
					.boarding = RFID_GET_ON
				};

				rfid_beep(5,6,200000);

				memcpy(rfid.uid, uid, len_uid);
				rfid.len_uid = len_uid;

				if(res_check_rfid == RFID_GET_ON)
					rfid.boarding = RFID_GET_ON;
				else
					rfid.boarding = RFID_GET_OFF;

				sender_add_data_to_buffer(PACKET_TYPE_RFID, &rfid, ePIPE_1);
			}
		}
		else
		{
			detach_card = 1;
		}
		*/
		sleep(1);
	}

	rfid_deinit();
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
	cycle_report = 0;
	section_clear_acc_dist();
	
	sender_add_data_to_buffer(PACKET_TYPE_REPORT, NULL, ePIPE_1);
	
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

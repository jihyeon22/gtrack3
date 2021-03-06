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
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"

#include <board/board_system.h>

#include <nisso_mdt800/packet.h>
#include <nisso_mdt800/gpsmng.h>
#include <nisso_mdt800/gps_utill.h>
#include <nisso_mdt800/geofence.h>
#include <nisso_mdt800/file_mileage.h>

void abort_callback(void);

//static void _check_battery(int ignition);
static int _process_poweroff(int now_poweroff_flag, char *log);
static int flag_run_thread_main = 1;

int is_ignition_off = 1; // initial status is key off

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

void abort_callback(void)
{
	write_vaild_data();
}

void init_model_callback(void)
{
	int mileage = 0;
	configurationModel_t *conf = get_config_model();

	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	
	printf("%s ++\n", __func__);
	stackdump_abort_callback = abort_callback; 
	LOGI(LOG_TARGET, "init_model_callback\n");	
	load_mileage_file(&mileage);
	set_server_mileage(mileage);
    init_nisso_pkt__invoice_info_2();
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

int geofence_test = 0;

void button1_callback(void)
{
    configurationModel_t *conf_model = NULL;

    printf("%s ++\n", __func__);
    LOGI(LOG_TARGET, "button1_callback ++\n");

    conf_model = get_config_model();
    // only key on send 
	if ( is_ignition_off == 0 )
    	sender_add_data_to_buffer(eBUTTON_NUM0_EVT, NULL, ePIPE_1);

#ifdef FEATURE_GEO_FENCE_SIMULATION
    //parse_model_sms("0000", "000111122222", "&12,1, 0,2,37.4756163,126.8818914,60 ,1,2,00.0000000,000.0000000,0 ,2,2,00.0000000,000.0000000,0,3,2,37.399693, 127.100937,10,1");
    //parse_model_sms("0000", "000111122222", "&12,2,3,2,37.4836196,126.8798599,150,4,2,37.4836196,126.8798599,70,1");
    geo_test_flag = 0; //geo out
#endif
    //test_gps_func();
}

void button2_callback(void)
{
    configurationModel_t *conf_model = NULL;

    printf("%s ++\n", __func__);
    LOGI(LOG_TARGET, "button2_callback ++\n");

    conf_model = get_config_model();

    if ( is_ignition_off == 0 )
        sender_add_data_to_buffer(eBUTTON_NUM1_EVT, NULL, ePIPE_1);

#ifdef FEATURE_GEO_FENCE_SIMULATION
    geo_test_flag = 1; //geo in
#endif
    //test_gps_func();
}

void ignition_on_callback(void)
{
    configurationModel_t *conf = get_config_model();

    // check key pair..
    if ( is_ignition_off == 0 )
    {
        devel_webdm_send_log("already key on skip evt proc");
        return;
    }

    // devel_webdm_send_log("normal key on sernaio");

    is_ignition_off = 0;

    printf("%s ++\n", __func__);
    LOGI(LOG_TARGET, "ignition_on_callback ++\n");
    
    init_gps_manager(); //jwrho

    if(conf->model.tempature_enable == 1)
    {
        therm_clear_fault();
        therm_set_sense_cycle(conf->model.tempature_cycle);
    }
    
    nisso_btn_mgr__init();
    
    wait_time_sync();

    sender_add_data_to_buffer(eIGN_ON_EVT, NULL, ePIPE_1);

    //parse_model_sms("0000", "000111122222", "&12,1, 0,2,37.4756163,126.8818914,60 ,1,2,00.0000000,000.0000000,0 ,2,2,00.0000000,000.0000000,0,3,2,37.399693, 127.100937,11,1");

}

void ignition_off_callback(void)
{
    configurationModel_t *conf = get_config_model();

    printf("%s ++\n", __func__);
    LOGI(LOG_TARGET, "ignition_off_callback ++\n");

    if(conf->model.tempature_enable == 1)
    {
        therm_set_sense_cycle(conf->model.tempature_cycle*3);
    }
    
    wait_time_sync();

    // check key pair..
    if ( is_ignition_off == 1 )
    {
        write_vaild_data();
        devel_webdm_send_log("already key off skip evt proc");
        return;
    }
    else
    {
        // devel_webdm_send_log("normal key off sernaio");

        nisso_btn_mgr__init();

        //TODO : last gps pos file save for correcting more gen-fence
        sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
        is_ignition_off = 1;
        sender_add_data_to_buffer(eIGN_OFF_EVT, NULL, ePIPE_1);

        sender_add_data_to_buffer(eCYCLE_REPORT_EVC, NULL, ePIPE_1);
        write_vaild_data();
    }

}

void power_on_callback(void)
{
	printf("%s ++\n", __func__);

    set_nisso_pkt__external_pwr(EXTERNAL_PWR_ON);

	LOGI(LOG_TARGET, "power_on_callback ++\n");
}

void power_off_callback(void)
{
    int batt = 0;

    printf("%s ++\n", __func__);

    set_nisso_pkt__external_pwr(EXTERNAL_PWR_OFF);

    if(is_ignition_off == 0 && power_get_ignition_status() == POWER_IGNITION_OFF)
    {
        devel_webdm_send_log("key off sync fail : force key off");
        ignition_off_callback();
    }

    LOGI(LOG_TARGET, "power_off_callback ++\n");

    // devel_webdm_send_log("normal power off sernaio");

    sender_add_data_to_buffer(ePOWER_SOURCE_CHANGE_EVT, NULL, ePIPE_1);
    // sender_add_data_to_buffer(eMDM_DEV_RESET, NULL, ePIPE_1); // remove..

    batt = battery_get_battlevel_internal();
    if(batt > 0 && batt < 3600)
    {
        devel_webdm_send_log("Warn : Internal battery is low - %d(mV)\n", batt);
    }

    //	if(g_dtg_mode_enable != 1)
        _process_poweroff(1, "power_off_callback");
}

//#define GPS_WEBDM_VERBOS	
#define MAX_CHK_GPS_DEACTIVE_CNT	60

void gps_parse_one_context_callback(void)
{
	gps_condition_t ret = eUNKNOWN_GPS_DATA;
	gpsData_t cur_gpsdata;
	gpsData_t gpsdata;

	configurationModel_t * conf = get_config_model();

	static int first_gps_active = 0;
	static int gps_cnt_active = 0;
	static int gps_cnt_deactive = 0;
	static int gps_debug_msg = 1;

    static int gps_fail_cnt = 0;
	////////////////////////////////////////////////////////////////////////
	// 1. gps packet gathering and filtering
	////////////////////////////////////////////////////////////////////////
	gps_get_curr_data(&cur_gpsdata);
    active_gps_process_force_routine(cur_gpsdata);

	mileage_set_m(get_server_mileage() + get_gps_mileage());
	
	if ( gps_chk_valid_time(&cur_gpsdata) <= 0 )
    {
        LOGI(LOG_TARGET, "gpsthread : invalid gps time?\r\n");
		return;
    }

    // key off �϶��� �ƹ��� ������ ���� �ʰ��Ѵ�. 
    // nisso ��û����
    if ( is_ignition_off == 1 )
    {
        int data_cnt = get_gps_data_count();
        int i = 0;
        LOGI(LOG_TARGET, "gpsthread : key off : do nothing.. [%d]\r\n", data_cnt);

        for ( i = 0 ; i < data_cnt ; i++ )
        {
            nisso_packet2_t *p_packet2 = NULL;

            LOGI(LOG_TARGET, "gpsthread : key off : free cont [%d]\r\n", i);
            if(list_pop(&gps_buffer_list, (void *)&p_packet2) < 0)
                break;
            free(p_packet2);
        }

        gps_fail_cnt = 0;

        return;
    }

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
            // gps deactivate 5 min ==> cold reset..
            if ( gps_fail_cnt++ > 300 )
            {
                devel_webdm_send_log("gps active fail.. cold boot");
                gps_reset_immediately(GPS_BOOT_COLD);
                gps_fail_cnt = 0;
            }
			break;
		case eACTIVE:
			if(cur_gpsdata.speed > 0)
				prev_gps_active_time = tools_get_kerneltime();
			ret = active_gps_process(cur_gpsdata, &gpsdata);
			break;
	}

	if(ret != eUSE_GPS_DATA) {
		print_skip_gps_reason(ret, cur_gpsdata);
		return;
	}

	LOGD(LOG_TARGET, "cur_gpsdata.active = [%d], this gps condition[%d]\n", cur_gpsdata.active, ret);

	LOGD(LOG_TARGET, "gps local time and date: %s\n", asctime(localtime(&gpsdata.utc_sec)) );	

	{
		nisso_packet2_t *p_packet2;
		p_packet2 = (nisso_packet2_t *)malloc(sizeof(nisso_packet2_t));
		if(p_packet2 == NULL) {
			LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
			return;
		}


		create_report2_data(eCYCLE_REPORT_EVC, p_packet2, gpsdata, NULL, 0);
		if(list_add(&gps_buffer_list, p_packet2) < 0)
		{
			LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
			free(p_packet2);
		}

	}

	LOGI(LOG_TARGET, "current report packet buffering count[%d]\n", get_gps_data_count());
	return;
}

#define GPS_ANT_CHK_INTERVAL_SEC	2
#define MAX_GPS_CHK_CNT_MSG_DM		60

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

	int main_loop_cnt = 0;
	static int last_gps_ant_stat = -1;
	
	static int gps_ant_chk_cnt = 0;
	static int gps_ant_send_msg = 0;

	init_gps_manager(); //jwrho
	init_geo_fence();
	
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

	if(conf->model.tempature_enable == 1)
	{
		therm_sense();
	}

	while(flag_run_thread_main)
	{
		int condition_send = 0;
		

		watchdog_set_cur_ktime(eWdMain);
		
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
		
		current_time = tools_get_kerneltime();
		ign_status = power_get_ignition_status();

		//_check_battery(ign_status);

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

		if ( ( main_loop_cnt % GPS_ANT_CHK_INTERVAL_SEC ) == 0 )
		{
			int cur_gps_ant_stat = mds_api_gps_util_get_gps_ant();

			if ( cur_gps_ant_stat != last_gps_ant_stat)
			{
				gps_ant_chk_cnt++;
				// devel_webdm_send_log("GPS ANT STAT [%d]", cur_gps_ant_stat);
				LOGI(LOG_TARGET, "GPS ANT STAT [%d] / [%d]", cur_gps_ant_stat, gps_ant_chk_cnt);
			}

			if (( gps_ant_chk_cnt > MAX_GPS_CHK_CNT_MSG_DM ) && (gps_ant_send_msg == 0))
			{
				gps_ant_send_msg = 1;
				devel_webdm_send_log("GPS ANT STAT IS INVAILD");
			}

			last_gps_ant_stat = cur_gps_ant_stat;
		}

		main_loop_cnt++;

		sleep(5);
	}
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}

void terminate_app_callback(void)
{
	write_vaild_data();
}

static int _process_poweroff(int now_poweroff_flag, char *log)
{
//	static int poweroff_count = 0;
	char smsmsg[100] = {0,};

//	if(poweroff_count++ > 5 || now_poweroff_flag == 1)
	{
		write_vaild_data();

		sprintf(smsmsg, "Accumulate distance : %um at the end\n", get_gps_mileage());
		devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		devel_webdm_send_log(smsmsg);
		
//		if(poweroff_count > 5)
		if(now_poweroff_flag == 0)
		{
			//devel_webdm_send_log("regular poweroff %d", poweroff_count);
			devel_webdm_send_log("regular poweroff");
		}

		sender_wait_empty_network(20);
		poweroff(log, strlen(log));
	}
//	else
//	{
//		system("echo 3 > /proc/sys/vm/drop_caches &");
//	}

	return 0;
}


/*
#define CYCLE_CHECK_BATT_DATA 60
#define CYCLE_SEND_BATT_DATA 3600
static void _check_battery(int ignition)
{
	static time_t prev_time_chk_batt = 0;
	static time_t prev_time_send = 0;
	static int prev_car_volt = 0;

	int cur_car_volt = 0;
	time_t cur_ktime = 0;

	if(ignition)
	{
		return;
	}

	cur_ktime = tools_get_kerneltime();

	if(prev_time_send == 0 || cur_ktime - prev_time_send > CYCLE_SEND_BATT_DATA)
	{
		cur_car_volt = battery_get_battlevel_car();
		if(cur_car_volt < 0)
		{
			LOGE(LOG_TARGET, "%s> Getting car volt is failed.\n", __FUNCTION__);
			return;
		}
		
		prev_time_send = cur_ktime;
		prev_time_chk_batt = cur_ktime;
		prev_car_volt = cur_car_volt;

		LOGI(LOG_TARGET, "%s> Prev:%d Cur:%d V:%d\n", __FUNCTION__, prev_time_chk_batt, cur_ktime, cur_car_volt);
		
		cur_car_volt = cur_car_volt / 1000;
		sender_add_data_to_buffer(eREPORT_BATT, &cur_car_volt, ePIPE_1);
		return;
	}

	if(cur_ktime - prev_time_chk_batt > CYCLE_CHECK_BATT_DATA)
	{
		cur_car_volt = battery_get_battlevel_car();
		if(cur_car_volt < 0)
		{
			LOGE(LOG_TARGET, "%s> Getting car volt is failed.\n", __FUNCTION__);
			return;
		}

		LOGI(LOG_TARGET, "%s> Prev:%d Cur:%d PreV:%d CurV:%d\n",
			__FUNCTION__, prev_time_chk_batt, cur_ktime, prev_car_volt, cur_car_volt);

		if(prev_car_volt - cur_car_volt > 1000 || prev_car_volt - cur_car_volt < -1000)
		{
			LOGI(LOG_TARGET, "%s> Changed Car volt is exceed 1V.", __FUNCTION__);
		
			prev_time_send = cur_ktime;
			prev_car_volt = cur_car_volt;
			
			cur_car_volt = cur_car_volt / 1000;
			sender_add_data_to_buffer(eREPORT_BATT, &cur_car_volt, ePIPE_1);
		}
		prev_time_chk_batt = cur_ktime;
		return;
	}
}
*/

void network_fail_emergency_reset_callback(void)
{
    _process_poweroff(1, "network chk fail");
}


void gps_ant_ok_callback(void)
{

}

void gps_ant_nok_callback(void)
{

}

static pthread_mutex_t g_mutex_write_vaild_dat_timeout = PTHREAD_MUTEX_INITIALIZER;
int pthread_mutex_timedlock( pthread_mutex_t * mutex, const struct timespec * abs_timeout );

void write_vaild_data()
{
	unsigned int prev_mileage = 0;
	int current_mileage = 0;

	//jwrho++
	struct timespec abs_time;
	int ret;

	clock_gettime(CLOCK_REALTIME , &abs_time);
    abs_time.tv_sec += 60;

	ret = pthread_mutex_timedlock(&g_mutex_write_vaild_dat_timeout,&abs_time);
	if(ret != 0){
		LOGD(LOG_TARGET, "%s Wait timed out [%d]\n", __FUNCTION__, ret);
		return -1;
	}
	//jwrho++	

    // every time save gps data..
    gps_valid_data_write();
    //mileage_write(); // overwrite bug fix

	mileage_set_m(get_server_mileage() + get_gps_mileage());
	mileage_write(); // overwrite bug fix

    save_mileage_file(get_server_mileage() + get_gps_mileage()); // save 

	pthread_mutex_unlock(&g_mutex_write_vaild_dat_timeout); //jwrho
}
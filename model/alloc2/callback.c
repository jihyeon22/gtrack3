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
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include <at/at_util.h>

#include "alloc2_pkt.h"
#include "alloc2_senario.h"
#include "alloc2_obd_mgr.h"
#include "alloc2_bcm_mgr.h"
#include "alloc2_daily_info.h"

static int flag_run_thread_main = 1;
static int model_ignition_stat = 0;

#include "allkey_bcm_1.h"
#include "seco_obd_1.h"

ALLKEY_BCM_1_INFO_T g_allkey_bcm_info = {0,};

int mdm_bcm_evt_proc(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list);



void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");

	//alloc2_init_car_daily_info();
#ifdef USE_ALLKEY_BCM_1
	allkey_bcm_1_init(&mdm_bcm_evt_proc);
	//allkey_bcm_ctr__theft_on(0);
	allkey_bcm_ctr__door_evt_on(1);
	//allkey_bcm_ctr__get_info(&g_allkey_bcm_info);
	printf("--------------------------------------------------\r\n");
	printf("g_allkey_bcm_info->init_stat [%x] \r\n", g_allkey_bcm_info.init_stat );
	printf("g_allkey_bcm_info->horn_cnt [%d]\r\n", g_allkey_bcm_info.horn_cnt );
	printf("g_allkey_bcm_info->light_cnt [%d]\r\n", g_allkey_bcm_info.light_cnt );
	printf("g_allkey_bcm_info->bcm_swver [0x%x]\r\n", g_allkey_bcm_info.bcm_swver );
	printf("--------------------------------------------------\r\n");
#endif

#ifdef USE_SECO_OBD_1
	init_seco_obd_mgr("/dev/ttyHSL1", 115200, alloc2_obd_mgr__obd_broadcast_proc);
#endif

	alloc2_init_car_daily_info();
	init_mdm_setting_pkt_val();
	init_obd_dev_pkt_info();

	alloc2_obd_mgr__init();
	
	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
}

void network_on_callback(void)
{
	printf("gtrack calback ::: network_on_callback !!!\r\n");
	devel_webdm_send_log("Accumulate distance : %um at the start\n", mileage_get_m());
}

void button1_callback(void)
{
	//allkey_bcm_ctr__door_lock(1);
	printf("gtrack calback ::: button1_callback !!!\r\n");
}

void button2_callback(void)
{
	//allkey_bcm_ctr__door_lock(0);
	printf("gtrack calback ::: button2_callback !!!\r\n");
	set_no_send_pwr_evt_reboot();
}

void ignition_on_callback(void)
{
	model_ignition_stat = 1;
	init_keyon_section_distance( mileage_get_m() );
	printf("gtrack calback ::: ignition_on_callback !!!\r\n");

	int evt_code = e_evt_code_igi_on;
	
	if ( get_no_send_pwr_evt_reboot(EVT_TYPE_IGI_ON) == SEND_TO_PWR_EVT_OK )
	{
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
	}
	else
		LOGE(eSVC_MODEL, "NEED TO IGI_ON EVT : BUT SKIP\r\n");

	sender_add_data_to_buffer(e_firm_info, NULL, ePIPE_2);
}

void ignition_off_callback(void)
{
	model_ignition_stat = 0;
	printf("gtrack calback ::: ignition_off_callback !!!\r\n");

	int evt_code = e_evt_code_igi_off;

	if ( get_no_send_pwr_evt_reboot(EVT_TYPE_IGI_OFF) == SEND_TO_PWR_EVT_OK )
	{
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
	}
	else
		LOGE(eSVC_MODEL, "NEED TO IGI_OFF EVT : BUT SKIP\r\n");

}

void power_on_callback(void)
{	
	int evt_code = e_evt_code_poweron;

	if ( get_no_send_pwr_evt_reboot(EVT_TYPE_POWER_ON) == SEND_TO_PWR_EVT_OK )
	{
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
	}
	else
		LOGE(eSVC_MODEL, "NEED TO POWER_ON EVT : BUT SKIP\r\n");
		
	printf("gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	int evt_code = e_evt_code_poweroff;

	if ( get_no_send_pwr_evt_reboot(EVT_TYPE_POWER_OFF) == SEND_TO_PWR_EVT_OK )
	{
		//if ( get_cur_status() > e_SEND_TO_SETTING_INFO_ING )
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
	}
	else
		LOGE(eSVC_MODEL, "NEED TO POWER_OFF EVT : BUT SKIP\r\n");

	printf("gtrack calback ::: power_off_callback !!!\r\n");
	alloc2_poweroff_proc("poweroff callback");

}

#define RESET_TARGET_TIME_DIFF_SEC		20
#define BATT_CHK_INTERVAL_SEC 			60
#define DEBUG_LOG_PRINT_INTERVAL		5

void gps_parse_one_context_callback(void)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val;

	static int gps_run_cnt = 0;
	static int batt_chk_cnt = 0;

	int keyon_send_interval = 0;
	int keyoff_send_interval = 0;
	int report_interval = 0;
	int current_time_sec = 0;
	int reset_target_sec = 0;
	int chk_car_low_batt = 0;
	
	int current_senario = 0;
	static int need_to_reset_senario = 0;

	gpsData_t gpsdata = {0,};

	gps_get_curr_data(&gpsdata);

	if ( model_ignition_stat == 1 )
		mileage_process(&gpsdata);

	current_senario = get_cur_status();
	p_mdm_setting_val = get_mdm_setting_val();
/*
	{
		cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
		save_car_daliy_info(cur_daily_date_num, g_obdData.car_mileage_total, 0);
			saved_daily_date_num = get_cur_daily_date_num();
		cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
		
		// 켰을때 날짜가 변경되었을 경우
		if ( cur_daily_date_num > saved_daily_date_num )
		{
			clr_daily_info();
			if ((init_server_routine() == 0) && ( g_obdData.obd_read_stat == OBD_RET_SUCCESS ))
				save_car_daliy_info(cur_daily_date_num, g_obdData.car_mileage_total, g_obdData.car_fuel_consumption_total);
		}
	}
*/

	if ( ( gpsdata.active == 1 ) && (gpsdata.speed > 30) )
		set_car_ctrl_enable(CAR_CTRL_DISABLE);
	else
		set_car_ctrl_enable(CAR_CTRL_ENABLE);


	if ( p_mdm_setting_val != NULL ) // always send to server..
	{
		keyon_send_interval = p_mdm_setting_val->key_on_gps_report_interval;
		keyoff_send_interval = p_mdm_setting_val->key_off_gps_report_interval;
		reset_target_sec = p_mdm_setting_val->mdm_reset_interval*100;
		chk_car_low_batt = p_mdm_setting_val->low_batt_voltage;
	}
	else
	{
		keyon_send_interval = 60;
		keyoff_send_interval = 180;
	}

	current_time_sec = (gpsdata.hour*10000) + (gpsdata.min*100) + gpsdata.sec;

	set_overspeed_info(&gpsdata);
	// if (!( gps_run_cnt % DEBUG_LOG_PRINT_INTERVAL ))
	//	LOGT(eSVC_MODEL,"[GPS THREAD] senario stat [%d] / cur time [%d] / reset target [%d]\r\n", current_senario, current_time_sec, reset_target_sec);
	 
	//printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");
//	if ( get_cur_status() < e_SEND_TO_SETTING_INFO_ING )
//	{
//		LOGI(eSVC_MODEL, "Not complete init process..\r\n");
//		return;
//	}

	// ---------------------------------------------------------------------
	// poweroff senario 
	// ---------------------------------------------------------------------
	// 특정시간이 되면 단말을 리셋한다. gps 시간기준으로 리셋
	// 즉 설정한 시간이 있고 & 현재 시간과의 차이가 0보다 크고 & 20초 이하면..
	// 무조건 0보다 클때 리셋하면 계속 리셋하겄지? 그래서 20초 차이날때만 리셋 
	if ( ( reset_target_sec > 0 ) && ( (current_time_sec - reset_target_sec) > 0 ) && ( (current_time_sec - reset_target_sec) < RESET_TARGET_TIME_DIFF_SEC ) )
	{
		if ( need_to_reset_senario == 0 ) 
		{
			need_to_reset_senario = 1;

			if ( ( gpsdata.speed == 0 ) && ( model_ignition_stat == 0 ) )
				set_no_send_pwr_evt_reboot();
			else
				LOGE(eSVC_MODEL, "NEED TO RESET BUT SKIP: spd[%d], igi[%d]\r\n",gpsdata.speed, model_ignition_stat);
		}

		
	}
	
	gps_run_cnt++;
	batt_chk_cnt++;


	// ---------------------------------------------------------------------
	// gps senario 
	// ---------------------------------------------------------------------
	if ( model_ignition_stat == 1 )
		report_interval = keyon_send_interval;
	else
		report_interval = keyoff_send_interval;
	
	if (!( gps_run_cnt % DEBUG_LOG_PRINT_INTERVAL ))
		LOGT(eSVC_MODEL, "[GPS THREAD] key [%d] / keyon gps [%d] / keyoff gps [%d] / cnt [%d]/[%d]\r\n", model_ignition_stat, keyon_send_interval, keyoff_send_interval, gps_run_cnt, report_interval);


	if ( (report_interval > 0 ) && ( gps_run_cnt > report_interval ) )
	{
		int evt_code = e_evt_code_normal;
		//if ( current_senario == e_SEND_TO_ONLY_GPS_DATA )
		LOGT(eSVC_MODEL, "[GPS THREAD] send gps info!!!! [%d]/[%d]\r\n", gps_run_cnt, report_interval);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_1);
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_1);

		gps_run_cnt = 0;
	}

	// ---------------------------------------------------------------------
	// batt chk 
	// ---------------------------------------------------------------------
	if ( ( chk_car_low_batt > 0 ) && ( batt_chk_cnt > BATT_CHK_INTERVAL_SEC ) )
	{
		int car_voltage = 0;
        at_get_adc_main_pwr(&car_voltage);
		car_voltage = car_voltage * 10;

		if ( chk_car_low_batt > car_voltage)
		{
			int evt_code = e_evt_code_car_low_batt;
			LOGT(eSVC_MODEL, "[GPS THREAD] send low batt!!!!!! [%d]/[%d]\r\n", gps_run_cnt, report_interval);

			sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
			sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);
		}
		batt_chk_cnt = 0;
	}

	{
		int cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
		save_daily_info__total_distance(cur_daily_date_num, mileage_get_m());
	}

}

void main_loop_callback(void)
{
	int main_loop_cnt = 0;
	int no_send_pwr_evt_flag_clr = 0;
	int keyon_obd_send_interval = 0;
	int keyoff_obd_send_interval = 0;
	int report_obd_interval = 0;

	
	/*
	// wait for network on and key on ?
	while(1)
	{
		if ( model_ignition_stat == 1)
			break;
		sleep(1);
	}*/

	while(1)
	{
		ALLOC_PKT_RECV__OBD_DEV_INFO* p_obd_dev_info = NULL;
 		
		main_loop_cnt ++;
		no_send_pwr_evt_flag_clr ++;
		// -----------------------------------------------------------
		// hw check
		// -----------------------------------------------------------
		chk_allkey_bcm();
		
		// ----------------------------------------------------------
		// senario setting
		// ----------------------------------------------------------
		if ( get_cur_status() == e_SEND_TO_SETTING_INFO )
		{
			set_cur_status(e_SEND_TO_SETTING_INFO_ING);
			sender_add_data_to_buffer(e_mdm_setting_val, NULL, ePIPE_1);
		}
	
		if ( get_cur_status() == e_SEND_TO_OBD_INFO )
		{
			set_cur_status(e_SEND_TO_OBD_INFO_ING);
			sender_add_data_to_buffer(e_obd_dev_info, NULL, ePIPE_1);
			{
				ALLOC_PKT_SEND__OBD_STAT_ARG obd_stat_arg;
				obd_stat_arg.obd_stat_flag = 0;
				obd_stat_arg.obd_stat = 0;
				obd_stat_arg.obd_remain_fuel_stat = 0;
				obd_stat_arg.obd_evt_code = 0;;
				obd_stat_arg.obd_fuel_type = 0;;
				obd_stat_arg.obd_remain_fuel = 0;;
				sender_add_data_to_buffer(e_obd_stat, &obd_stat_arg, ePIPE_1);
			}
		}
		
		// sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_1);
		p_obd_dev_info = get_obd_dev_info();

		if ( p_obd_dev_info != NULL ) // always send to server..
		{
			keyon_obd_send_interval = p_obd_dev_info->obd_recv_keyon_interval;
			keyoff_obd_send_interval = p_obd_dev_info->obd_recv_keyoff_interval;
		}
		else
		{
			keyon_obd_send_interval = -1;
			keyoff_obd_send_interval = -1;
		}

		// ----------------------------------------------------
		// obd report senario
		// ----------------------------------------------------
		if ( model_ignition_stat == 1 )
			report_obd_interval = keyon_obd_send_interval;
		else
			report_obd_interval = keyoff_obd_send_interval;
		
		if (!( main_loop_cnt % DEBUG_LOG_PRINT_INTERVAL ))
			LOGT(eSVC_MODEL, "[MAIN LOOP] key [%d] / keyon obd [%d] / keyoff obd [%d] / cnt [%d]/[%d]\r\n", model_ignition_stat, keyon_obd_send_interval, keyoff_obd_send_interval, main_loop_cnt,report_obd_interval);

		if ( (report_obd_interval > 0 ) && ( main_loop_cnt > report_obd_interval ) )
		{
			int evt_code = e_evt_code_normal;
			sender_add_data_to_buffer(e_obd_data, NULL, ePIPE_1);
			sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_1);
			main_loop_cnt = 0;
		}

		alloc2_obd_mgr__run_cmd_proc();

		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();

		// 부팅한지 30초가 지나면 기존의 no send pwr 플래그는 강제로 지운다.
		if ( no_send_pwr_evt_flag_clr == 30 )
			clr_no_send_pwr_evt_reboot();
		sleep(1);
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


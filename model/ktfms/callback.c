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
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include "logd/logd_rpc.h"

#include <base/dmmgr.h>

#include "include/defines.h"
#include "util/nettool.h"
#include "base/watchdog.h"
#include "board/modem-time.h"
#include "board/led.h"

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "seco_obd.h"
#include "kt_fms_packet.h"
#include "pkt_manage.h"
#include "net_kti_senario.h"


static int flag_run_thread_main = 1;
static obdData_t g_obdData = {0,};
static int need_to_daily_info_init = 0;

static int _gps_time_cnt = 0;

#define OBD_READ_INTERVAL_SEC	1
#define FAIL_OBD_FAIL_CNT 		2

#define LOG_PRINT_INTERVAL_SEC	5
#define HW_CHK_INTERVAL_SEC		20

// 바뀌는 즉시 noti 해야한다.
#define CHK_KEY_STAT_HOLD_SEC		0

#define CHK_KEY_STAT_HOLD_SEC3		(30)

//#define ONLY_KT_FOTA_TEST

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int test_sdr_factor[e_MAX_FACTOR_ID] = {0,};


long long last_total_trip = 0;
unsigned int last_total_fuel = 0;


void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();
	
#ifdef KT_FOTA_TEST_SVR
	thread_network_set_warn_timeout(0);
#else
	thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
#endif

#ifdef ONLY_KT_FOTA_TEST
	return;
#endif	

	init_fms_server_policy();
	
	init_car_daily_info();
	init_trip_seq();
	init_hw_err_code();
	
	g_obdData.obd_read_stat = OBD_CMD_RET_INVALID_COND;
	
	load_obd_info();
}

void network_on_callback(void)
{
}

void button1_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
}

void button2_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
}

static power_status = 0;

void ignition_on_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
	LOGI(LOG_TARGET, "IGNI ON Callback ...\n");
	//if ( get_send_policy() != KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
		//set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);	
	power_status = 1;
}

void ignition_off_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
	LOGI(LOG_TARGET, "IGNI OFF Callback ...\n");
	//mileage_write();
	//if ( get_send_policy() != KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
//		set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
	power_status = 0;
}

void power_on_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
	LOGI(LOG_TARGET, "PWR ON Callback ...\n");
	//if ( get_send_policy() != KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
		//set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);	
}

void power_off_callback(void)
{
	printf("%s() - [%d]\r\n", __func__, __LINE__);
	LOGI(LOG_TARGET, "PWR OFF Callback ...\n");
	//if ( get_send_policy() != KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
	set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);

	mileage_write();
	//_process_poweroff("power_off_callback");
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	poweroff("poweroff senario", strlen("poweroff senario"));
}

//static int need_to_seq_init = 0;

//#define TEST_CODE_DAY_CHANGE	
#ifdef TEST_CODE_DAY_CHANGE
int day_change = 0;
#endif



void gps_parse_one_context_callback(void)
{
#ifdef ONLY_KT_FOTA_TEST
	return;
#endif

	char head_buff[1024] = {0,};
	char body_buff[1024] = {0,};
	
	char body_len = 0;
	
	char test_buff[512] = {0,};
	
	static int init_flag = 0;
	
	// static int need_to_seq_init = 0;
	
	static int collect_interval = 0;
	static int report_interval = 0;
	
	//static int cur_sdr_factor[e_MAX_FACTOR_ID] = {0,};
	static int cur_send_policy = KT_FMS_SEND_POLICY__NONE;
	
	static int prev_sec = 0;
	static int cur_sec = 0;
	
	static int first_gps_hit = 0;
	
	int total_body_cnt = 0;
	
	int saved_daily_date_num = 0;
	
	int cur_daily_date_num = 0;
	static int last_daily_date_num = 0;
	
	int force_pkt_done = 0;
	
	server_policy_t cur_server_policy = {0,};
	
	gpsData_t gpsdata = {0,};
	gpsData_t last_gpsdata = {0,};
	
	gps_get_curr_data(&gpsdata);
	
	// gpsdata.active == 1 // active
	// gpsdata.active == 0 // inactive
	
	if (gpsdata.active == 1)
	{
	//	printf("gps active : cur lat [%f]\r\n",gpsdata.lat);
	//	printf("gps active : cur lon [%f]\r\n",gpsdata.lon);
		
		set_hw_err_code(e_HW_ERR_CODE_GPS_INVAILD , 0);

		if ( power_status == 1 )
			mileage_process(&gpsdata);

		first_gps_hit = 1;
	}
	else
	{
		// 최초로 gps 을 잡기전까지는 최종적으로 수신되었던 좌표를 입력한다.
		if ( first_gps_hit == 0)
		{
			gps_valid_data_get(&last_gpsdata);
			gpsdata.lat = last_gpsdata.lat;
			gpsdata.lon = last_gpsdata.lon;
			printf("gps deactive case 1: cur lat [%f]\r\n",gpsdata.lat);
			printf("gps deactive case 1: cur lon [%f]\r\n",gpsdata.lon);
		}
		else
		{
			printf("gps deactive case 2: cur lat [%f]\r\n",gpsdata.lat);
			printf("gps deactive case 2: cur lon [%f]\r\n",gpsdata.lon);
		}
		
		set_hw_err_code(e_HW_ERR_CODE_GPS_INVAILD , 1);
	}
	
	
	// ---------------------------------
	// time check..
	// ---------------------------------
	if (( (gpsdata.year % 100) < 16))
	{
		LOGE(LOG_TARGET, "GPS PARSE : TIME IS INVALID!! \n");
		return;
	}
	
	#ifdef TEST_CODE_DAY_CHANGE
	gpsdata.day += day_change;
	#endif
	
	//printf("g_obdData.obd_read_stat is [%d]\r\n",g_obdData.obd_read_stat);
	
	// 최초 1회 세팅
	if ( init_flag == 0 )
	{
		collect_interval = get_collection_interval();
		report_interval = get_report_interval();
		cur_send_policy = get_send_policy();
		
		cur_sec = prev_sec = gpsdata.sec + (gpsdata.min*60) + (gpsdata.hour*60*60);
		last_daily_date_num = cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
		
		if ( g_obdData.obd_read_stat == OBD_CMD_RET_INVALID_COND )
		{
			LOGE(LOG_TARGET, "WAIT FOR MAIN THREAD\n");
			return;
		}
		
		init_flag = 1;
		
		// need_to_seq_init = 1;
	}
	
	// ---------------------------------
	// interval valid check.
	// ---------------------------------
	if ( ( report_interval <= 0 ) || ( collect_interval <= 0 ) )
	{
		LOGE(LOG_TARGET, "GPS PARSE : PKT INTERVAL INVAILD !! \n");
		
		// need_to_seq_init = 1;
		return ;
	}
	

	
	// ---------------------------------
	// pkt size check...
	// ---------------------------------
	total_body_cnt = report_interval / collect_interval;
	
	if ( total_body_cnt > MDS_PACKET_1_MAX_SAVE_CNT )
	{
		LOGE(LOG_TARGET, "GPS PARSE : ERR support body cnt [%d] / [%d] \n", total_body_cnt, MDS_PACKET_1_MAX_SAVE_CNT );
		
		// need_to_seq_init = 1;
		
		return;
	}

	
	// -------------------------------
	// skip condition
	// -------------------------------
	
	// 서버에서 fail stop 을 받았으면..
	// 굳이 패킷을 만들필요없이 바로 종료시킨다.
	/*
	if ( get_send_policy() == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
	{
		LOGI(LOG_TARGET, " >>>> policy case --> fail stop \n");
		set_send_policy(KT_FMS_SEND_POLICY__SERVER_FAIL_STOP);
		cur_send_policy = KT_FMS_SEND_POLICY__SERVER_FAIL_STOP;
		
		mds_packet_1_clear_rear();
	}
	*/
	
	if ( g_obdData.obd_daily_info_init == 1)
	{
		g_obdData.obd_daily_info_init = 0;
		
		clr_daily_info();
		
		cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
		
		if ( (init_server_routine() == 0) && ( g_obdData.obd_read_stat == OBD_RET_SUCCESS ) )
			save_car_daliy_info(cur_daily_date_num, g_obdData.car_mileage_total, g_obdData.car_fuel_consumption_total);
	}
	
	if ( g_obdData.obd_read_stat == OBD_RET_SUCCESS )
	{
		get_daliy_fuel(g_obdData.car_fuel_consumption_total);
		get_daliy_trip(g_obdData.car_mileage_total);
	}
				
	if (( cur_send_policy == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP ) || \
		( cur_send_policy == KT_FMS_SEND_POLICY__NONE ) || \
		( cur_send_policy == KT_FMS_SEND_POLICY__PWR_OFF_EVENT ))
	{
		LOGE(LOG_TARGET, "GPS PARSE : skip condition... [%d]", cur_send_policy);
	
		if ( ( g_obdData.obd_read_stat == OBD_RET_SUCCESS ) && (g_obdData.car_key_stat == 0 ) )
		{
			//need_to_seq_init = 1;
			clear_trip_seq();
		}
		
		if ( cur_send_policy == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
			mds_packet_1_clear_rear();
		
		if ( cur_send_policy != get_send_policy() )
			cur_send_policy = get_send_policy();

		// packet 갯수를 맞추기 위해서..
		_gps_time_cnt = 0;

		return;
	}
	
	// ------------------------------------------------
	// trip seq & daily info
	// ------------------------------------------------

	saved_daily_date_num = get_cur_daily_date_num();
	cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
	
	// 켰을때 날짜가 변경되었을 경우
	if ( cur_daily_date_num > saved_daily_date_num )
	{
		clr_daily_info();
		if ((init_server_routine() == 0) && ( g_obdData.obd_read_stat == OBD_RET_SUCCESS ))
			save_car_daliy_info(cur_daily_date_num, g_obdData.car_mileage_total, g_obdData.car_fuel_consumption_total);
	}
	
	// 운행도중 날짜변경될 경우
	if ( cur_daily_date_num > last_daily_date_num )
	{
		
		// 무조건 패킷을 마무리 하고 만들게 한다!!
		force_pkt_done = 1;
		
		LOGI(LOG_TARGET, "CHANGE DATE!!! [%d]\r\n" ,_gps_time_cnt);
		LOGD(LOG_TARGET, "CHANGE DATE!!! [%d]\r\n" ,_gps_time_cnt);
	}

	

	// GPS 시간 필터링
	cur_sec = gpsdata.sec + (gpsdata.min*60) + (gpsdata.hour*60*60) + (gpsdata.day*24*60*60);

	// 시간을 필터링 한다. 시간은 무조건 흘러야 한다. 멈추거나, 뒤로 가면 안된다.
	if ( cur_sec <= prev_sec )
	{
		LOGE(LOG_TARGET, "GPS PARSE : invalid sec [%d] / [%d] ", prev_sec , prev_sec);
		return ;
	}
	
	prev_sec = cur_sec;
	
	// make pkt policy
	get_runtime_server_policy(&cur_server_policy);

	// seq factor re setting
	if ( ( cur_send_policy == KT_FMS_SEND_POLICY__INIT_EVENT ) || ( cur_send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT) )
	{
		// 전원 이벤트와 관련한 factor 설정
		// LOGE(LOG_TARGET, ">>>>>>>>>> GPS PARSE : seq trip re-init. \n");
		if (init_server_routine() == 0)
			set_trip_seq( gpsdata.year, gpsdata.mon, gpsdata.day, gpsdata.hour, gpsdata.min, gpsdata.sec);
		// need_to_seq_init = 0;
	}
	
	if (( _gps_time_cnt % LOG_PRINT_INTERVAL_SEC ) == 0)
	{
		convert_factor_id_to_str(test_buff,  cur_server_policy.sdr_factor);
		LOGT(LOG_TARGET, "CUR FACTOR is [%s]\r\n", test_buff);
	}

	
	
	// ---------------------------------------------------------------------
	// 패킷만들기 : 1초마다 한번씩 들어온다는 가정하에.. 만들기
	// ---------------------------------------------------------------------
	if (( _gps_time_cnt % 3 ) == 0)
		LOGT(LOG_TARGET, "CUR SEND POLICY [%d] / [%d/%d] / [%d/%d] \n", cur_send_policy, _gps_time_cnt % cur_server_policy.pkt_collect_interval_sec, cur_server_policy.pkt_collect_interval_sec, _gps_time_cnt % cur_server_policy.pkt_send_interval_sec, cur_server_policy.pkt_send_interval_sec);
	
	// collect 주기마다 body 를 만든다.
	// poweroff 라면... body 를 일단 마무리한다.
	if ( g_obdData.obd_read_stat == OBD_RET_SUCCESS )
	{
		if ((force_pkt_done == 0)  && ( ((_gps_time_cnt % cur_server_policy.pkt_collect_interval_sec) == 0)  || ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) ))
		{
			// 강제로 poweroff 를 한다고 시뮬레이션 하면... 
			// 실제 key stat field 에 0 으로 채워서 패킷을 보낸다.
			if ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT)
				g_obdData.car_key_stat = 0;

			//printf(" body insert [%d] : [%d]	\r\n", _gps_time_cnt, cur_send_policy);

			// collect timing 에 일단 body 를 만든다.
			// - OBD 장애와 정상일때의 패킷의 형태가 다르다.
			if ( g_obdData.obd_read_stat == OBD_RET_SUCCESS )
				body_len = make_sdr_body(body_buff, &gpsdata, &g_obdData, cur_server_policy.sdr_factor);
			else 
				body_len = make_sdr_body_null(body_buff, &gpsdata, &g_obdData, cur_server_policy.sdr_factor);

			mds_packet_1_make_and_insert(body_buff, body_len);
		}
	}
	else
	{
			// 강제로 poweroff 를 한다고 시뮬레이션 하면... 
			// 실제 key stat field 에 0 으로 채워서 패킷을 보낸다.
			if ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT)
				g_obdData.car_key_stat = 0;

			//printf(" body insert [%d] : [%d]	\r\n", _gps_time_cnt, cur_send_policy);

			// collect timing 에 일단 body 를 만든다.
			// - OBD 장애와 정상일때의 패킷의 형태가 다르다.
			if ( g_obdData.obd_read_stat == OBD_RET_SUCCESS )
				body_len = make_sdr_body(body_buff, &gpsdata, &g_obdData, cur_server_policy.sdr_factor);
			else 
				body_len = make_sdr_body_null(body_buff, &gpsdata, &g_obdData, cur_server_policy.sdr_factor);

			mds_packet_1_make_and_insert(body_buff, body_len);
	}
	
	// obd ret fail 이다.
	// 그러면 패킷을 마무리 하고 끝낸다.
	// 끝내는 과정은 poweroff 하고 똑같이 한다.
	if ( g_obdData.obd_read_stat != OBD_RET_SUCCESS)
	{
		LOGE(LOG_TARGET, "INVAILD OBD DATA [%d]\r\n", g_obdData.car_key_stat);
		set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
		
		// 다시 초기화 시작 
		init_flag = 1;
	}
	
	// 만약에 Poweroff 정책이 반영된다면, 바로 패킷을 마무리하고 쏴야한다.	
	if (( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) || ( g_obdData.car_key_stat == 0 ) )
	{
		_gps_time_cnt = (cur_server_policy.pkt_send_interval_sec-1);
		
		printf("POWER OFF SEQ pkt done..	\r\n");
		printf("POWER OFF SEQ pkt done..	\r\n");
		
		// 기존의 seq 저장한것을 지운다.
		LOGT(LOG_TARGET, "POWER OFF SEQ pkt done..	\n");
	}
	
	//printf("_gps_time_cnt is [%d]\r\n", _gps_time_cnt);
	
	// send 주기마다 header 를 붙인다.
	if ( (force_pkt_done == 1) || (((_gps_time_cnt >= (cur_server_policy.pkt_send_interval_sec-1)) ) && (_gps_time_cnt != 0)))
	{
		// header 를 만들기위해 현재 패킷의 body 의 size 를 얻어온다.
		int sdr_body_size = mds_packet_1_get_cur_body_size();
		int sdr_head_size = 0;
		
		force_pkt_done = 0;
		
		//printf("------------- pkt done ----------------------\r\n");
		LOGI(LOG_TARGET, " >>>> PKT DONE.. policy [%d] / [%d]\n", cur_send_policy, get_send_policy() );
		
		// http header 포함 header 를 만들고 해당 패킷을 마무리 한다.
		memset(head_buff, 0x00, sizeof(head_buff));
		sdr_head_size = make_sdr_header(head_buff, sdr_body_size, cur_server_policy.sdr_factor, cur_server_policy.policy_num, cur_send_policy);
		mds_packet_1_make_done(head_buff, sdr_head_size);
		
		// running 에서 power off 로 변경시에..
		if ( ( cur_send_policy == KT_FMS_SEND_POLICY__RUNNING ) && ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) )
		{
			LOGI(LOG_TARGET, " >>>> policy case 1 \n");
			set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
			cur_send_policy = KT_FMS_SEND_POLICY__PWR_OFF_EVENT;
			//need_to_seq_init = 1;
			
			clear_trip_seq();
		}
		// poweroff 까지 끝냈으면 none 으로 변경
		else if ( ( cur_send_policy == KT_FMS_SEND_POLICY__PWR_OFF_EVENT ) && ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) )
		{
			LOGI(LOG_TARGET, " >>>> policy case 2 \n");
			set_send_policy(KT_FMS_SEND_POLICY__NONE);
			cur_send_policy = KT_FMS_SEND_POLICY__NONE;
		}
		// on 일경우는 running 으로 변경
		else if ( ( cur_send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT ) && ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_ON_EVENT) )
		{
			LOGI(LOG_TARGET, " >>>> policy case 3 \n");
			//server_policy_t tmp_server_policy = {0,};
			//get_server_policy(&tmp_server_policy);
			set_send_policy(KT_FMS_SEND_POLICY__RUNNING);
			cur_send_policy = KT_FMS_SEND_POLICY__RUNNING;
			
		}
		// none 상태에서... on 이면 on.
		else if ( ( cur_send_policy == KT_FMS_SEND_POLICY__NONE ) && ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_ON_EVENT) )
		{
			LOGI(LOG_TARGET, " >>>> policy case 4 \n");
			set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);
			cur_send_policy = KT_FMS_SEND_POLICY__PWR_ON_EVENT;
		}
		// init 상태에서... init 이면 on.
		else if ( ( cur_send_policy == KT_FMS_SEND_POLICY__INIT_EVENT ) && ( get_send_policy() == KT_FMS_SEND_POLICY__INIT_EVENT ) )
		{
			LOGI(LOG_TARGET, " >>>> policy case 5 \n");
			
			if ( cur_server_policy.policy_num != 0)
			{
				set_send_policy(KT_FMS_SEND_POLICY__RUNNING);
				cur_send_policy = KT_FMS_SEND_POLICY__RUNNING;
			}
			else
			{
				//need_to_seq_init = 1;
				set_send_policy(KT_FMS_SEND_POLICY__INIT_EVENT);
				cur_send_policy = KT_FMS_SEND_POLICY__INIT_EVENT;
			}
		}
		else if ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) 
		{
			LOGI(LOG_TARGET, " >>>> policy case --> poweroff \n");
			set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
			cur_send_policy = KT_FMS_SEND_POLICY__PWR_OFF_EVENT;
			//need_to_seq_init = 1;
			
			clear_trip_seq();
		}
		else if ( get_send_policy() == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP) 
		{
			LOGI(LOG_TARGET, " >>>> policy case --> fail stop \n");
			set_send_policy(KT_FMS_SEND_POLICY__SERVER_FAIL_STOP);
			cur_send_policy = KT_FMS_SEND_POLICY__SERVER_FAIL_STOP;
			//need_to_seq_init = 1;
			
			//clear_trip_seq();
		}
		else
		{
			LOGI(LOG_TARGET, " >>>> policy case .... [%d] / [%d]\n",cur_send_policy, get_send_policy());
			set_send_policy(KT_FMS_SEND_POLICY__RUNNING);
			cur_send_policy = KT_FMS_SEND_POLICY__RUNNING;
		}
		
		// 현재의 sdr factor 적용 for running mode..
		set_runtime_server_policy();
		
		
		// 수집주기 변경됐을때, 다음패킷부터 적용
		if ( collect_interval != get_collection_interval() )
			collect_interval = get_collection_interval();
		
		if ( report_interval != get_report_interval() )
			report_interval = get_report_interval();
			
		prev_sec = cur_sec;
		
		// 정책이 변경될수도있기때문에 pkt 만드는 횟수 카운터를 초기화 한다.
		
		// 운행도중 날짜변경될 경우
		if ( cur_daily_date_num > last_daily_date_num )
		{
			// daily info 초기화
			clr_daily_info();
			if ( g_obdData.obd_read_stat == OBD_RET_SUCCESS )
				save_car_daliy_info(cur_daily_date_num, g_obdData.car_mileage_total, g_obdData.car_fuel_consumption_total);
			
			// 트립시퀀스 초기화
			clear_trip_seq();
			if (init_server_routine() == 0)
				set_trip_seq( gpsdata.year, gpsdata.mon, gpsdata.day, gpsdata.hour, gpsdata.min, gpsdata.sec);

			LOGI(LOG_TARGET, "CHANGE DATE!!! 2 [%d]\r\n" ,_gps_time_cnt);
			LOGD(LOG_TARGET, "CHANGE DATE!!! 2 [%d]\r\n" ,_gps_time_cnt);
		}
		
		_gps_time_cnt = 0;
		
		last_daily_date_num = cur_daily_date_num;
		
		return;
	}

	//convert_factor_id_to_str(test_buff, cur_sdr_factor);
	//printf("cur_sdr_factor - 6 is [%s]\r\n",test_buff);
	last_daily_date_num = cur_daily_date_num;
	
	prev_sec = cur_sec;
	_gps_time_cnt++;
}

#define MAX_SET_TOTAL_ANDO_INIT_DAILY_RETRY_CNT		5

void main_loop_callback(void)
{
#ifdef ONLY_KT_FOTA_TEST
	while(1)
		sleep(10);
#endif
	int obd_ret = 0;	
	
	int modem_time_sec_cur = 0;
	int modem_time_sec_last = 0;
	
	int main_time = 0;
	int obd_fail_cnt = 0;
	
	int cur_key_stat_on = 0;
	int cur_key_stat_off = 0;
	
	int change_server_proc_hold = 0;
	
	

	int main_cur_send_policy = 0;
	
	odbGender_t cur_gender_spec = {0,};
	//odbGender_t last_gender_spec = {0,};
			
	
	
	obdData_t obd_data = {0,};
	
	modem_time_sec_last = modem_time_sec_cur = get_modem_time_utc_sec();
	
	printf("MAIN : main thread start.... \r\n");
	
	if ( get_obd_gender_spec(&cur_gender_spec) == OBD_RET_SUCCESS)
	{
		dbg_print_gender_spec(&cur_gender_spec);
	}
	
	pre_init();
				
	while(flag_run_thread_main)
	{
		main_time++;

		modem_time_sec_cur = get_modem_time_utc_sec();
		
		// printf("[%d]:[%d]:[%d]\r\n",modem_time_sec_cur/3600,modem_time_sec_cur/60,modem_time_sec_cur%10); 
		
		watchdog_set_cur_ktime(eWdMain);
		
		// 서버 변경요청이 있을경우...
		//  - 강제로 poweroff 를 만들어서 서버에 poweroff pkt 을 보내고
		//  - 기존의 모든 저장되었던 정책을 삭제후에 재부팅한다.
		if ( chk_change_server() == 1 )
		{
			// 기존에 서버를 무한정 대기시키던것을 다시 원복한다.
			set_server_send_interval_default();
			
			printf("change server req power off! [%d] !!!!\r\n", change_server_proc_hold);
			
			// 강제로 poweroff 루틴을 타게 한다.
			set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
		
			// 기존의 모든 패킷을 모두 쐈다면...
			// 모든정보를 초기화하고 재부팅한다.
			if (( flush_mds_packet_1(1) == -1 ) && ( change_server_proc_hold > CHK_KEY_STAT_HOLD_SEC3 ))
			{
				init_server_and_poweroff();
			}

			change_server_proc_hold++;
			
			sleep(1);
			continue;
		}
		

		// 시나리오 step. 1
		//  현재 모뎀의 차대번호, 차량번호, 등등이 없을때 power led 를 붉은색 점등을 시키며
		check_init();
		
		
		watchdog_set_cur_ktime(eWdMain);

		
		// -----------------------------------------
		// 매초 odb_data 를 획득한다.
		// -----------------------------------------
		if (( main_time % OBD_READ_INTERVAL_SEC ) == 0)
		{
			memset(&obd_data, 0x00, sizeof(obdData_t));
			obd_ret = req_obd_data(&obd_data);
			
			set_last_obd_stat(obd_ret);
			// ******* OBD READ SUCCESS *******************			
			if (obd_ret == OBD_RET_SUCCESS)
			{
				led_on(ePOWER_LED, eCOLOR_GREEN);
				printf("obd read success!\r\n");
				obd_fail_cnt = 0;
				
				g_last_dev_stat.obd_stat = 1;
				g_last_dev_stat.obd_key = obd_data.car_key_stat;
				g_last_dev_stat.obd_rpm = obd_data.car_rpm;
				g_last_dev_stat.obd_speed = obd_data.car_speed;
				g_last_dev_stat.last_trip = obd_data.car_mileage_total;
				
				set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DISCONN , 0);
				set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DATA_ERR , 0);
				
				// 운행거리는 이전값보다 작으면 안된다.
				if ( obd_data.car_mileage_total < last_total_trip )
				{
					LOGE(LOG_TARGET, "OBD ERR : obd trip data [%d] / [%d]\n", obd_data.car_mileage_total, last_total_trip);
					obd_data.car_mileage_total = last_total_trip;
				}
				else
				{
					last_total_trip = obd_data.car_mileage_total;
				}
				
				// 기름소모량는 이전값보다 작으면 안된다.
				if ( obd_data.car_fuel_consumption_total < last_total_fuel )
				{
					LOGI(LOG_TARGET, "OBD ERR : obd fuel data [%d] / [%d]\n", obd_data.car_fuel_consumption_total, last_total_fuel);
					obd_data.car_fuel_consumption_total = last_total_fuel;
				}
				else
				{
					last_total_fuel = obd_data.car_fuel_consumption_total;
				}
				
				
				/*
				printf("----------------------------------\r\n");
				dbg_print_obd_data(&g_obdData);
				printf("----------------------------------\r\n");
				*/
				
				if (( main_time % LOG_PRINT_INTERVAL_SEC ) == 0)
					LOGI(LOG_TARGET, "CHK : obd read success.. rpm [%d] key stat [%d]\n", obd_data.car_rpm ,obd_data.car_key_stat);
				
				
				// -------------------------------------
				// key 상태변화 확인
				// -------------------------------------
				if ( obd_data.car_key_stat == 1 )
				{
					cur_key_stat_on++;
					cur_key_stat_off = 0;
				}
				else if ( obd_data.car_key_stat == 0 )
				{
					cur_key_stat_on = 0;
					cur_key_stat_off++;
				}
				
				// poweroff 시에만 trip 정보를 변경해야한다.
				//  - 이전에 저장되어있던 트립변경요청을 실행한다.
				//if ( cur_key_stat_off > 3)
				{
					if ( proc_set_trip() == 0 )
					{
						int re_read = 0;
						int re_read_retry = MAX_SET_TOTAL_ANDO_INIT_DAILY_RETRY_CNT;
						
						last_total_trip = 0;
						last_total_fuel = 0;
						// 세팅값 바꾼후 이므로 데이터를 이전꺼를 버리고 새로 읽는다.
						req_obd_data(&obd_data);
						re_read = req_obd_data(&obd_data);
						
						while (re_read_retry--)
						{
							if (re_read == OBD_RET_SUCCESS)
							{
								memcpy(&g_obdData, &obd_data, sizeof(obdData_t));
								g_obdData.obd_daily_info_init = 1;
								printf("need to daily info init...\r\n");
								printf("need to daily info init...\r\n");
								printf("need to daily info init...\r\n");
								printf("need to daily info init...\r\n");
								printf("need to daily info init...\r\n");
								printf("need to daily info init...\r\n");
								
								break;
							}
							else
								sleep(1);
						}

						sleep(5);
						sleep(1);
						
						continue;
					}
				}
				
				// gps thread 에서 만들 pkt
				memcpy(&g_obdData, &obd_data, sizeof(obdData_t));

				
				if (( main_time % LOG_PRINT_INTERVAL_SEC ) == 0)
					LOGI(LOG_TARGET, ">> KEY STAT :: ON/OFF => [%d]/[%d] , OBD KEY STAT => [%d]\r\n", cur_key_stat_on, cur_key_stat_off, obd_data.car_key_stat);
				
				// key 시나리오 
				main_cur_send_policy = get_send_policy();
				
				// key - on 인지확인
				if ( cur_key_stat_on > CHK_KEY_STAT_HOLD_SEC )
				{
					if ( ( main_cur_send_policy == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) || \
						 ( main_cur_send_policy == KT_FMS_SEND_POLICY__NONE) )
					{
						LOGI(LOG_TARGET, "MAIN : KEY STAT CHANGE TO ON [%d] \n", obd_data.car_key_stat );
						set_send_policy(KT_FMS_SEND_POLICY__PWR_ON_EVENT);
						
						// 기존에 서버를 무한정 대기시키던것을 다시 원복한다.
						set_server_send_interval_default();

					#ifndef KT_FOTA_TEST_SVR
						devel_webdm_send_log("OBD KEY STAT - ON");
					#endif
					}
				}
				
				// off 인지확인
				if ( ( cur_key_stat_off > CHK_KEY_STAT_HOLD_SEC ) )
				{
					if ( ( main_cur_send_policy == KT_FMS_SEND_POLICY__INIT_EVENT) ||\
						 ( main_cur_send_policy == KT_FMS_SEND_POLICY__PWR_ON_EVENT) ||\
						 ( main_cur_send_policy == KT_FMS_SEND_POLICY__RUNNING) ||\ 
						 ( main_cur_send_policy == KT_FMS_SEND_POLICY__SERVER_FAIL_STOP))
					{
						LOGI(LOG_TARGET, "MAIN : KEY STAT CHANGE TO OFF [%d] \n", obd_data.car_key_stat );
						// 기존에 서버를 무한정 대기시키던것을 다시 원복한다.
						set_server_send_interval_default();
						
						//if ( get_send_policy() != KT_FMS_SEND_POLICY__SERVER_FAIL_STOP )
						set_send_policy(KT_FMS_SEND_POLICY__PWR_OFF_EVENT);
					#ifndef KT_FOTA_TEST_SVR
						devel_webdm_send_log("OBD KEY STAT - OFF");
					#endif
						dmmgr_send(eEVENT_UPDATE, NULL, 0);
					}
						
				}
				
				// g_obdData.obd_read_stat = OBD_CMD_RET_INVALID_COND;
				obd_fail_cnt = 0;
				
			}
			// ******* OBD READ FAIL *******************
			else
			{
				led_off(ePOWER_LED);
				printf("obd read fail!\r\n");
				obd_fail_cnt++;
				
				g_last_dev_stat.obd_stat = 0;
				g_last_dev_stat.obd_key = -1;
				g_last_dev_stat.obd_rpm = -1;
				g_last_dev_stat.obd_speed = -1;
					
				if (obd_fail_cnt >= FAIL_OBD_FAIL_CNT)
				{
					//LOGI(LOG_TARGET, " --> clear obd info");
					//memset(&g_obdData, 0x00, sizeof(obdData_t));
					
					set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DISCONN , 1);
					set_hw_err_code(e_HW_ERR_CODE_OBD_CONN_DATA_ERR , 1);
					
					g_obdData.obd_read_stat = OBD_RET_FAIL;
					
					obd_fail_cnt = 0;
				}
				
				LOGE(LOG_TARGET, "CHK : obd read fail.. [%d] \n", obd_fail_cnt);
			}
		}
		
		LOGI(LOG_TARGET, "CHK : server interval [%d] / [%d] \n", modem_time_sec_cur - modem_time_sec_last, get_server_send_interval());
		
		watchdog_set_cur_ktime(eWdMain);
		
		
		// poweroff 상태 혹은 non 상태일때..
		// queue 에 남은 패킷이 없다고 하면..
		// obd 에 실제 power line 을 단락시켜달라고 요청한다.
		poweroff_proc_1();
		
		
		// daliy trip info
		/*
		{
			long long daliy_trip = get_daliy_trip();
			int daliy_fuel = get_daliy_fuel();
			// 0~ 24 시 소모량
			printf("   - body pkt elelment --> daliy trip is [%lld]\r\n", daliy_trip);
			printf("   - body pkt elelment --> daliy fuel is [%d]\r\n", daliy_fuel);
		}*/
			
		// =====================================================
		//printf("modem_time_sec_cur - modem_time_sec_last = [%d]\r\n",modem_time_sec_cur - modem_time_sec_last);
		//printf("report_interval = [%d]\r\n",report_interval);
		// =====================================================
		
		if ( ( modem_time_sec_cur - modem_time_sec_last ) >= get_server_send_interval() )
		{
			LOGT (LOG_TARGET, "PKT Send Time ~ ! \n");
			
			/*
				if ( ( flush_mds_packet_1(1) == -1 ) &&
					  ( get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT ))
				{
					// 전송할 데이터가 없고, 현재 power off 정책이라면... 실제 꺼도 된다.
					// 하지만.. 일단 끄지는 않는다.
					// 재시작시에 부팅이 오래걸리기 때문에 그냥 이상태로 냅둔다?
					// req_obd_ext_pwr_line_off();
					LOGT (LOG_TARGET, "REAL POWER OFF TIMING.. \n");
					LOGT (LOG_TARGET, "REAL POWER OFF TIMING.. \n");
					LOGT (LOG_TARGET, "REAL POWER OFF TIMING.. \n");
				}
			*/
			flush_mds_packet_1(1);
			
			
			// 일단 한개 전송후 default 로 돌린다.
			set_server_send_interval_default();
			modem_time_sec_last = modem_time_sec_cur;
		}
		
		// 쌓여있던 cli cmd 들을 처리한다.
		run_cli_cmd();
		
		watchdog_set_cur_ktime(eWdMain);
		
		// --------------------
		// hw 점검한다.
		// --------------------
		if (( main_time % HW_CHK_INTERVAL_SEC ) == 0)
		{
			hw_check_proc();
		}	
		
		
		sleep(1);
	}
}

void terminate_app_callback(void)
{
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}

/*
static int _process_poweroff(char *log)
{
    gps_valid_data_write();

    sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
    poweroff(log, strlen(log));
    return 0;
}
*/

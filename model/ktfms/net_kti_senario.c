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

#include <mdsapi/mds_api.h>

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

//#define DEBUG_VERBOS_ENABLE

int check_init_stat_1()
{
	static int check_obd_routine = 0;
	
	char car_vin[17+1] = {0,};
	char car_num[12+1] = {0,};
	char url_path[32+1] = {0,};
	
	int init_fail = 0;
	int obd_read_result = 0;
		
	int count = 0;
	
	obdData_t obd_data = {0,};
	
	if ( check_obd_routine == 1 )
		return;
		
	while(1)
	{
		init_fail = 0;
		
		memset(&obd_data, 0x00, sizeof(obdData_t));
		if ( req_obd_data(&obd_data) == OBD_RET_SUCCESS)
		{
			obd_read_result = 1;
			if ( proc_set_trip() == 0 )
			{
				clr_daily_info();
			}
			g_last_dev_stat.obd_stat = 1;
			g_last_dev_stat.obd_key = obd_data.car_key_stat;
			g_last_dev_stat.obd_rpm = obd_data.car_rpm;
			g_last_dev_stat.obd_speed = obd_data.car_speed;
			g_last_dev_stat.last_trip = obd_data.car_mileage_total;
		}
		else
		{
			led_off(ePOWER_LED);
			obd_read_result = 0;
			
			g_last_dev_stat.obd_stat = 0;
			g_last_dev_stat.obd_key = -1;
			g_last_dev_stat.obd_rpm = -1;
			g_last_dev_stat.obd_speed = -1;
			g_last_dev_stat.last_trip = -1;
		}
		
		watchdog_set_cur_ktime(eWdMain);
		
		memset(car_vin, 0x00, sizeof(car_vin));
		memset(car_num, 0x00, sizeof(car_num));
		memset(url_path, 0x00, sizeof(url_path));
		
		get_car_info_car_vin(car_vin);
		get_car_info_car_no(car_num);
		get_car_info_url_path(url_path);
		
		//led_set_all(eCOLOR_RED, count++ % 2);

		//req_obd_data(&obd_data);
#ifdef DEBUG_VERBOS_ENABLE
		printf("init 1 -> [%s]/[%s]\r\n",car_vin, DEFAULT_FMS_CAR_VIN);
#endif
		if ( strcmp(car_vin, DEFAULT_FMS_CAR_VIN ) == 0 ) 
		{
#ifdef DEBUG_VERBOS_ENABLE
			printf("init fail case 1\r\n");
#endif
			init_fail = 1;
			//continue;
		}
		
#ifdef DEBUG_VERBOS_ENABLE
		printf("init 2 -> [%s]/[%s]\r\n",car_num, DEFAULT_FMS_CAR_NUM);
#endif
		if ( strcmp(car_num, DEFAULT_FMS_CAR_NUM) == 0 ) 
		{
#ifdef DEBUG_VERBOS_ENABLE
			printf("init fail case 2\r\n");
#endif
			init_fail = 1;
			//continue;
		}
		
#ifdef DEBUG_VERBOS_ENABLE
		printf("init 3 -> [%s]/[%s]\r\n",url_path, DEFAULT_FMS_SUB_SERVER_PATH);
#endif
		if ( strcmp(url_path, DEFAULT_FMS_SUB_SERVER_PATH) == 0 ) 
		{
#ifdef DEBUG_VERBOS_ENABLE
			printf("init fail case 3\r\n");
#endif
			init_fail = 1;
			//continue;
		}
		
		
		if ( (init_fail == 1) )
		{
#ifdef DEBUG_VERBOS_ENABLE
			printf("init fail case 4 [%d] \r\n",(count % 2));
#endif
			// ------------------
			// led senario
			// ------------------
			if (obd_read_result == 1)
			{
				if (count++ % 2)
					led_on(ePOWER_LED, eCOLOR_RED);
				else
					led_off(ePOWER_LED);
			}
			
			sleep(1);
			continue;
		}
		else
		{
			led_on(ePOWER_LED, eCOLOR_GREEN);
			
			/*
			if ( nettool_get_state() == DEFINES_MDS_OK )
				led_on(eWCDMA_LED, eCOLOR_GREEN);
			else
				led_on(eWCDMA_LED, eCOLOR_YELLOW);
			*/
			check_obd_routine = 1;
			
			return 0;
		}
	}
}

int check_init_stat_2()
{
	static int check_obd_routine = 0;
	
	char car_vin[17+1] = {0,};
	char car_num[12+1] = {0,};
	char url_path[32+1] = {0,};
	
	int init_fail = 0;
	int obd_read_result = 0;
		
	int count = 0;
	
	odbGender_t cur_gender_spec = {0,};
	
	obdData_t obd_data = {0,};
	
	if ( check_obd_routine == 1 )
		return;
		
	while(1)
	{
		init_fail = 0;
		
		memset(&obd_data, 0x00, sizeof(obdData_t));
		if ( req_obd_data(&obd_data) == OBD_RET_SUCCESS)
		{
			obd_read_result = 1;
			if ( proc_set_trip() == 0 )
			{
				clr_daily_info();
			}
			g_last_dev_stat.obd_stat = 1;
			g_last_dev_stat.obd_key = obd_data.car_key_stat;
			g_last_dev_stat.obd_rpm = obd_data.car_rpm;
			g_last_dev_stat.obd_speed = obd_data.car_speed;
			g_last_dev_stat.last_trip = obd_data.car_mileage_total;
		}
		else
		{
			led_off(ePOWER_LED);
			obd_read_result = 0;
			
			g_last_dev_stat.obd_stat = 0;
			g_last_dev_stat.obd_key = -1;
			g_last_dev_stat.obd_rpm = -1;
			g_last_dev_stat.obd_speed = -1;
			g_last_dev_stat.last_trip = -1;
		}
		
		watchdog_set_cur_ktime(eWdMain);
		
		//sleep(1);
		
		if ( get_obd_gender_spec(&cur_gender_spec) == OBD_RET_SUCCESS)
		{
			printf("obd init case 2 ==> gender spec is ..\r\n");
			dbg_print_gender_spec(&cur_gender_spec);
			save_car_info_car_no(cur_gender_spec.gender_car_num);
			save_car_info_car_vin(cur_gender_spec.gender_car_vin);
			
			led_on(ePOWER_LED, eCOLOR_GREEN);
			return 0;
		}
		else
		{
			LOGE(LOG_TARGET, "obd init case 2 ==> FAIL\r\n");
			sleep(1);
			// ------------------
			// led senario
			// ------------------
			if (obd_read_result == 1)
			{
				if (count++ % 2)
					led_on(ePOWER_LED, eCOLOR_RED);
				else
					led_off(ePOWER_LED);
			}
			
			continue;
		}
		
	}
}

void pre_init()
{
	odbGender_t cur_gender_spec = {0,};
	odbGender_t last_gender_spec = {0,};
	
	obdData_t obd_data = {0,};
	/*
	char car_vin[17+1] = {0,};
	char car_num[12+1] = {0,};
	char url_path[32+1] = {0,};
	*/
	int obd_read_result = 0;
	int count = 0;
	
	/*
	memset(car_vin, 0x00, sizeof(car_vin));
	memset(car_num, 0x00, sizeof(car_num));
	memset(url_path, 0x00, sizeof(url_path));
	
	get_car_info_car_vin(car_vin);
	get_car_info_car_no(car_num);
	get_car_info_url_path(url_path);
	*/
	
	// 미연결시에는 그냥 실패로 리턴한다.
	if ( get_obd_gender_spec(&cur_gender_spec) != OBD_RET_SUCCESS)
		return -1;
	
	printf("pre init... start\r\n");
	
	dbg_print_gender_spec(&cur_gender_spec);
	
	if ( get_obd_info(&last_gender_spec) != 0 )
	{
		LOGI(LOG_TARGET, "NO Saved OBD Gender\r\n");
		return -1;
	}
	
	// check last spec..
	printf(" >> 1 get gender cur spec sn [%s]\r\n", cur_gender_spec.gender_sn);
	printf(" >> 1 get gender last spec sn [%s]\r\n", last_gender_spec.gender_sn);
	
	printf(" >> 2 get gender cur car no [%s]\r\n", cur_gender_spec.gender_car_num);
	printf(" >> 2 get gender last car no [%s]\r\n", last_gender_spec.gender_car_num);
	
	printf(" >> 3 get gender cur total trip [%lld]\r\n", cur_gender_spec.gender_total_trip);
	printf(" >> 3 get gender last total trip [%lld]\r\n", last_gender_spec.gender_total_trip);
					
	if ( strcmp(cur_gender_spec.gender_car_num, last_gender_spec.gender_car_num) == 0 )
	{
		LOGI(LOG_TARGET, "Same OBD Gender\r\n");
		return -2;
	}
	
	LOGE(LOG_TARGET, "OBD Gender Changed !! try gender setting\r\n");
	
	while(1)
	{
		memset(&obd_data, 0x00, sizeof(obdData_t));
		
		if ( req_obd_data(&obd_data) == OBD_RET_SUCCESS)
		{
			obd_read_result = 1;
			
			g_last_dev_stat.obd_stat = 1;
			g_last_dev_stat.obd_key = obd_data.car_key_stat;
			g_last_dev_stat.obd_rpm = obd_data.car_rpm;
			g_last_dev_stat.obd_speed = obd_data.car_speed;
			g_last_dev_stat.last_trip = obd_data.car_mileage_total;
		}
		else
		{
			obd_read_result = 0;

			led_off(ePOWER_LED);
			
			g_last_dev_stat.obd_stat = 0;
			g_last_dev_stat.obd_key = -1;
			g_last_dev_stat.obd_rpm = -1;
			g_last_dev_stat.obd_speed = -1;
			g_last_dev_stat.last_trip = -1;
		}
		
		if (obd_read_result == 1)
		{
			if (count++ % 2)
				led_on(ePOWER_LED, eCOLOR_GREEN);
			else
				led_off(ePOWER_LED);
		}
			
		if (obd_data.car_key_stat == 1 )
		{
			sleep(1);
			LOGE (LOG_TARGET, "pre init... fail case 1\r\n");
			printf("pre init... fail case 1 and retry\r\n");
			continue;
		}
		
		if ( set_obd_gender_spec(&last_gender_spec) != OBD_RET_SUCCESS )
		{
			sleep(1);
			LOGE(LOG_TARGET, "pre init... fail case 2 and retry\r\n");
			continue;
		}
		
		printf("pre init... success\r\n");
		
		if ( get_obd_gender_spec(&cur_gender_spec) == OBD_RET_SUCCESS)
		{
			LOGE(LOG_TARGET, "pre init... success \r\n");
			dbg_print_gender_spec(&cur_gender_spec);
		}
	
		return 0;
	}
}

void check_init()
{
	char car_vin[17+1] = {0,};
	char car_num[12+1] = {0,};
	char url_path[32+1] = {0,};
	
	int init_routine = 0;
	static int last_init_stat = 1;
	
	if ( last_init_stat == 0 )
		return;
	
	memset(car_vin, 0x00, sizeof(car_vin));
	memset(car_num, 0x00, sizeof(car_num));
	memset(url_path, 0x00, sizeof(url_path));
	
	get_car_info_car_vin(car_vin);
	get_car_info_car_no(car_num);
	get_car_info_url_path(url_path);
	
	printf("init 1 -> [%s]/[%s]\r\n",car_vin, DEFAULT_FMS_CAR_VIN);
	if ( strcmp(car_vin, DEFAULT_FMS_CAR_VIN ) == 0 ) 
	{
		printf("init fail case 1\r\n");
		init_routine = 1;
		//continue;
	}
	else if ( strcmp(car_vin, DEFAULT_FMS_CAR_VIN2 ) == 0 ) 
	{
		printf("init fail case 1\r\n");
		init_routine = 2;
		//continue;
	}
	
	
	switch (init_routine)
	{
		case 1:	// car vin 1
		{
			check_init_stat_1();
			break;
		}
		case 2: // car vin 2
		{
			check_init_stat_2();
			break;
		}
		default:
			break;
	}
	
	last_init_stat = 0;
}

void poweroff_proc_1()
{
	odbGender_t cur_gender_spec = {0,};
	
	char saved_car_num[12+1] = {0,};
	char saved_car_vin[17+1] = {0,};
	
	static int cur_pwr_off_stat = 0;
	
	int i = 0;
	int obd_ret_success = 0;
			
	if ( ( get_send_policy() == KT_FMS_SEND_POLICY__NONE ) || (get_send_policy() == KT_FMS_SEND_POLICY__PWR_OFF_EVENT) )
	{
		LOGI(LOG_TARGET, "CHK : pwr off time [%d] \n", cur_pwr_off_stat);
		
		if ( ( flush_mds_packet_1(1) == -1 ) && ( cur_pwr_off_stat++ > CHK_KEY_STAT_HOLD_SEC2 ) )
		{
			get_car_info_car_vin(saved_car_vin);
			get_car_info_car_no(saved_car_num);
			
			proc_set_trip();
			
			// 한번만 실행되게 한다.
			//cur_pwr_off_stat = 0;
			// kt 에서 쓸때없는거 넣지 말란다 ㅋ

			i = MAX_SET_OBD_INFO_RETRY_CNT;
			
			while(i --)
			{
				if ( get_obd_gender_spec(&cur_gender_spec) == OBD_RET_SUCCESS)
				{
					printf("poweroff routine ==> cur gender info...\r\n");
					dbg_print_gender_spec(&cur_gender_spec);
					obd_ret_success = 1;
					break;
				}
				sleep(1);
			}
			
			// gender 정보 읽는것을 실패했으면. 그냥 종료
			if ( obd_ret_success == 0)
			{
				sleep(1);
				printf("poweroff routine ==> get gender fail.. req poweroff\r\n");
				req_obd_ext_pwr_line_off();
				return;
			}
			
			// 여기까지왔으면 정보읽기 성공
			i = MAX_SET_OBD_INFO_RETRY_CNT;
			
			// 차량정보 번호를 모뎀의 내용과 일치하는지 확인한다.
			if ( ( strcmp(saved_car_num, cur_gender_spec.gender_car_num) != 0 ) ||
				 ( strcmp(saved_car_vin, cur_gender_spec.gender_car_vin) != 0 ) )
			{
				printf("poweroff routine ==> save to car info\r\n");
				while(i --)
				{
					if ( set_seco_obd_car_info(saved_car_num, saved_car_vin) == OBD_RET_SUCCESS)
						break;
					sleep(1);
				}
			}
			
			
			
			memset(cur_gender_spec.gender_car_num, 0x00, sizeof(cur_gender_spec.gender_car_num));
			strcpy(cur_gender_spec.gender_car_num, saved_car_num);
			
			memset(cur_gender_spec.gender_car_vin, 0x00, sizeof(cur_gender_spec.gender_car_vin));
			strcpy(cur_gender_spec.gender_car_vin, saved_car_vin);
			
			
			save_obd_info(&cur_gender_spec);
			
			// 최소 30분 유지
			req_obd_ext_pwr_line_off();
		}
	}
}




void hw_check_proc()
{
	static int last_conn_stat = 0;
	
	char temp_ant[10] = {0};
	int conn_stat = 0;

	printf("hw check time!!!!!!!\r\n");

	if (mds_api_gps_util_get_gps_ant() == DEFINES_MDS_API_OK )
	{
		set_hw_err_code(e_HW_ERR_CODE_GPS_ANT_DISCONN ,0);
	}		
	else
	{
		set_hw_err_code(e_HW_ERR_CODE_GPS_ANT_DISCONN ,0);
	}

	conn_stat =  nettool_get_state();

	if ( conn_stat != DEFINES_MDS_OK )
	{
		set_hw_err_code(e_HW_ERR_CODE_DATA_DISCON ,1);
		set_server_send_interval(10);	// 일단 dissconn 일때는 바로 처리
	}
	else
	{
		set_hw_err_code(e_HW_ERR_CODE_DATA_DISCON ,0);
	}

	// 네트워크 변화가 감지되면 기존 interval 은 초기화 시킨다.?
	if ( last_conn_stat != conn_stat)
	{
		LOGT (LOG_TARGET, "CONN STAT IS CHANGED \n");
		set_server_send_interval_default();
	}


	last_conn_stat = conn_stat;

}

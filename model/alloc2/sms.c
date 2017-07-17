#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <board/power.h>
#include <base/devel.h>

#include <config.h>
#include <logd_rpc.h>

#include "sms.h"

#include "alloc2_pkt.h"
#include "alloc2_senario.h"

#include "allkey_bcm_1.h"

#include <base/gpstool.h>
#include <base/mileage.h>

#include <mdsapi/mds_api.h>

#include "alloc2_obd_mgr.h"
#include "alloc2_bcm_mgr.h"
#include "alloc2_daily_info.h"

#define SMS_CMD_ALLOC2_RET_SUCCESS		0
#define SMS_CMD_ALLOC2_RET_FAIL			1
#define SMS_CMD_ALLOC2_RET_REQ_RESET	2
#define SMS_CMD_ALLOC2_RET_REQ_INIT		3

static int _sms_cmd_proc_set__dummy_test_success(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
	printf( "[MODEL SMS PROC] : [%s] start \n",__func__);

	for(i = 0; i < argc; i++)
	{
		printf("  + SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	


	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

static int _sms_cmd_proc_set__dummy_test_fail(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	
	printf( "[MODEL SMS PROC] : [%s] start \n",__func__);

	for(i = 0; i < argc; i++)
	{
		printf("  + SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	


	return SMS_CMD_ALLOC2_RET_FAIL;
}

// 중계서버의 IP:port	111.112.113.114:1234
static int _sms_cmd_proc_set__server_info(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	char *tr;
    char token_0[ ] = ":\r\n";
	//char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_cmd = argv[1];
	if (p_cmd == NULL)
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return SMS_CMD_ALLOC2_RET_FAIL;
	strcpy(p_mdm_setting_val->proxy_server_ip, tr);
		
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return SMS_CMD_ALLOC2_RET_FAIL;
	if ( mds_api_check_is_num(tr, strlen(tr)) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;
	p_mdm_setting_val->proxy_server_port = atoi(tr);

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 위치정보 보고 주기 설정(IG1 On)	1 ~ 7200 secs
static int _sms_cmd_proc_set__gps_report_keyon_interval(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_mdm_setting_val->key_on_gps_report_interval = atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 위치정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
static int _sms_cmd_proc_set__gps_report_keyoff_interval(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_mdm_setting_val->key_off_gps_report_interval = atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// GPS기반 누적거리 설정	meter단위
static int _sms_cmd_proc_set__mdm_gps_total_distance(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	mileage_set_m(atoi(argv[1]));
	mileage_write();

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 저전압 경고 설정	0.1단위 예)12.0 -> 120
static int _sms_cmd_proc_set__car_low_batt(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_mdm_setting_val->low_batt_voltage= atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 주기적 리셋 타임 설정	분단위 예)03:20 -> 320
static int _sms_cmd_proc_set__mdm_reset_time(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	p_mdm_setting_val = get_mdm_setting_val();

	if ( p_mdm_setting_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_mdm_setting_val->mdm_reset_interval= atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
	
}

// 시동 차단 설정 명령	0 : 시동차단 설정 1 : 시동차단 해제
static int _sms_cmd_proc_set__engine_off(int argc, char* argv[], const char* phonenum)
{
	//ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	//p_mdm_setting_val = get_mdm_setting_val();

	int opt_val = 0;

	//if ( p_mdm_setting_val == NULL )
	//	return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( get_car_ctrl_enable() == CAR_CTRL_DISABLE ) 
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);

	// 시동차단과 관련해서는 아직 동작이 불가능하다.
	// bcm 에 그런기능이 없네?
	if ( opt_val == 0 ) // 시동차단 설정
		;
	else if ( opt_val == 1 ) // 시동차단 해제
		;
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	
	return SMS_CMD_ALLOC2_RET_FAIL;
	
}

// 단말기 상태 요청	0
static int _sms_cmd_proc_set__req_mdm_stat(int argc, char* argv[], const char* phonenum)
{
	// ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	// p_mdm_setting_val = get_mdm_setting_val();

	int opt_val = 0;

	//if ( p_mdm_setting_val == NULL )
	//	return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	if ( opt_val == 0 ) // 단말 상태요청
	{
		int evt_code = e_evt_code_normal;
		sender_add_data_to_buffer(e_mdm_stat_evt, &evt_code, ePIPE_2);;
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 단말기 초기화 명령	0
static int _sms_cmd_proc_set__req_mdm_init(int argc, char* argv[], const char* phonenum)
{
	// ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	// p_mdm_setting_val = get_mdm_setting_val();

	int opt_val = 0;

	//if ( p_mdm_setting_val == NULL )
	//	return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	if ( opt_val == 0 ) // 단말 초기화
	{
		return SMS_CMD_ALLOC2_RET_REQ_INIT; //set_cur_status(e_STAT_START);
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// 단말기 재부팅 명령	0
static int _sms_cmd_proc_set__req_mdm_reset(int argc, char* argv[], const char* phonenum)
{
	//ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;
	//p_mdm_setting_val = get_mdm_setting_val();

	int opt_val = 0;

	//if ( p_mdm_setting_val == NULL )
	//	return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	// 단말 리셋 : 
	// 단말리셋의 경우 sms proc 를 리턴하지말구 그냥 여기서 다처리 
	if ( opt_val == 0 ) 
		return SMS_CMD_ALLOC2_RET_REQ_RESET;
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_FAIL;
}

// OBD정보 보고 주기 설정 (IG1 On)	1 ~ 7200 secs
static int _sms_cmd_proc_set__obd_keyon_report_interval(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__OBD_DEV_INFO* p_obd_dev_info_val = NULL;
	p_obd_dev_info_val = get_obd_dev_info();

	if ( p_obd_dev_info_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_obd_dev_info_val->obd_recv_keyon_interval= atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// OBD정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
static int _sms_cmd_proc_set__obd_keyoff_report_interval(int argc, char* argv[], const char* phonenum)
{
	ALLOC_PKT_RECV__OBD_DEV_INFO* p_obd_dev_info_val = NULL;
	p_obd_dev_info_val = get_obd_dev_info();

	if ( p_obd_dev_info_val == NULL )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	p_obd_dev_info_val->obd_recv_keyoff_interval= atoi(argv[1]);
	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// Door Lock	0
static int _sms_cmd_proc_set__bcm_door_lock(int argc, char* argv[], const char* phonenum)
{
	int opt_val = 0;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( get_car_ctrl_enable() == CAR_CTRL_DISABLE ) 
		return SMS_CMD_ALLOC2_RET_FAIL;
		
	opt_val = atoi(argv[1]);
	
	// Door Lock	0
	if ( opt_val == 0 ) 
	{
		if ( allkey_bcm_ctr__door_lock(1) != ALLKEY_BCM_RET_SUCCESS )
			return SMS_CMD_ALLOC2_RET_FAIL;
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// Door Unlock	0
static int _sms_cmd_proc_set__bcm_door_unlock(int argc, char* argv[], const char* phonenum)
{
int opt_val = 0;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( get_car_ctrl_enable() == CAR_CTRL_DISABLE ) 
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	// Door Lock	0
	if ( opt_val == 0 ) 
	{
		if ( allkey_bcm_ctr__door_lock(0) != ALLKEY_BCM_RET_SUCCESS )
			return SMS_CMD_ALLOC2_RET_FAIL;
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// Horn	0
static int _sms_cmd_proc_set__bcm_horn_on(int argc, char* argv[], const char* phonenum)
{
	int opt_val = 0;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( get_car_ctrl_enable() == CAR_CTRL_DISABLE ) 
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	// Door Lock	0
	if ( opt_val == 0 ) 
	{
		if ( allkey_bcm_ctr__horn_on(1) != ALLKEY_BCM_RET_SUCCESS )
			return SMS_CMD_ALLOC2_RET_FAIL;
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}

// Emergency Light	0
static int _sms_cmd_proc_set__bcm_light_on(int argc, char* argv[], const char* phonenum)
{
	int opt_val = 0;
	
	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	if ( get_car_ctrl_enable() == CAR_CTRL_DISABLE ) 
		return SMS_CMD_ALLOC2_RET_FAIL;

	opt_val = atoi(argv[1]);
	
	// Door Lock	0
	if ( opt_val == 0 ) 
	{
		if ( allkey_bcm_ctr__light_on(1) != ALLKEY_BCM_RET_SUCCESS )
			return SMS_CMD_ALLOC2_RET_FAIL;
	}
	else
		return SMS_CMD_ALLOC2_RET_FAIL;

	return SMS_CMD_ALLOC2_RET_SUCCESS;
}


static int _sms_cmd_proc_set__obd_total_distance(int argc, char* argv[], const char* phonenum)
{
	int opt_val = 0;
	int cmd_retry_cnt = 10;

	if ( argc != 2 )
		return SMS_CMD_ALLOC2_RET_FAIL;
	
	if ( mds_api_check_is_num(argv[1], strlen(argv[1])) != DEFINES_MDS_API_OK )
		return SMS_CMD_ALLOC2_RET_FAIL;

	
	alloc2_obd_mgr__set_cmd_proc(SECO_OBD_CMD_TYPE__SET_DISTANCE, argv[1]);

	while(cmd_retry_cnt--)
	{
		if ( alloc2_obd_mgr__get_cmd_proc_result(SECO_OBD_CMD_TYPE__SET_DISTANCE) == SECO_OBD_CMD_RET__SUCCESS )
		{
			printf(" >>>> sms proc :: set obd distance success!! \r\n");
			return SMS_CMD_ALLOC2_RET_SUCCESS;
		}
		sleep(1);
	}

	printf(" >>>> sms proc :: set obd distance fail!! \r\n");
	return SMS_CMD_ALLOC2_RET_FAIL;

}

static SMS_CMD_FUNC_T sms_cmd_func[] =
{
	{eSMS_CMD_SET__SERVER_INFO, "&11", _sms_cmd_proc_set__server_info} , // 중계서버의 IP:port	111.112.113.114:1234
	{eSMS_CMD_SET__MDM_GPS_REPORT_KEYON_INTERVAL, "&12", _sms_cmd_proc_set__gps_report_keyon_interval} , // 위치정보 보고 주기 설정(IG1 On)	1 ~ 7200 secs
    {eSMS_CMD_SET__MDM_GPS_REPORT_KEYOFF_INTERVAL, "&13", _sms_cmd_proc_set__gps_report_keyoff_interval},  // 위치정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
    {eSMS_CMD_SET__MDM_GPS_TOTAL_DISTANCE, "&17", _sms_cmd_proc_set__mdm_gps_total_distance},  // GPS기반 누적거리 설정	meter단위
    {eSMS_CMD_SET__CAR_LOW_BATT_VOLT, "&18", _sms_cmd_proc_set__car_low_batt},  // 저전압 경고 설정	0.1단위 예)12.0 -> 120
    {eSMS_CMD_SET__MDM_RESET_TIME, "&19", _sms_cmd_proc_set__mdm_reset_time},  // 주기적 리셋 타임 설정	분단위 예)03:20 -> 320
    {eSMS_CMD_SET__BCM_HORN_CNT, "&20", _sms_cmd_proc_set__dummy_test_fail},  // 경적횟수 설정	1 ~ 10
    {eSMS_CMD_SET__BCM_LIGHT_CNT, "&21", _sms_cmd_proc_set__dummy_test_fail},  // 비상등 횟수 설정	1 ~ 10
    {eSMS_CMD_SET__OBD_OVERSPEED_KMH, "&22", _sms_cmd_proc_set__dummy_test_fail},  // 과속기준 속도 설정	Km/h 단위
    {eSMS_CMD_SET__OBD_OVERSPEED_TIME, "&23", _sms_cmd_proc_set__dummy_test_fail},  // 과속기준 시간 설정	secs
    {eSMS_CMD_SET__CAR_ENGINE_OFF, "&30", _sms_cmd_proc_set__engine_off},  // 시동 차단 설정 명령	0 : 시동차단 설정 1 : 시동차단 해제
    {eSMS_CMD_SET__MDM_STAT_REQ, "&40", _sms_cmd_proc_set__req_mdm_stat},  // 단말기 상태 요청	0
    {eSMS_CMD_SET__BCM_KNOCK_REGI_REQ, "&45", _sms_cmd_proc_set__dummy_test_fail},  // 마스터 키 등록 요청	0
    {eSMS_CMD_SET__BCM_MASTERKEY_REGI_REQ, "&46", _sms_cmd_proc_set__dummy_test_fail},  // 마스터 카드 등록 요청	0
    {eSMS_CMD_SET__BCM_RESERVATION_INFO_REQ, "&47", _sms_cmd_proc_set__dummy_test_fail},  // 예약정보 요청	0
    {eSMS_CMD_SET__MDM_REINIT, "&50", _sms_cmd_proc_set__req_mdm_init},  // 단말기 초기화 명령	0
    {eSMS_CMD_SET__MDM_RESET, "&60", _sms_cmd_proc_set__req_mdm_reset},  // 단말기 재부팅 명령	0
    {eSMS_CMD_SET__OBD_REPORT_KEY_ON_INTERVAL, "&71", _sms_cmd_proc_set__obd_keyon_report_interval},  // OBD정보 보고 주기 설정 (IG1 On)	1 ~ 7200 secs
    {eSMS_CMD_SET__OBD_REPORT_KEY_OFF_INTERVAL, "&72", _sms_cmd_proc_set__obd_keyoff_report_interval},  // OBD정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
    {eSMS_CMD_SET__OBD_TOTAL_DISTANCE_SET, "&73", _sms_cmd_proc_set__obd_total_distance},  // OBD기반 누적거리 설정	meter단위
    {eSMS_CMD_SET__BCM_DOOR_LOCK, "&81", _sms_cmd_proc_set__bcm_door_lock},  // Door Lock	0
    {eSMS_CMD_SET__BCM_DOOR_UNLOCK, "&82", _sms_cmd_proc_set__bcm_door_unlock},  // Door Unlock	0
    {eSMS_CMD_SET__BCM_HORN, "&83", _sms_cmd_proc_set__bcm_horn_on},  // Horn	0
    {eSMS_CMD_SET__BCM_LIGHT, "&84", _sms_cmd_proc_set__bcm_light_on},  // Emergency Light	0
    {eSMS_CMD_SET__FIRM_UPDATE_MDM, "&91", _sms_cmd_proc_set__dummy_test_fail},  // 단말 펌웨어	버전 
    {eSMS_CMD_SET__FIRM_UPDATE_OBD, "&92", _sms_cmd_proc_set__dummy_test_fail},  // OBD 펌웨어	버전
    {eSMS_CMD_SET__OBD_DEV_INIT, "&93", _sms_cmd_proc_set__dummy_test_fail},  // OBD 초기화	Car code
    {eSMS_CMD_SET__FIRM_UPDATE_ALLKEY_BCM, "&94", _sms_cmd_proc_set__dummy_test_fail},  // All key 펌웨어	버전
    {eSMS_CMD_SET__FIRM_UPDATE_BCM, "&95", _sms_cmd_proc_set__dummy_test_fail},  // BCM 펌웨어	버전
    {eSMS_CMD_SET__FIRM_UPDATE_DTG, "&96", _sms_cmd_proc_set__dummy_test_fail},  // DTG 펌웨어	버전
    {eSMS_CMD_SET__FIRM_UPDATE_KNOCK, "&97", _sms_cmd_proc_set__dummy_test_fail},  // 노크센서 펌웨어	버전
};

#define MAX_SUPPORT_SMS_SUB	10

int parse_model_sms(const char *time, const char *phonenum, const char *sms)
{
//	int msg_type = 0;
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};

	char *base = 0;
	int len = 0;
	int i = 0;
	int j = 0;
	
	char original_sms_backup[80] = {0,};

	int sms_proc_result = SMS_CMD_ALLOC2_RET_SUCCESS;
	int sms_pkt_result_code = 0;

	int sms_pwd_idx = 0;

	char remove_cr_sms[80] = {0,};
	int remove_cr_sms_len = 0;

	int need_to_reset_flag = 0;
	int need_to_init_flag = 0;

	// cr / lr 은 제거한다.
	remove_cr_sms_len = mds_api_remove_cr(sms, remove_cr_sms, 80);
	strcpy(original_sms_backup, remove_cr_sms);
	
	devel_webdm_send_log("[DEBUG] sms recv [%s] [%s]\r\n", phonenum, sms);

	printf("remove_cr_sms is [%s] / [%c] / [%c] \r\n", remove_cr_sms, remove_cr_sms[0] , remove_cr_sms[remove_cr_sms_len-1] );
	// 처음문자와 끝문자는 {} 이여야한다.
	if ( (remove_cr_sms[0] != '{') || (remove_cr_sms[remove_cr_sms_len-1] != '}') )
	{
		LOGE(eSVC_MODEL, "[MODLE SMS] invalid sms format 1 [%s]\n", original_sms_backup);
		return -1;
	}

	// } 캐릭터 강제 제거
	remove_cr_sms[remove_cr_sms_len-1] = 0;

	// original sms
	

	base = remove_cr_sms+1; // 처음문자가 { 이므로 제거
	len = strlen(remove_cr_sms)-1; // 끝문자와 첫문자는 제거한 길이
	
	model_argv[model_argc++] = base;

	while(model_argc <= 32 && len--) 
	{
		switch(*base) {
			case ',':
				*base = '\0';
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case '\r':
			case '\n':
				*base = '\0';
				break;
			default:
				break;
		}
		base++;
	}
	//model_argc--;
	
	for(i = 0; i < model_argc; i++)
	{
		printf( "%d %s\n", i, model_argv[i]);
	}

	if( (model_argv[0] == NULL))
	{
		LOGE(eSVC_MODEL, "[MODLE SMS] invalid sms format 2 [%s]\n", original_sms_backup);
		return -1;
	}

	// --------------------------
	sms_pwd_idx = model_argc - 1;

	if ( strncasecmp(SMS_CMD_ALLOC2_PWD, model_argv[sms_pwd_idx], strlen(model_argv[sms_pwd_idx]) ) != 0 )
	{
		printf("sms pwd invalid \r\n");
		LOGE(eSVC_MODEL, "  + SMS PWD invalid [%s] / [%s]\n", model_argv[sms_pwd_idx], SMS_CMD_ALLOC2_PWD);
		return -1;

		int sms_cmd_ret = SMS_CMD_ALLOC2_RET_FAIL;
		int sms_code = 0;
		int pkt_evt_code = e_evt_code_normal;
		ALLOC_PKT_SEND__SMS_PKT_ARG sms_pkt_arg = {0,};

		sms_pkt_arg.sms_cmd_seq = 0;
		sms_pkt_arg.sms_cmd_code = sms_code;
		sms_pkt_arg.sms_cmd_success = sms_cmd_ret;
		strcpy(sms_pkt_arg.sms_cmd_contents, original_sms_backup);

		sender_add_data_to_buffer(e_mdm_stat_evt, &pkt_evt_code, ePIPE_2);
		sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
		sender_add_data_to_buffer(e_sms_recv_info, &sms_pkt_arg, ePIPE_2);
		
		return SMS_CMD_ALLOC2_RET_FAIL;
	}

	// 마지막에 항상 인증코드가 오니까..
	// 인증코드는 의미없으므로 삭제
	model_argc--;

	// 
	// Command prefix always place in 1st argv.
	
	for(i = 0; i < model_argc; i++)
	{
		printf(" >> sms command chk target [%s]\r\n",model_argv[i]);
		for(j = 0; j < MAX_SMS_CMD; j++)
		{
			if  (!( strncasecmp ( model_argv[i], sms_cmd_func[j].cmd, strlen(sms_cmd_func[j].cmd) ) ))
			{
				// found command
				sms_pkt_result_code = get_sms_pkt_cmd_code(atoi(sms_cmd_func[i].cmd+1));
				printf(" >> sms cmd found target [%s] / code [%d]\r\n",model_argv[i],sms_pkt_result_code);
				
				if ( sms_cmd_func[j].proc_func != NULL )
				{
					// argc는 항상 2개다.
					printf("-----------------------------\r\n");
					int proc_ret = sms_cmd_func[j].proc_func(2, &model_argv[i], phonenum);

					if ( proc_ret == SMS_CMD_ALLOC2_RET_FAIL )
					{
						printf(" >> sms cmd proc result fail [%s] \r\n",model_argv[i]);
						sms_proc_result = SMS_CMD_ALLOC2_RET_FAIL;
					}
					else if ( proc_ret == SMS_CMD_ALLOC2_RET_REQ_RESET ) 
					{
						need_to_reset_flag = 1;
					}
					else if ( proc_ret == SMS_CMD_ALLOC2_RET_REQ_INIT )
					{
						need_to_init_flag = 1;
					}
					else
					{
						printf(" >> sms cmd proc result success [%s] \r\n",model_argv[i]);
					}
					// 그리고 argc2가 두개씩이니...
				}
				i++;
			}
		}
	}

	{
		int sms_cmd_ret = sms_proc_result;
		int sms_code = sms_pkt_result_code;
		int pkt_evt_code = e_evt_code_normal;
		ALLOC_PKT_SEND__SMS_PKT_ARG sms_pkt_arg = {0,};

		sms_pkt_arg.sms_cmd_seq = 0;
		sms_pkt_arg.sms_cmd_code = sms_code;
		sms_pkt_arg.sms_cmd_success = sms_cmd_ret;
		strcpy(sms_pkt_arg.sms_cmd_contents, original_sms_backup);

		if ( get_cur_status() > e_SEND_TO_SETTING_INFO_ING )
		{
			sender_add_data_to_buffer(e_mdm_stat_evt, &pkt_evt_code, ePIPE_2);
			sender_add_data_to_buffer(e_mdm_gps_info, NULL, ePIPE_2);
			sender_add_data_to_buffer(e_sms_recv_info, &sms_pkt_arg, ePIPE_2);
		}
		else
			LOGE(eSVC_MODEL, "[MODEL SMS] cannot send pkt : senario is not init");
		
		if ( need_to_reset_flag == 1 )
		{
			alloc2_poweroff_proc("sms reset");
		}

		if ( need_to_init_flag == 1 )
			set_cur_status(e_STAT_START);
	}
	
	return ret;
}

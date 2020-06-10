#include <stdio.h>
#include <string.h>
#include <time.h>

#include <board/led.h>
#include <board/modem-time.h>
#include <util/poweroff.h>
#include <util/storage.h>
#include <util/tools.h>

#include <callback.h>

#include "kt_fota_svc/kt_fs_send.h"
#include "kt_fota_svc/kt_fs_udf.h"
#include "kt_fota_svc/kt_fs_parser.h"
#include "kt_fota.h"
#include "kt_fota_svc/kt_dmc_proc.h"
#include "kt_fota_config.h"

#include <at/at_util.h>

#include "logd/logd_rpc.h"

#define LOG_TARGET eSVC_BASE

void _deinit_essential_functions(void);


typedef enum {
	KT_DEV_FOTA_NO_EVT_NUM = 0,

	KT_USIM_CARD_REMOVE      = 40,
	KT_DATA_DATA_SEND_PUASE = 202, //"$$ALT: 202, #8, Operator Determined Barring"
	/*
	KT_DATA_GMM_REJECT      = 302,
	KT_FOTA_SMS_RECEIVE 	= 770,
	KT_FOTA_SESSION_OPEN 	= 762,
	KT_FOTA_SESSION_CLOSE 	= 763,
	KT_FOTA_DOWNLOAD_START 	= 764,
	KT_FOTA_DOWNLOADING 	= 765,
	KT_FOTA_DOWNLOAD_END 	= 766,
	*/

	KT_DEV_FOTA_SVC_FOTA_REQ_PUSH,
	KT_DEV_FOTA_SVC_DEVICE_RESET,
	KT_DEV_FOTA_SVC_DEVICE_RESET2,
	KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET,
	KT_DEV_FOTA_SVC_DEVICE_QTY,
	KT_DEV_FOTA_SVC_MODEM_QTY,

	KT_MODEM_FOTA_DMC_PROC_1,
	KT_MODEM_FOTA_DMC_PROC_2,

	KT_DEV_FOTA_MAX_NUM
}KT_FOTA_PROCESS;

typedef struct
{
	int evt_num;
    unsigned char * cmd;
}KT_FOTA_STRING_TYPE_T;

static KT_FOTA_STRING_TYPE_T fota_push_type[]=
{
	{KT_DEV_FOTA_SVC_DEVICE_QTY, "$$SFOTA:01"}, // 통합품질정보
	{KT_DEV_FOTA_SVC_MODEM_QTY, "$$SFOTA:02"}, // 모뎀품질정보 // $$SFOTA:99
	{KT_DEV_FOTA_SVC_DEVICE_RESET, "$$SFOTA:10"}, // 모뎀품질정보 // $$SFOTA:99
	{KT_DEV_FOTA_SVC_DEVICE_RESET2, "$$SFOTA:08"}, // 모뎀품질정보 // $$SFOTA:99
	{KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET, "$$SFOTA:09"}, // 모뎀품질정보 // $$SFOTA:99
	{KT_MODEM_FOTA_DMC_PROC_1, KT_MODEM_FOTA_DMC_PROC_1_STR}, // APPS_AT$$KT_DMC_EXECUTE=E3087F6B0C0EE24A28C854302A60464702C800000028DB094B545349445F303031,
	{KT_MODEM_FOTA_DMC_PROC_2, KT_MODEM_FOTA_DMC_PROC_2_STR}, 
	{KT_DEV_FOTA_MAX_NUM, "NULL"} // 모뎀품질정보 // $$SFOTA:99
};


int get_fota_noti_type(const char* msg)
{
	char temp_buff[256] = {0,};
	int i = 0;

	mds_api_remove_cr(msg, temp_buff, 256);

	for ( i = 0 ; i < KT_DEV_FOTA_MAX_NUM ; i++ )
	{
		if ( strncmp(temp_buff, fota_push_type[i].cmd, strlen(fota_push_type[i].cmd) ) == 0 )
			return fota_push_type[i].evt_num;
		else if ( strcmp(fota_push_type[i].cmd, "NULL") == 0 )
			return KT_DEV_FOTA_NO_EVT_NUM;
	}

}

int bool_fota_init = 0;

void kt_fota_init(void)
{
	
	if ( strcmp(get_kt_fota_dm_server_ip_addr(), KT_FOTA_TEST_SVR_DM_IP) == 0 )
		set_modem_fota_testmode_for_tl500k(TELADIN_DMS_SETTING_TEST_MODE);
	else
		set_modem_fota_testmode_for_tl500k(TELADIN_DMS_SETTING_NONTEST_MODE);


	if(bool_fota_init == 1)
	{
		return;
	}
	
	LOGI(LOG_TARGET, "[KT FOTA SVC] - send :: fota req", __func__);
	if(send_fota_req_packet(ePOLLING_MODE, eHTTP, 30) < 0)
	{
		return;
	}

	LOGI(LOG_TARGET, "[KT FOTA SVC] - send :: device on ", __func__);
	if(send_device_on_packet(30) < 0)
	{
		return;
	}

	LOGI(LOG_TARGET, "[KT FOTA SVC] - run :: teladin fota svc", __func__);
	system("/kt_dms/bin/kt_svc &");	

	bool_fota_init = 1;
	
	LOGI(LOG_TARGET, "[KT FOTA SVC] - send :: device,modem qty info", __func__);
	send_qty_packet(ePOLLING_MODE, eDEVICE_MODEM_QTY_MODE, 30);

}

void kt_fota_send(void)
{
	if(bool_fota_init == 0)
	{
		kt_fota_init();	
	}

	send_qty_packet(ePOLLING_MODE, eDEVICE_MODEM_QTY_MODE, 30);
}

void kt_fota_deinit(void)
{
	if(bool_fota_init == 0)
	{
		kt_fota_init();
	}
	
	send_device_off_packet(30);
}

int kt_fota_check_cycle(void)
{
	static time_t last_cycle = 0;
	time_t cur_time = 0;

	if(get_kt_fota_qry_report() <= 0)
	{
		return 0;
	}

	cur_time = get_modem_time_utc_sec();
	if(cur_time == 0)
	{
		return -1;
	}
	
	if(last_cycle == 0)
	{
		last_cycle = cur_time;
		
		return 0;
	}
	
	if(cur_time - last_cycle >= get_kt_fota_qry_report())
	{
		last_cycle = cur_time;
		
		return 1;
	}
	return 0;
}

int KT_FOTA_NOTI_RECEIVE(char* buf)
{
	int noti_num = get_fota_noti_type(buf);
	
	LOGI(LOG_TARGET, "[KT FOTA SVC] -  [%s] / [%d]\r\n", buf, noti_num);
	printf("KT FOTA RECIVE EVT :: [%s] / [%d]\r\n", buf, noti_num);
	
	switch(noti_num) {
////start--------for kt fota svc

/*
		case KT_USIM_CARD_REMOVE:
		case KT_DATA_GMM_REJECT:
			//wcdma_error_led_notification();
			break;
		case KT_DATA_DATA_SEND_PUASE:
			//wcdma_pause_error_led_notification();
			break;
*/
		case KT_DEV_FOTA_SVC_FOTA_REQ_PUSH:
			send_fota_req_packet(ePUSH_MODE, eHTTP, 30);
			break;
		case KT_DEV_FOTA_SVC_DEVICE_QTY:
			send_qty_packet(ePUSH_MODE, eDEVICE_MODEM_QTY_MODE, 30);
			break;
		// case KT_DEV_FOTA_SVC_MODEM_QTY:
		//	send_qty_packet(ePUSH_MODE, eMODEM_QTY_MODE, 30);
		//	break;
		case KT_DEV_FOTA_SVC_DEVICE_RESET:
		case KT_DEV_FOTA_SVC_DEVICE_RESET2:
		case KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET:
			_deinit_essential_functions();
			terminate_app_callback();
			poweroff(__FUNCTION__, sizeof(__FUNCTION__));
			break;
////end--------for kt fota svc
		case KT_MODEM_FOTA_DMC_PROC_1:
		case KT_MODEM_FOTA_DMC_PROC_2:
			LOGE(LOG_TARGET, "[KT FOTA SVC] noti proc :: dmc start\r\n");
			kt_dmc_proc_for_tl500k(buf);
			break;
		default:
			break;
	}
	return 0;
}

void check_atl_buffer_data(char *buffer)
{
		int retry_cnt = 10;
		char *pAtcmd = NULL;
		char token_1[ ] = "\r\n";
		char *temp_bp = NULL;
		char *tr;

		printf("3333\n");

		tr = strtok_r(buffer, token_1, &temp_bp);

		printf("4444\n");

		if(tr == NULL)
					return;

		printf("tr = [%s]\n", tr);

		while(retry_cnt-- > 0) {
					pAtcmd = strstr(tr, "$$ALT:");

					if(pAtcmd != NULL) {
							printf("pAtcmd = [%s]\n", pAtcmd);
							KT_FOTA_NOTI_RECEIVE(pAtcmd);
					}
					
					tr = strtok_r(NULL, token_1, &temp_bp);
					if(tr == NULL)
			break;
		}
}


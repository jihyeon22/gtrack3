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
#include "kt_fota_config.h"

void _deinit_essential_functions(void);

typedef enum {

	KT_USIM_CARD_REMOVE      = 40,
	KT_DATA_DATA_SEND_PUASE = 202, //"$$ALT: 202, #8, Operator Determined Barring"
	KT_DATA_GMM_REJECT      = 302,
	KT_FOTA_SMS_RECEIVE 	= 770,
	KT_FOTA_SESSION_OPEN 	= 762,
	KT_FOTA_SESSION_CLOSE 	= 763,
	KT_FOTA_DOWNLOAD_START 	= 764,
	KT_FOTA_DOWNLOADING 	= 765,
	KT_FOTA_DOWNLOAD_END 	= 766,

	KT_DEV_FOTA_SVC_FOTA_REQ_PUSH = 782,
	KT_DEV_FOTA_SVC_DEVICE_RESET  = 780,
	KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET  = 781,
	KT_DEV_FOTA_SVC_DEVICE_QTY    = 783,
	KT_DEV_FOTA_SVC_MODEM_QTY     = 784,
}KT_FOTA_PROCESS;

int bool_fota_init = 0;

void kt_fota_init(void)
{
	if(bool_fota_init == 1)
	{
		return;
	}
	
	if(send_fota_req_packet(ePOLLING_MODE, eHTTP, 30) < 0)
	{
		return;
	}

	if(send_device_on_packet(30) < 0)
	{
		return;
	}	

	bool_fota_init = 1;
	
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

int KT_FOTA_NOTI_RECEIVE(char *buf)
{
	char *bp;
	char token[ ] = ",";
	char *tr;
	int noti_num;

	printf("ATD, KT_FOTA_NOTI_RECEIVE = [%s]\n", buf);
	tr = strtok_r(&buf[7], token, &bp);
	if(tr != NULL)
		printf("1. KT_FOTA_NOTI_RECEIVE = [%s]\n", tr);

	noti_num = atoi(tr);
	printf("ATD, KT FOTA NOTI NUMBER = [%d]\n", noti_num);
	
	switch(noti_num) {
////start--------for kt fota svc
		case KT_USIM_CARD_REMOVE:
		case KT_DATA_GMM_REJECT:
			//wcdma_error_led_notification();
			break;
		case KT_DATA_DATA_SEND_PUASE:
			//wcdma_pause_error_led_notification();
			break;
		case KT_DEV_FOTA_SVC_FOTA_REQ_PUSH:
			send_fota_req_packet(ePUSH_MODE, eHTTP, 30);
			break;
		case KT_DEV_FOTA_SVC_DEVICE_QTY:
			send_qty_packet(ePUSH_MODE, eDEVICE_MODEM_QTY_MODE, 30);
			break;
		case KT_DEV_FOTA_SVC_MODEM_QTY:
			send_qty_packet(ePUSH_MODE, eMODEM_QTY_MODE, 30);
			break;
		case KT_DEV_FOTA_SVC_DEVICE_RESET:
		case KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET:
			_deinit_essential_functions();
			terminate_app_callback();
			poweroff(__FUNCTION__, sizeof(__FUNCTION__));
			break;
////end--------for kt fota svc
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


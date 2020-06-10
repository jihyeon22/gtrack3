<<<<<<< HEAD


#include <stdio.h>
#include <stdlib.h>

#include "ktth_senario.h"

#include <base/sender.h>
#include <board/board_system.h>
#include "logd/logd_rpc.h"

#include <kt_thermal_mdt800/packet.h>
#include <kt_thermal_mdt800/gpsmng.h>
#include <kt_thermal_mdt800/gps_utill.h>
#include <kt_thermal_mdt800/geofence.h>
#include <kt_thermal_mdt800/file_mileage.h>

#define LOG_TARGET eSVC_MODEL


btn_status_t g_btn_stat;
kttn_resum_data_t g_resume_data;

void ktth_sernaio__load_resume_data()
{
	kttn_resum_data_t resume_data = {0,};
	int ret = 0;

    ktth_sernaio__init_btn();

	if ( storage_load_file(KTTH_RESUME_DATA_PATH, &resume_data, sizeof(resume_data)) >= 0 )
	{
        memcpy(&g_btn_stat, &resume_data.btn_data, sizeof(btn_status_t));
	}
}


void ktth_sernaio__save_resume_data()
{
	int try_cnt = 4;
	kttn_resum_data_t resume_data = {0,};

    memcpy(&resume_data.btn_data, &g_btn_stat, sizeof(btn_status_t));

	while(try_cnt--)
	{
		if( storage_save_file(KTTH_RESUME_DATA_PATH, &resume_data, sizeof(resume_data)) >= 0)
			break;
	}
}


int ktth_sernaio__init_btn()
{
    g_btn_stat.btn_start__stat = BTN_STATUS__RELEASE;
    g_btn_stat.btn_start__mileage = 0 ;
    g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
    g_btn_stat.btn_end__mileage = 0 ;
}

int ktth_sernaio__push_btn(int btn_type, int status)
{
    int cur_odo = get_server_mileage() + get_gps_mileage();
    int need_to_save_resume = 0;
    switch(btn_type)
    {
        case BTN_TYPE__START:
        {
            // 시작버튼이 최초로 눌렸을때..
            if (( g_btn_stat.btn_start__stat == BTN_STATUS__RELEASE) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_start__stat = BTN_STATUS__PUSH;
                g_btn_stat.btn_start__mileage = cur_odo;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
                
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);

                LOGI(LOG_TARGET,  "[BTN] START CASE 1 - start mileage [%d] ++", g_btn_stat.btn_start__mileage);
                devel_webdm_send_log("[BTN] S - 1 [%d] ++\n", g_btn_stat.btn_start__mileage);

                sender_add_data_to_buffer(eBUTTON_START_MILEAGE_EVT, NULL, ePIPE_2);
                need_to_save_resume = 1;
            }
            // 시작버튼만 2번 눌림.
            else if (( g_btn_stat.btn_start__stat == BTN_STATUS__PUSH) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_end__mileage = cur_odo;

                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);

                LOGI(LOG_TARGET, "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                devel_webdm_send_log( "[BTN] E - 2 [%d]", g_btn_stat.btn_end__mileage);

                sender_add_data_to_buffer(eBUTTON_END_MILEAGE_EVT, NULL, ePIPE_2);

                sleep(1);
                g_btn_stat.btn_start__stat = BTN_STATUS__PUSH;
                g_btn_stat.btn_start__mileage = cur_odo;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;

                

                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);

                LOGI(LOG_TARGET,  "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                devel_webdm_send_log( "[BTN] S - 2 [%d]", g_btn_stat.btn_start__mileage);

                sender_add_data_to_buffer(eBUTTON_START_MILEAGE_EVT, NULL, ePIPE_2);

                need_to_save_resume = 1;
            }
            break;
        }
        case BTN_TYPE__END:
        {
            // 정상시나리오
            if (( g_btn_stat.btn_start__stat == BTN_STATUS__PUSH) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_start__stat = BTN_STATUS__RELEASE;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
                g_btn_stat.btn_end__mileage = cur_odo;

                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);

                LOGI(LOG_TARGET, "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                devel_webdm_send_log( "[BTN] E - 1 [%d]", g_btn_stat.btn_end__mileage);

                sender_add_data_to_buffer(eBUTTON_END_MILEAGE_EVT, NULL, ePIPE_2);

                need_to_save_resume = 1;
            }
            else
            {
                LOGE(LOG_TARGET, "[BTN] END CASE 3 - do nothing.. [%d]\n");

                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");

                devel_webdm_send_log( "[BTN] E - 3 : err");
            }
            break;
        }
        default:
        {
            break;
        }

    }

    if ( need_to_save_resume )
    {
        ktth_sernaio__save_resume_data();
    }
}

int ktth_sernaio__normal_pkt_odo_calc(int odo)
{
    static int last_odo = 0;
    int cur_odo = odo;

    int ret_odo = 0;

    if ( last_odo == 0 )
        last_odo = cur_odo;

	if ( cur_odo >= last_odo)
		ret_odo = cur_odo - last_odo;

    last_odo = cur_odo;

    return ret_odo;
}


int ktth_sernaio__keybtn_pkt_odo_calc()
{
    int ret_odo = g_btn_stat.btn_end__mileage - g_btn_stat.btn_start__mileage;


    if ( ret_odo < 0 )
        ret_odo = 0;

    LOGI(LOG_TARGET, "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    devel_webdm_send_log( "[BTN] CALC E [%d] S [%d] = [%d]", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    return ret_odo;
}


=======


#include <stdio.h>
#include <stdlib.h>

#include "ktth_senario.h"

#include <base/sender.h>
#include <board/board_system.h>
#include "logd/logd_rpc.h"

#include <kt_thermal_mdt800/packet.h>
#include <kt_thermal_mdt800/gpsmng.h>
#include <kt_thermal_mdt800/gps_utill.h>
#include <kt_thermal_mdt800/geofence.h>
#include <kt_thermal_mdt800/file_mileage.h>

#define LOG_TARGET eSVC_MODEL


btn_status_t g_btn_stat;
kttn_resum_data_t g_resume_data;

void ktth_sernaio__load_resume_data()
{
	kttn_resum_data_t resume_data = {0,};
	int ret = 0;

    ktth_sernaio__init_btn();

	if ( storage_load_file(KTTH_RESUME_DATA_PATH, &resume_data, sizeof(resume_data)) >= 0 )
	{
        memcpy(&g_btn_stat, &resume_data.btn_data, sizeof(btn_status_t));
	}
}


void ktth_sernaio__save_resume_data()
{
	int try_cnt = 4;
	kttn_resum_data_t resume_data = {0,};

    memcpy(&resume_data.btn_data, &g_btn_stat, sizeof(btn_status_t));

	while(try_cnt--)
	{
		if( storage_save_file(KTTH_RESUME_DATA_PATH, &resume_data, sizeof(resume_data)) >= 0)
			break;
	}
}


int ktth_sernaio__init_btn()
{
    g_btn_stat.btn_start__stat = BTN_STATUS__RELEASE;
    g_btn_stat.btn_start__mileage = 0 ;
    g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
    g_btn_stat.btn_end__mileage = 0 ;
}

int ktth_sernaio__push_btn(int btn_type, int status)
{
    int cur_odo = get_server_mileage() + get_gps_mileage();
    int need_to_save_resume = 0;
    switch(btn_type)
    {
        case BTN_TYPE__START:
        {
            // 시작버튼이 최초로 눌렸을때..
            if (( g_btn_stat.btn_start__stat == BTN_STATUS__RELEASE) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_start__stat = BTN_STATUS__PUSH;
                g_btn_stat.btn_start__mileage = cur_odo;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
                
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 1 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);

                LOGI(LOG_TARGET,  "[BTN] START CASE 1 - start mileage [%d] ++", g_btn_stat.btn_start__mileage);
                devel_webdm_send_log("[BTN] S - 1 [%d] ++\n", g_btn_stat.btn_start__mileage);

                sender_add_data_to_buffer(eBUTTON_START_MILEAGE_EVT, NULL, ePIPE_2);
                need_to_save_resume = 1;
            }
            // 시작버튼만 2번 눌림.
            else if (( g_btn_stat.btn_start__stat == BTN_STATUS__PUSH) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_end__mileage = cur_odo;

                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);

                LOGI(LOG_TARGET, "[BTN] END CASE 2 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                devel_webdm_send_log( "[BTN] E - 2 [%d]", g_btn_stat.btn_end__mileage);

                sender_add_data_to_buffer(eBUTTON_END_MILEAGE_EVT, NULL, ePIPE_2);

                sleep(1);
                g_btn_stat.btn_start__stat = BTN_STATUS__PUSH;
                g_btn_stat.btn_start__mileage = cur_odo;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;

                

                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                printf( "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);

                LOGI(LOG_TARGET,  "[BTN] START CASE 2 - start mileage [%d] ++\n", g_btn_stat.btn_start__mileage);
                devel_webdm_send_log( "[BTN] S - 2 [%d]", g_btn_stat.btn_start__mileage);

                sender_add_data_to_buffer(eBUTTON_START_MILEAGE_EVT, NULL, ePIPE_2);

                need_to_save_resume = 1;
            }
            break;
        }
        case BTN_TYPE__END:
        {
            // 정상시나리오
            if (( g_btn_stat.btn_start__stat == BTN_STATUS__PUSH) && ( status == BTN_STATUS__PUSH) )
            {
                g_btn_stat.btn_start__stat = BTN_STATUS__RELEASE;
                g_btn_stat.btn_end__stat = BTN_STATUS__RELEASE;
                g_btn_stat.btn_end__mileage = cur_odo;

                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                printf( "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);

                LOGI(LOG_TARGET, "[BTN] END CASE 1 - end mileage [%d] ++\n", g_btn_stat.btn_end__mileage);
                devel_webdm_send_log( "[BTN] E - 1 [%d]", g_btn_stat.btn_end__mileage);

                sender_add_data_to_buffer(eBUTTON_END_MILEAGE_EVT, NULL, ePIPE_2);

                need_to_save_resume = 1;
            }
            else
            {
                LOGE(LOG_TARGET, "[BTN] END CASE 3 - do nothing.. [%d]\n");

                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");
                printf( "[BTN] END CASE 3 - do nothing.. [%d]\n");

                devel_webdm_send_log( "[BTN] E - 3 : err");
            }
            break;
        }
        default:
        {
            break;
        }

    }

    if ( need_to_save_resume )
    {
        ktth_sernaio__save_resume_data();
    }
}

int ktth_sernaio__normal_pkt_odo_calc(int odo)
{
    static int last_odo = 0;
    int cur_odo = odo;

    int ret_odo = 0;

    if ( last_odo == 0 )
        last_odo = cur_odo;

	if ( cur_odo >= last_odo)
		ret_odo = cur_odo - last_odo;

    last_odo = cur_odo;

    return ret_odo;
}


int ktth_sernaio__keybtn_pkt_odo_calc()
{
    int ret_odo = g_btn_stat.btn_end__mileage - g_btn_stat.btn_start__mileage;


    if ( ret_odo < 0 )
        ret_odo = 0;

    LOGI(LOG_TARGET, "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);
    printf( "[BTN] odo calc btn_end__mileage [%d] / btn_start__mileage [%d] / calc [%d] \r\n", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    devel_webdm_send_log( "[BTN] CALC E [%d] S [%d] = [%d]", g_btn_stat.btn_end__mileage, g_btn_stat.btn_start__mileage, ret_odo);

    return ret_odo;
}


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

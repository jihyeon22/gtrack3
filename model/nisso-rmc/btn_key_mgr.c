#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <base/devel.h>
#include <base/sender.h>
#include <util/transfer.h>

#include "netcom.h"
#include "custom.h"

#include "logd/logd_rpc.h"
#include "btn_key_mgr.h"

#include <nisso_mdt800/packet.h>
#include <mdsapi/mds_api.h>

#define LOG_TARGET eSVC_MODEL

extern int is_ignition_off;

GPIP_EVT_INFO_T gpio_evt[] = {
    {NISSO_EXT_GPIO_BTN_GPIO_NUM_1, GPIO_EVT_LEVEL_LOW, eGpioEdgeFalling}, 
    {NISSO_EXT_GPIO_BTN_GPIO_NUM_2, GPIO_EVT_LEVEL_LOW, eGpioEdgeFalling}, 
    {-1, 0, 0}, 
};

int nisso_btn_mgr__gpio_evt_proc(GPIP_EVT_INFO_T* evt_res)
{
    time_t cur_time = 0;

    static time_t last_key_0 = 0;
    static time_t last_key_1 = 0;

	struct timeval tv;
	struct tm ttm;
    
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);
    
    cur_time = mktime(&ttm);

    if ( ( evt_res->gpio_num == NISSO_EXT_GPIO_BTN_GPIO_NUM_1 ) && ( evt_res->evt_type == eGpioEdgeFalling))
    {
        if ( (cur_time - last_key_0) > CHECK_EVENT_FILERING_SEC )
        {
            usleep(500);
            // only key on send 
            if ( power_get_ignition_status() == POWER_IGNITION_ON )
            {
                LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 1 !!! SEND PKT\n", __func__);
                //devel_webdm_send_log("btn 0");
                sender_add_data_to_buffer(eBUTTON_NUM0_EVT, NULL, ePIPE_1);
            }
            else
            {
                LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 1 !!! SEND PKT ->  but key off skip\n", __func__);
            }

            // devel_webdm_send_log("BTN 1 push");
            last_key_0 = cur_time;
        }
        else
        {
            LOGE(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 1 !!! BUT TIME CHECK... DO NOTHING..\n", __func__);
        }

    }


    if ( ( evt_res->gpio_num == NISSO_EXT_GPIO_BTN_GPIO_NUM_2 ) && ( evt_res->evt_type == eGpioEdgeFalling))
    {
        if ( (cur_time - last_key_1) > CHECK_EVENT_FILERING_SEC )
        {
            // only key on send 
            usleep(500);
            if ( power_get_ignition_status() == POWER_IGNITION_ON )
            {
                LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 2 !!! SEND PKT\n", __func__);
                sender_add_data_to_buffer(eBUTTON_NUM1_EVT, NULL, ePIPE_1);
                //devel_webdm_send_log("btn 1");
            }
            else
            {
                LOGE(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 2 !!! SEND PKT ->  but key off skip\n", __func__);
            }

            // devel_webdm_send_log("BTN 2 push");
            last_key_1 = cur_time;
        }
        else
        {
            LOGE(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT 2 !!! BUT TIME CHECK... DO NOTHING..\n", __func__);
        }

    }

    return 0;
}

void nisso_btn_mgr__init()
{
    static int init_run_flag = 0;

    if ( init_run_flag == 0 ) 
    {
        LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] BTN EVT INIT 1 => GPIO [%d]\n", NISSO_EXT_GPIO_BTN_GPIO_NUM_1 );
        LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] BTN EVT INIT 2 => GPIO [%d]\n", NISSO_EXT_GPIO_BTN_GPIO_NUM_2 );
        mds_api_gpio_evt_start(gpio_evt, nisso_btn_mgr__gpio_evt_proc);
    }
    init_run_flag = 1;
}

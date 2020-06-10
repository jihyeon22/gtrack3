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

#include <mdt800/packet.h>
#include <mdsapi/mds_api.h>

#define LOG_TARGET eSVC_MODEL

GPIP_EVT_INFO_T gpio_evt[] = {
    {BIZINCAR_GPIO_BTN_GPIO_NUM, GPIO_EVT_LEVEL_LOW, eGpioEdgeFalling}, 
    {-1, 0, 0}, 
};

int bizincar_btn_mgr__gpio_evt_proc(GPIP_EVT_INFO_T* evt_res)
{
    if ( ( evt_res->gpio_num == BIZINCAR_GPIO_BTN_GPIO_NUM ) && ( evt_res->evt_type == eGpioEdgeFalling))
    {
        LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] %s> BTN EVT !!! SEND PKT\n", __func__);
        sender_add_data_to_buffer(eEXT_GPIO_BTN_1, NULL, ePIPE_2);
    }
    
    return 0;
}

void bizincar_btn_mgr__init()
{
    LOGI(LOG_TARGET, "[BIZINCAR : EXT BTN] BTN EVT INIT => GPIO [%d]\n", BIZINCAR_GPIO_BTN_GPIO_NUM );
    mds_api_gpio_evt_start(gpio_evt, bizincar_btn_mgr__gpio_evt_proc);
}

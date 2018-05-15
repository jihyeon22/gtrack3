#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logd_rpc.h>
#include <base/devel.h>
#include <logd/logd_rpc.h>

#include <util/geofence-v2.h>


#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"


void skyan_senario__pre_init()
{
    skyan_tools__load_resume_data();
    init_geo_fence_v2(eGEN_FENCE_V2_DEBUG_MODE, eGEN_FENCE_V2_NO_READ_SAVED_DATA_MODE);
}

void skyan_senario__init()
{

}

// call every 1-sec
void skyan_senario__nostart_callback()
{
    printf("%s() -> start\r\n", __func__);
}

// call every 1-sec
void skyan_senario__start_callback()
{
    SKY_AUTONET_PKT__SETTING_INFO_T* p_setting_info;
    static int main_cnt = 0;
    gpsData_t gpsdata = {0,};
    int fence_num = 0;
    fence_v2_notification_t fnoti;

    int send_interval = 0;
    int send_key_evt = 0;

    p_setting_info = skyan_tools__get_setting_info();

    gps_get_curr_data(&gpsdata);
    
    printf("%s() -> start\r\n", __func__);
    // batt check..
    if ( p_setting_info != NULL)
        skyan_tools__chk_car_batt_level(p_setting_info->set_info__low_batt, BATT_CHK_INTERVAL_SEC);

    if ( gpsdata.active == 1 )
    {
		fnoti = get_geofence_notification_v2(&fence_num, gpsdata);
		if(fnoti != eFENCE_V2_NONE_NOTIFICATION)
		{
            send_sky_autonet_geofence_evt(fence_num, fnoti);
		}
    }

    if ( skyan_tools__get_key_stat() == SKYAN_KEY_STAT_ON )
    {
        send_interval = get_user_cfg_keyon_interval();
        send_key_evt = SKY_AUTONET_EVT__KEYON_REPORT;
        printf("%s() -> current key stat on [%d]\r\n", __func__, skyan_tools__get_key_stat());
    }
    else
    {
        send_interval = get_user_cfg_keyoff_interval();
        send_key_evt = SKY_AUTONET_EVT__KEYOFF_REPORT;
        printf("%s() -> current key stat off [%d]\r\n", __func__, skyan_tools__get_key_stat());
    }

    printf("%s() -> main_cnt [%d] send_interval [%d]\r\n", __func__, main_cnt, send_interval);

    if ( main_cnt++ > send_interval )
    {
        send_sky_autonet_evt_pkt(send_key_evt);
        main_cnt = 0;
    }
    
    // normal senario
}

void skyan_senario__poweroff()
{
    skyan_tools__save_resume_data();
    // deinit_geo_fence_v2();
}




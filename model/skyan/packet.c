#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <at/at_util.h>
#include <base/gpstool.h>
#include <base/thermtool.h>
#include <board/battery.h>
#include <board/power.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/mileage.h>

#include <board/modem-time.h>

#include <logd/logd_rpc.h>
#include <util/geofence-v2.h>
#include <jansson.h>

#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"
#include "netcom.h"
#include "config.h"



int _get_phonenum_int_type()
{
    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    static unsigned int phonenum_int = 0;

    if ( phonenum_int > 0 )
        return phonenum_int;

    at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);
    phonenum_int = atoi(phonenum);
    // test code
    //phonenum_int = 1223687989;

    return phonenum_int;
}


char* _pkt_to_json_format(SKY_AUTONET_PKT__GPS_PKT_T* gps_pkt)
{
  
    char* s = NULL;
    char* debug_s = NULL;

    json_t *root = json_object();
    json_t *pkt_element = json_object();

    json_t *json_arr = json_array();
  
    json_array_append(json_arr, pkt_element);
    json_object_set_new( root, "packetLogList", json_arr );

    json_object_set_new( pkt_element, "serviceId", json_string(gps_pkt->header.pkt_stx)); // 서비스ID(STX)
    json_object_set_new( pkt_element, "unitId", json_integer(gps_pkt->header.phonenum)); // 단말식별번호
    json_object_set_new( pkt_element, "manageType", json_string(gps_pkt->header.pkt_msg)); // 메시지구분
    json_object_set_new( pkt_element, "msgCode", json_string(gps_pkt->header.pkt_evt_code)); // 이벤트코드
    json_object_set_new( pkt_element, "reqDt", json_integer(gps_pkt->body.report_time)); // 보고시간
    json_object_set_new( pkt_element, "keyOnDt", json_integer(gps_pkt->body.dev_stat__keyon_time)); // 최종 키ON 시간
    json_object_set_new( pkt_element, "keyOffDt", json_integer(gps_pkt->body.dev_stat__keyoff_time)); // 최종 키OFF 시간
    json_object_set_new( pkt_element, "keyStatus", json_integer(gps_pkt->body.dev_stat__keystat)); // 키 상태
    json_object_set_new( pkt_element, "deviceStatus", json_integer(gps_pkt->body.dev_stat__stat)); // 단말상태
    json_object_set_new( pkt_element, "batteryVolt", json_integer(gps_pkt->body.dev_stat__main_volt)); // 차량전압
    json_object_set_new( pkt_element, "firmwareInfo", json_string(gps_pkt->body.dev_stat__firm_ver)); // 펌웨어버전
    json_object_set_new( pkt_element, "gpsStatus", json_integer(gps_pkt->body.gps_dev__stat)); // 현재 GPS 상태
    json_object_set_new( pkt_element, "gpsEffectiveDt", json_integer(gps_pkt->body.gps_dev__last_fix_time)); // 최종 유효 GPS 시
    json_object_set_new( pkt_element, "latitude", json_integer(gps_pkt->body.gps_dev__lat)); // 위도
    json_object_set_new( pkt_element, "longitude", json_integer(gps_pkt->body.gps_dev__lon)); // 경도
    json_object_set_new( pkt_element, "direction", json_integer(gps_pkt->body.gps_dev__dir)); // 방향
    json_object_set_new( pkt_element, "drivingSpeed", json_integer(gps_pkt->body.gps_dev__speed)); // 주행속도
    json_object_set_new( pkt_element, "accumulDist", json_integer(gps_pkt->body.gps_dev__total_dist)); // 장착누적거리
    json_object_set_new( pkt_element, "visitPointId", json_integer(gps_pkt->body.gps_dev__geofence_id)); // 경유지Id
    json_object_set_new( pkt_element, "keyOnReportInterval", json_integer(gps_pkt->body.set_info.set_info__keyon_interval)); // 키On 보고주기
    json_object_set_new( pkt_element, "keyOffReportInterval", json_integer(gps_pkt->body.set_info.set_info__keyoff_interval)); // 키Off 보고주기
    json_object_set_new( pkt_element, "gpsColdWarm", json_integer(gps_pkt->body.set_info.set_info__gps_on)); // GPS On
    json_object_set_new( pkt_element, "dischargeVoltLevel", json_integer(gps_pkt->body.set_info.set_info__low_batt)); // 방전전압레벨
    json_object_set_new( pkt_element, "overSpeedDefault", json_integer(gps_pkt->body.set_info.set_info__over_speed)); // 과속 기준
    json_object_set_new( pkt_element, "engineIdleDefault", json_integer(gps_pkt->body.set_info.set_info__idle)); // 공회전 기준
    json_object_set_new( pkt_element, "maxRpmDefault", json_integer(gps_pkt->body.set_info.set_info__over_rpm)); // 고RPM 기준
    json_object_set_new( pkt_element, "axisActivation", json_integer(gps_pkt->body.set_info.set_info__gps_act)); // 위경도 활성화


    
    s = json_dumps(root, 0);
    //puts(s);
    // free(s); // not free

    printf("-------------\r\n");
    debug_s = json_dumps(root, JSON_INDENT(2));
    puts(debug_s);
    free(debug_s);
    printf("-------------\r\n");

    json_decref(root);
    json_decref(pkt_element);
    json_decref(json_arr);

    return s;

}


int create_sky_autonet_report_pkt(SKY_AUTONET__PKT_ARG_T* pkt_arg, unsigned char **pbuf, unsigned short *packet_len)
{
    static int last_gps_fix_time = 0;
    gpsData_t gpsdata = {0,};
    int pkt_total_len = 0;
    char* json_buff = NULL;

   // ------------------------------------------
    // pkt layout.
    // 1. header
    // 2. body
    // 3. tail
    // ------------------------------------------
    SKY_AUTONET_PKT__GPS_PKT_T gps_pkt;

    unsigned char *packet_buf;

    gps_get_curr_data(&gpsdata);

    sprintf(gps_pkt.header.pkt_stx, "%c", SKY_AUTONET_PKT__PREFIX);

    gps_pkt.header.phonenum = _get_phonenum_int_type();
    // header.pkt_len = sizeof(body) + sizeof(tail) + 3; // TODO: remove.
    sprintf(gps_pkt.header.pkt_msg, "%c",'B');
    //header.pkt_end_flag = '1';
    sprintf(gps_pkt.header.pkt_evt_code, "%c", pkt_arg->evt_code);


    gps_pkt.body.report_time = get_modem_time_utc_sec();
    gps_pkt.body.dev_stat__keyon_time = skyan_tools__get_key_on_time();
    gps_pkt.body.dev_stat__keyoff_time = skyan_tools__get_key_off_time();
    gps_pkt.body.dev_stat__keystat = skyan_tools__get_key_stat();
    gps_pkt.body.dev_stat__stat = skyan_tools__get_devstat();
    gps_pkt.body.dev_stat__main_volt = skyan_tools__get_car_batt_level();
    skyan_tools__get_firm_ver(gps_pkt.body.dev_stat__firm_ver);
    
    if ( gpsdata.active == 1 )
    {
        gps_pkt.body.gps_dev__lat = gpsdata.lat * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
        gps_pkt.body.gps_dev__lon = gpsdata.lon * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
        gps_pkt.body.gps_dev__dir = gpsdata.angle;
    }
    else if ( gpsdata.active == 0 )
    {
        gpsData_t last_gpsdata;
        gps_valid_data_get(&last_gpsdata);

        gps_pkt.body.gps_dev__lat = last_gpsdata.lat * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
        gps_pkt.body.gps_dev__lon = last_gpsdata.lon * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
        gps_pkt.body.gps_dev__dir = 0;
    }

    gps_pkt.body.gps_dev__stat = skyan_tools__get_gps_stat(gpsdata.active);
    if ( gps_pkt.body.gps_dev__stat == 2)
        last_gps_fix_time = gps_pkt.body.report_time;
    
    gps_pkt.body.gps_dev__last_fix_time = last_gps_fix_time;

    gps_pkt.body.gps_dev__speed = gpsdata.speed;
    gps_pkt.body.gps_dev__total_dist = mileage_get_m();
    gps_pkt.body.gps_dev__geofence_id = pkt_arg->fence_id;

    gps_pkt.body.set_info.set_info__keyon_interval = get_user_cfg_keyon_interval();;
    gps_pkt.body.set_info.set_info__keyoff_interval = get_user_cfg_keyoff_interval();
    gps_pkt.body.set_info.set_info__gps_on = 0;
    gps_pkt.body.set_info.set_info__low_batt = skyan_tools__get_low_batt_level();
    gps_pkt.body.set_info.set_info__over_speed = 0;
    gps_pkt.body.set_info.set_info__idle = 0;
    gps_pkt.body.set_info.set_info__over_rpm = 0;
    gps_pkt.body.set_info.set_info__gps_act = 0;

   // *packet_len = pkt_total_len;
	
    json_buff = _pkt_to_json_format(&gps_pkt);
    *pbuf = json_buff;
    *packet_len = strlen(json_buff);
    
    return 0;
}


int send_sky_autonet_report_pkt(int evt)
{
    // no use...
    return 0;
}


int send_sky_autonet_evt_pkt(int evt)
{
    SKY_AUTONET__PKT_ARG_T pkt_arg;

    pkt_arg.fence_id = 0;
    pkt_arg.evt_code = evt;
    
    LOGT(eSVC_MODEL, "[SKYAN PKT] send normal pkt!! evt [%d]\r\n", evt);

    sender_add_data_to_buffer(e_skyan_pkt__evt, &pkt_arg, ePIPE_2);
    return 0;
}


int send_sky_autonet_req_geofence_info()
{
    SKY_AUTONET__PKT_ARG_T pkt_arg;

    pkt_arg.fence_id = 0;
    pkt_arg.evt_code = SKY_AUTONET_EVT__GEOFENCE__GET_FROM_SVR;
    
    LOGT(eSVC_MODEL, "[SKYAN PKT] send req geofence info !! \r\n");

    sender_add_data_to_buffer(e_skyan_pkt__req_geofence_info, &pkt_arg, ePIPE_2);
    return 0;
}

int send_sky_autonet_geofence_evt(int fence_id, fence_v2_notification_t geofence_evt)
{
    SKY_AUTONET__PKT_ARG_T pkt_arg;

    pkt_arg.fence_id = fence_id;

    if(geofence_evt == eFENCE_V2_IN_NOTIFICATION)
        pkt_arg.evt_code = SKY_AUTONET_EVT__GEOFENCE__ENTRY;
    else if(geofence_evt == eFENCE_V2_OUT_NOTIFICATION)
        pkt_arg.evt_code = SKY_AUTONET_EVT__GEOFENCE__EXIT;
    else
        return -1;
    
    LOGT(eSVC_MODEL, "[SKYAN PKT] send geofence pkt!! geo id [%d] evt [%d]\r\n", pkt_arg.fence_id, pkt_arg.evt_code);

    sender_add_data_to_buffer(e_skyan_pkt__geofence_evt, &pkt_arg, ePIPE_2);
    return 0;
}


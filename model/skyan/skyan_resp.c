
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logd_rpc.h>

#include <jansson.h>
#include <util/geofence-v2.h>

#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"
#include "skyan_resp.h"

/* recv example.
{"status":0,"packetLogList":[{"serviceId":"S","unitId":1220129234,"manageType":"B","msgCode":"F","reqDt":1526295523,"keyOnDt":0,"keyOffDt":0,"keyStatus":0,"deviceStatus":0,"batteryVolt":0,"firmwareInfo":"","gpsStatus":1,"gpsEffectiveDt":0,"latitude":373993492,"longitude":1271030197,"direction":8,"drivingSpeed":0,"accumulDist":11096,"visitPointId":0,"keyOnReportInterval":60,"keyOffReportInterval":3600,"gpsColdWarm":0,"dischargeVoltLevel":0,"overSpeedDefault":0,"engineIdleDefault":0,"maxRpmDefault":0,"axisActivation":0,"visitList":[]}]}
// convert
{
	"status" : 0,
	"packetLogList" : [
		{
			"serviceId" : S,
			"unitId" : 1220129234,
			"manageType" : B,
			"msgCode" : F,
			"reqDt" : 1526295523,
			"keyOnDt" : 0,
			"keyOffDt" : 0,
			"keyStatus" : 0,
			"deviceStatus" : 0,
			"batteryVolt" : 0,
			"firmwareInfo" : ,
			"gpsStatus" : 1,
			"gpsEffectiveDt" : 0,
			"latitude" : 373993492,
			"longitude" : 1271030197,
			"direction" : 8,
			"drivingSpeed" : 0,
			"accumulDist" : 11096,
			"visitPointId" : 0,
			"keyOnReportInterval" : 60,
			"keyOffReportInterval" : 3600,
			"gpsColdWarm" : 0,
			"dischargeVoltLevel" : 0,
			"overSpeedDefault" : 0,
			"engineIdleDefault" : 0,
			"maxRpmDefault" : 0,
			"axisActivation" : 0,
			"visitList" : [
			]
		}
	]
}
*/

#define SKYAN_RESP_JSON_PARSER_DEBUG_MSG

int _json_to_resp_pkt(char* recv_buff, SKY_AUTONET_PKT__RESP_PKT_T* resp_pkt)
{

    json_t *root = NULL;

    json_t *packetLogList_array;
    json_t *packetLogList_element;
    int     packetLogList_array_size;

    json_t *visitList_array;
    json_t *visitList_element;
    int     visitList_array_size;

    int i, j;
    
    int result;
	json_error_t error;

    //char* json_example = "{\"status\":0,\"packetLogList\":[{\"serviceId\":\"S\",\"unitId\":1220129234,\"manageType\":\"B\",\"msgCode\":\"F\",\"reqDt\":1526295523,\"keyOnDt\":0,\"keyOffDt\":0,\"keyStatus\":0,\"deviceStatus\":0,\"batteryVolt\":0,\"firmwareInfo\":\"\",\"gpsStatus\":1,\"gpsEffectiveDt\":0,\"latitude\":373993492,\"longitude\":1271030197,\"direction\":8,\"drivingSpeed\":0,\"accumulDist\":11096,\"visitPointId\":0,\"keyOnReportInterval\":60,\"keyOffReportInterval\":3600,\"gpsColdWarm\":0,\"dischargeVoltLevel\":0,\"overSpeedDefault\":0,\"engineIdleDefault\":0,\"maxRpmDefault\":0,\"axisActivation\":0,\"visitList\":[]}]}";

	root = json_loads(recv_buff, 0, &error);
	if(!root){
	    //LOGE(LOG_TARGET, "%s:%d> json_loads \n", __func__, __LINE__);
        printf( "%s:%d> json_loads \n", __func__, __LINE__);
		//LOGE(LOG_TARGET, "error : on line %d: %s\n", error.line, error.text);
        printf( "error : on line %d: %s\n", error.line, error.text);
		result = -20;
        //devel_webdm_send_log("DOWNLOAD USER : invaild json [%s] [%d]", error.text, _g_parse_fail_cnt);
        printf("parse fail : invaild json [%s] [%d]", error.text, 0);
		return SKYAN_JSON_PARSE_FAIL;
	}

    
    if ( json_is_integer(json_object_get(root,"status")) )
    {
        resp_pkt->status =  json_integer_value(json_object_get(root,"status"));
    #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
        printf("%s() [%d] => resp_pkt->status [%d]\r\n", __func__, __LINE__, resp_pkt->status);
    #endif
    }

    // packetLogList array list
    packetLogList_array = json_object_get(root, "packetLogList");
    packetLogList_array_size = json_array_size(packetLogList_array);
    printf("packetLogList_array_size => [%d]\r\n", packetLogList_array_size);
    
    for(j = 0 ; j < packetLogList_array_size ; j++)
    {
        packetLogList_element = json_array_get(packetLogList_array,j);

        if ( json_is_string(json_object_get(packetLogList_element, "serviceId")) )          // 서비스ID(STX)	Char
        {
            snprintf(resp_pkt->header.pkt_stx, 2, json_string_value(json_object_get(packetLogList_element, "serviceId")));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->header.pkt_stx [%s]\r\n", __func__, __LINE__, resp_pkt->header.pkt_stx);
        #endif
        }
        
        if ( json_is_integer(json_object_get(packetLogList_element, "unitId")) )                // 단말식별번호	Unit32
        {
            resp_pkt->header.phonenum = json_integer_value(json_object_get(packetLogList_element, "unitId"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->header.phonenum [%d]\r\n", __func__, __LINE__, resp_pkt->header.phonenum);
        #endif
        }

        if ( json_is_string(json_object_get(packetLogList_element, "manageType")) )            // 메시지구분	Char
        {
            snprintf(resp_pkt->header.pkt_msg, 2, json_string_value(json_object_get(packetLogList_element, "manageType")));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->header.pkt_msg [%s]\r\n", __func__, __LINE__, resp_pkt->header.pkt_msg);
        #endif
        }
        
        if ( json_is_string(json_object_get(packetLogList_element, "msgCode")) )               // 이벤트코드	Char
        {
            snprintf(resp_pkt->header.pkt_evt_code, 2,  json_string_value(json_object_get(packetLogList_element, "msgCode")));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => (resp_pkt->header.pkt_evt_code [%s]\r\n", __func__, __LINE__, resp_pkt->header.pkt_evt_code);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "reqDt")) )                 // 보고시간	Time_t
        {
            resp_pkt->body.report_time = json_integer_value(json_object_get(packetLogList_element, "reqDt"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.report_time [%d]\r\n", __func__, __LINE__, resp_pkt->body.report_time);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "keyOnDt")) )               // 최종 키ON 시간	Time_t
        {
            resp_pkt->body.dev_stat__keyon_time = json_integer_value(json_object_get(packetLogList_element, "keyOnDt"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.dev_stat__keyon_time [%d]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__keyon_time);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "keyOffDt")) )              // 최종 키OFF 시간	Time_t
        {
            resp_pkt->body.dev_stat__keyoff_time = json_integer_value(json_object_get(packetLogList_element, "keyOffDt"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.dev_stat__keyoff_time [%d]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__keyoff_time);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "keyStatus")) )             // 키 상태	Uint8
        {
            resp_pkt->body.dev_stat__keystat = json_integer_value(json_object_get(packetLogList_element, "keyStatus"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.dev_stat__keystat [%d]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__keystat);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "deviceStatus")) )          // 단말상태	Uint8
        {
            resp_pkt->body.dev_stat__stat = json_integer_value(json_object_get(packetLogList_element, "deviceStatus"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.dev_stat__stat [%d]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__stat);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "batteryVolt")) )           // 차량전압	Unit16
        {
            resp_pkt->body.dev_stat__main_volt = json_integer_value(json_object_get(packetLogList_element, "batteryVolt"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.dev_stat__main_volt [%d]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__main_volt);
        #endif
        }

        if ( json_is_string(json_object_get(packetLogList_element, "firmwareInfo")) )          // 펌웨어버전	String
        {
            snprintf(resp_pkt->body.dev_stat__firm_ver, 10, json_string_value(json_object_get(packetLogList_element, "firmwareInfo")));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => (resp_pkt->header.dev_stat__firm_ver [%s]\r\n", __func__, __LINE__, resp_pkt->body.dev_stat__firm_ver);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "keyOnReportInterval")) )   // 키On 보고주기	Unit16
        {
            resp_pkt->body.set_info.set_info__keyon_interval = json_integer_value(json_object_get(packetLogList_element, "keyOnReportInterval"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__keyon_interval [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__keyon_interval);
        #endif
        }
        
        if ( json_is_integer(json_object_get(packetLogList_element, "keyOffReportInterval")) )  // 키Off 보고주기	Unit16
        {
            resp_pkt->body.set_info.set_info__keyoff_interval = json_integer_value(json_object_get(packetLogList_element, "keyOffReportInterval"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__keyoff_interval [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__keyoff_interval);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "gpsColdWarm")) )           // GPS On	Uint8
        {
            resp_pkt->body.set_info.set_info__gps_on = json_integer_value(json_object_get(packetLogList_element, "gpsColdWarm"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__gps_on [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__gps_on);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "dischargeVoltLevel")) )    // 방전전압레벨	Unit16
        {
            resp_pkt->body.set_info.set_info__low_batt = json_integer_value(json_object_get(packetLogList_element, "dischargeVoltLevel"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__low_batt [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__low_batt);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "overSpeedDefault")) )      // 과속 기준	Uint8
        {
            resp_pkt->body.set_info.set_info__over_speed = json_integer_value(json_object_get(packetLogList_element, "overSpeedDefault"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__over_speed [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__over_speed);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "engineIdleDefault")) )     // 공회전 기준	Unit16
        {
            resp_pkt->body.set_info.set_info__idle = json_integer_value(json_object_get(packetLogList_element, "engineIdleDefault"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__idle [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__idle);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "maxRpmDefault")) )         // 고RPM 기준	Unit16
        {
            resp_pkt->body.set_info.set_info__over_rpm = json_integer_value(json_object_get(packetLogList_element, "maxRpmDefault"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__over_rpm [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__over_rpm);
        #endif
        }

        if ( json_is_integer(json_object_get(packetLogList_element, "axisActivation")) )        // 위경도 활성화	Uint8
        {
            resp_pkt->body.set_info.set_info__gps_act = json_integer_value(json_object_get(packetLogList_element, "axisActivation"));
        #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
            printf("%s() [%d] => resp_pkt->body.set_info.set_info__gps_act [%d]\r\n", __func__, __LINE__, resp_pkt->body.set_info.set_info__gps_act);
        #endif
        }


        // visit list
        visitList_array = json_object_get(packetLogList_element, "visitList");
        visitList_array_size = json_array_size(visitList_array);
        printf("visitList_array_size => [%d]\r\n", visitList_array_size);
        
        resp_pkt->fence_cnt = packetLogList_array_size;
        for(i = 0 ; i < packetLogList_array_size ; i++)
        {
            visitList_element = json_array_get(visitList_array, i);
            int unit_scale = 0;

            if (json_is_integer(json_object_get(visitList_element, "visitPointId")))
            {
                resp_pkt->fence[i].fence_point_id = json_integer_value(json_object_get(visitList_element, "visitPointId"));          // 목표 방문지점 id	Unit32
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].fence_point_id [%d]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].fence_point_id);
            #endif
            }

            if (json_is_integer(json_object_get(visitList_element, "visitPathId")))
            {
                resp_pkt->fence[i].fence_path_id = json_integer_value(json_object_get(visitList_element, "visitPathId"));           // 방문 경로 id	Unit32
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].fence_path_id [%d]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].fence_path_id);
            #endif
            }

            if (json_is_integer(json_object_get(visitList_element, "latitude")))
            {
                float tmp_floatval = json_integer_value(json_object_get(visitList_element, "latitude"));
                resp_pkt->fence[i].gps_lat = tmp_floatval / 1000000.0 ;              // 위도	Unit32
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].fence_path_id [%f]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].gps_lat);
            #endif
            }

            if (json_is_integer(json_object_get(visitList_element, "longitude")))
            {
                float tmp_floatval = json_integer_value(json_object_get(visitList_element, "longitude"));             // 경도	Unit32
                resp_pkt->fence[i].gps_lon = tmp_floatval / 1000000.0 ;              // 위도	Unit32
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].gps_lon [%f]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].gps_lon);
            #endif
            }

            if (json_is_integer(json_object_get(visitList_element, "visitOrder")))
            {
                resp_pkt->fence[i].visit_order = json_object_get(visitList_element, "visitOrder");            // 방문 순서	Uint8
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].visit_order [%d]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].visit_order);
            #endif
            }

            if (json_is_string(json_object_get(visitList_element, "radiusUnit")))
            {
                if ( strcmp(json_string_value(json_object_get(visitList_element, "radiusUnit")), "m") == 0 ) 
                    unit_scale = 1;
                else if  ( strcmp(json_string_value(json_object_get(visitList_element, "radiusUnit")), "km") == 0 ) 
                    unit_scale = 1000;
            
            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d]  :: unit_scale [%d] / [%s]\r\n", __func__, __LINE__, i, unit_scale, json_string_value(json_object_get(visitList_element, "radiusUnit")));
            #endif

            }

            if (json_is_integer(json_object_get(visitList_element, "radius")))
            {
                resp_pkt->fence[i].radius_m = json_integer_value(json_object_get(visitList_element, "radius"));                // 반경	Unit16
                resp_pkt->fence[i].radius_m =  resp_pkt->fence[i].radius_m * unit_scale;

            #ifdef SKYAN_RESP_JSON_PARSER_DEBUG_MSG 
                printf("%s() [%d] => resp_pkt->fence[%d].radius_m [%d] / [%d]\r\n", __func__, __LINE__, i, resp_pkt->fence[i].radius_m, json_integer_value(json_object_get(visitList_element, "radius")));
            #endif
            }

            resp_pkt->fence[i].set_info = 1;

        }

    }


    json_decref(root);
    //json_decref(packetLogList_array);

    return SKYAN_JSON_PARSE_SUCCESS;
}

int skyan_resp__parse(char* recv_buff)
{
    SKY_AUTONET_PKT__RESP_PKT_T resp_pkt;
    int i = 0 ;

    memset(&resp_pkt, 0xff, sizeof(resp_pkt));

    printf("recv -----------------------------\r\n");
    printf("%s\r\n",recv_buff);
    printf("----------------------------------\r\n");
    
    // fail check..
    if ( _json_to_resp_pkt(recv_buff, &resp_pkt) == SKYAN_JSON_PARSE_FAIL)
    {
        return 0;
    }
    
    if ( resp_pkt.status != 0 )
        return -1;
    
    // fence setting
    for ( i = 0 ; i < resp_pkt.fence_cnt ; i ++ )
    {
        geo_fence_v2_setup_t geosetup_info = {0,};

        if ( resp_pkt.fence[i].set_info == 1 ) // enable 
            geosetup_info.enable = eGEN_FENCE_V2_ENABLE;
        else // disable
            geosetup_info.enable = eGEN_FENCE_V2_DISABLE;
    
        geosetup_info.latitude = resp_pkt.fence[i].gps_lat;
        geosetup_info.longitude = resp_pkt.fence[i].gps_lon;

        geosetup_info.range = resp_pkt.fence[i].radius_m;
        geosetup_info.setup_fence_status = eFENCE_V2_SETUP_ENTRY_EXIT;

        set_geo_fence_setup_info_v2(resp_pkt.fence[i].fence_point_id, &geosetup_info);
    }
    
    set_user_cfg_keyoff_interval(resp_pkt.body.set_info.set_info__keyoff_interval);
    set_user_cfg_keyon_interval(resp_pkt.body.set_info.set_info__keyon_interval);

    skyan_tools__set_setting_info(&resp_pkt.body.set_info);

    return 0;
}


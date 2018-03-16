
#include <stdio.h>
#include <stdlib.h>

#include "alloc2_pkt.h"
#include <mdsapi/mds_api.h>
#include <at/at_util.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <base/mileage.h>

#include <board/power.h>
#include <board/board_system.h>

#include "alloc2_senario.h"
#include "alloc2_obd_mgr.h"
#include "alloc2_daily_info.h"

#include <logd_rpc.h>

#ifdef SERVER_ABBR_ALM2
#define ALLOC_WEBDM_DBG__SVR_RET_CODE
#endif

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

/*
static int _get_mdm_sn_int_type()
{
    return 3522803456;
}
*/

static int _get_mdm_sn_str_type(char* buff)
{
    // 16
    char* tmp_imei[16] = {0,};
    at_get_imei(tmp_imei, 16);
    sprintf(buff, "%16s", tmp_imei);
    
    // strcpy(buff, "3522803456");
    return 0;
}

int _make_common_header(ALLOC_PKT_COMMON_HEADER* header, int pkt_len, int pkt_id)
{
    configurationBase_t *conf = get_config_base();
    //ALLOC_PKT_COMMON_HEADER header_tmp;
/*
    header->pkt_total_len = htonl(pkt_len);
    header->mdm_phonenum = htonl(_get_phonenum_int_type());
    header->time_stamp = htonl((int)get_system_time_utc_sec(conf->gps.gps_time_zone));
    header->msg_type = pkt_id;
    header->reserved = 0x00;
    */

    header->pkt_total_len = pkt_len;
    header->mdm_phonenum = _get_phonenum_int_type();
    header->time_stamp = (int)get_system_time_utc_sec(conf->gps.gps_time_zone);
    header->msg_type = pkt_id;
    header->reserved = 0x00;
/*
    printf("---- header : [%x] print start---------------\r\n", pkt_id);
    printf(" >> pkt_total_len [%d]/[%x]\r\n", pkt_len, pkt_len);
    printf(" >> mdm_phonenum [%ud]/[%x]\r\n", _get_phonenum_int_type(), _get_phonenum_int_type());
    printf(" >> time_stamp [%d]/[%x]\r\n", get_system_time_utc_sec(conf->gps.gps_time_zone), get_system_time_utc_sec(conf->gps.gps_time_zone));
    printf(" >> msg_type [%d]/[%x]\r\n", pkt_id, pkt_id);
    printf("---- header : [%x] print end ---------------\r\n", pkt_id);
*/
    return 0;
}
// ===============================================================================================
// e_mdm_setting_val = 0x01
// ===============================================================================================

int make_pkt__mdm_setting_val(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__MDM_SETTING_VAL target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_mdm_setting_val;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

	LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
    _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    _get_mdm_sn_str_type(target_pkt.mdm_sn);

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

    //printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);
    //mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
    //printf("-------------------------------------------------\r\n");

    return 0;
}

int parse_pkt__mdm_setting_val(ALLOC_PKT_RECV__MDM_SETTING_VAL* recv_buff, char* target_ip, int target_port)
{
    int changed_server = 0;

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);



    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved ;

    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);

    printf("parse_pkt__mdm_setting_val ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->proxy_server_ip => [%s]\r\n", recv_buff->proxy_server_ip);
    printf("recv_buff->proxy_server_port => [%d]\r\n", recv_buff->proxy_server_port);
    printf("recv_buff->key_on_gps_report_interval => [%d]\r\n", recv_buff->key_on_gps_report_interval);
    printf("recv_buff->key_off_gps_report_interval => [%d]\r\n", recv_buff->key_off_gps_report_interval);
    printf("recv_buff->low_batt_voltage => [%d]\r\n", recv_buff->low_batt_voltage);
    printf("recv_buff->mdm_reset_interval => [%d]\r\n", recv_buff->low_batt_voltage);
    printf("recv_buff->warnning_horn_cnt => [%d]\r\n", recv_buff->warnning_horn_cnt);
    printf("recv_buff->warnning_light_cnt => [%d]\r\n", recv_buff->warnning_light_cnt);
    printf("recv_buff->over_speed_limit_km => [%d]\r\n", recv_buff->over_speed_limit_km);
    printf("recv_buff->over_speed_limit_time => [%d]\r\n", recv_buff->over_speed_limit_time);
    printf("recv_buff->use_obd => [%d]\r\n", recv_buff->use_obd);
    printf("recv_buff->use_dtg => [%d]\r\n", recv_buff->use_dtg);
    printf("recv_buff->use_zone_function => [%d]\r\n", recv_buff->use_zone_function);
    printf("recv_buff->use_bcm => [%d]\r\n", recv_buff->use_bcm);
    printf("recv_buff->use_knock_sensor => [%d]\r\n", recv_buff->use_knock_sensor);
#ifdef PKT_VER_V_1_6   
    printf("recv_buff->door_lock_time => [%d]\r\n", recv_buff->door_lock_time);
#endif
    printf("recv_buff->reserved => [0x%d]\r\n", recv_buff->reserved);

    printf("parse_pkt__mdm_setting_val ----------------------------------------------------------\r\n"); 
    // valid check..
    // TODO: 각종 데이터에 대해서 체크후에 setting 값을 체크한다.

    if ( strncmp(target_ip, recv_buff->proxy_server_ip, strlen(target_ip)) != 0 )
        changed_server = 1;

    if ( target_port != recv_buff->proxy_server_port ) 
        changed_server = 1;

    set_mdm_setting_pkt_val(recv_buff);

    if ( changed_server == 1 ) 
    {
        set_cur_status(e_SEND_TO_SETTING_INFO);
        return 0;
    }
    
    // 기본적으로 서버정책을 gps data only 로 했다.
    set_cur_status(e_SEND_TO_SETTING_INFO_COMPLETE);


    // 만약에 obd 를 사용하겠다고 설정하면 obd 설정 시퀀스로간다.
    if ( recv_buff->use_obd == 1 )
        set_cur_status(e_SEND_TO_OBD_INFO);

#ifdef SERVER_ABBR_ALM1
    allkey_bcm_ctr__set_horn_light(recv_buff->warnning_horn_cnt, recv_buff->warnning_light_cnt);
#endif

#ifdef SERVER_ABBR_ALM2
    set_cur_status(e_SEND_REPORT_RUN);
        
#ifdef ALLOC_WEBDM_DBG__SVR_RET_CODE
    devel_webdm_send_log("[mdm_setting : 0x01] RET [%d]\n", pkt_ret_code);
#endif

    pkt_ret_code = 0; // 강제 성공하게 ㅋㅋ
#endif
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;

}




// ===============================================================================================
// e_mdm_stat_evt = 0x02,
// ===============================================================================================
int make_pkt__mdm_stat_evt(unsigned char **pbuf, unsigned short *packet_len, int evt_code)
{
    ALLOC_PKT_SEND__MDM_STAT_EVT target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_mdm_stat_evt;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val;
	p_mdm_setting_val = get_mdm_setting_val();

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);
    // make header
     _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    gpsData_t gpsdata = {0,}; 
    gps_get_curr_data(&gpsdata);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    // ------------------------------------------------
    {
        if ( power_get_ignition_status() == POWER_IGNITION_ON )
            target_pkt.car_stat = 0;   // (b-1) 차량상태구분 : 0x00 IG1 on , 0x01 IG1 off, 0x02 이동중
        else 
            target_pkt.car_stat = 1;
    }

    // ------------------------------------------------
    {
        static int last_rssi = 0;
        int cur_rssi = 0;
        if ( at_get_rssi(&cur_rssi) != AT_RET_SUCCESS ) 
            cur_rssi = last_rssi;
        else
            last_rssi = cur_rssi;

        target_pkt.mdm_rssi = cur_rssi;   // (b-1) 모뎀 수신레벨 : rssi
    }

    // ------------------------------------------------
    if ( p_mdm_setting_val == NULL ) 
    {
        target_pkt.key_on_gps_report_interval = 60;    // (b-2) ig on gps 정보보고 시간설정(초단위) 운행시
        target_pkt.key_off_gps_report_interval = 180;    // (b-2) ig off gps 정보보고 시간설정(초단위) 비운행시
    }
    else
    {
        target_pkt.key_on_gps_report_interval = p_mdm_setting_val->key_on_gps_report_interval;    // (b-2) ig on gps 정보보고 시간설정(초단위) 운행시
        target_pkt.key_off_gps_report_interval = p_mdm_setting_val->key_off_gps_report_interval;    // (b-2) ig off gps 정보보고 시간설정(초단위) 비운행시
    }

    target_pkt.low_batt_voltage = p_mdm_setting_val->low_batt_voltage;    // (b-1) 저전압설정 : 0.1v 단위 설정
    target_pkt.warnning_horn_cnt = p_mdm_setting_val->warnning_horn_cnt;    // (b-1) 경적횟수 : 1~10
    target_pkt.warnning_light_cnt = p_mdm_setting_val->warnning_light_cnt;   // (b-1) 비상등횟수
    target_pkt.over_speed_limit_km = p_mdm_setting_val->over_speed_limit_km;     // (b-1) 과속 기준속도 : km/h
    target_pkt.over_speed_limit_time = p_mdm_setting_val->over_speed_limit_time;     // (b-1) 과속 과속기준시간 secs

    // ------------------------------------------------
    {
        if ( power_get_power_source() == POWER_SRC_DC )
            target_pkt.main_pwr = 0; // (b-1) 주전원 유형 : 0 상시전원 , 1 내장배터리
        else
            target_pkt.main_pwr = 1; // (b-1) 주전원 유형 : 0 상시전원 , 1 내장배터리
    }
    // ------------------------------------------------
    target_pkt.always_pwr_stat = 0; // (b-1) 상시전원상태 : 0 : 정상 1 : 저전압 2 : 탈거
    // ------------------------------------------------
    {
        target_pkt.mdm_reset_interval = p_mdm_setting_val->mdm_reset_interval; // (b-2) 단말주기적 리셋시간 : 03:20 에 리셋시에는 320
        
    }
    // ------------------------------------------------
    target_pkt.mdm_internal_batt_stat = 0; // (b-1) 자체배터리상태 : 0 : 정상 1 : 저전압 2 : 탈거
    // ------------------------------------------------
    {
        // (b-1) gps 상태 : 0 : 정상 1 : 무효(연결 위성수가 기준치 이하) 2 : 탈거
        if ( mds_api_gps_util_get_gps_ant() == DEFINES_MDS_API_NOK ) 
            target_pkt.mdm_gps_stat = 2;
        else
        {
            if (gpsdata.satellite > 3 )
                target_pkt.mdm_gps_stat = 0;
            else
                target_pkt.mdm_gps_stat = 1;
        }
    }
    // ------------------------------------------------
    {
        target_pkt.car_batt_volt = get_car_batt_level(); // (b-2) 차량배터리전압 : 0.1 volt 단위
    }
    // ------------------------------------------------
    {
        int internal_batt = 0;
        mds_api_get_internal_batt_tl500(&internal_batt);
        target_pkt.mdm_internal_batt_volt = internal_batt/10; // (b-2) 자체배터리전압 : 0.1 volt 단위
    }
    target_pkt.car_start_stop = 0; // (b-1) 시동차단여부 : 0: 정상 1: 시동차단중
    target_pkt.event_code = evt_code; // (b-2) 발생 이벤트 code : 
    {
        ALLOC2_DAILY_OVERSPEED_MGR_T cur_over_speed_info;
        get_overspeed_info(&cur_over_speed_info);
        target_pkt.over_speed_cnt = cur_over_speed_info.over_speed_cnt;     // (b-1) 과속 과속기준시간 secs
    }

    printf("   >> mdm stat pkt :: car_stat [%d]\r\n", target_pkt.car_stat);
    printf("   >> mdm stat pkt :: mdm_rssi [%d]\r\n", target_pkt.mdm_rssi);
    printf("   >> mdm stat pkt :: key_on_gps_report_interval [%d]\r\n", target_pkt.key_on_gps_report_interval);
    printf("   >> mdm stat pkt :: key_off_gps_report_interval [%d]\r\n", target_pkt.key_off_gps_report_interval);
    printf("   >> mdm stat pkt :: low_batt_voltage [%d]\r\n", target_pkt.low_batt_voltage);
    printf("   >> mdm stat pkt :: warnning_horn_cnt [%d]\r\n", target_pkt.warnning_horn_cnt);
    printf("   >> mdm stat pkt :: warnning_light_cnt [%d]\r\n", target_pkt.warnning_light_cnt);
    printf("   >> mdm stat pkt :: over_speed_limit_km [%d]\r\n", target_pkt.over_speed_limit_km);
    printf("   >> mdm stat pkt :: over_speed_limit_time [%d]\r\n", target_pkt.over_speed_limit_time);
    printf("   >> mdm stat pkt :: main_pwr [%d]\r\n", target_pkt.main_pwr);
    printf("   >> mdm stat pkt :: always_pwr_stat [%d]\r\n", target_pkt.always_pwr_stat);
    printf("   >> mdm stat pkt :: mdm_reset_interval [%d]\r\n", target_pkt.mdm_reset_interval);
    printf("   >> mdm stat pkt :: mdm_internal_batt_stat [%d]\r\n", target_pkt.mdm_internal_batt_stat);
    printf("   >> mdm stat pkt :: mdm_gps_stat [%d]\r\n", target_pkt.mdm_gps_stat);
    printf("   >> mdm stat pkt :: car_batt_volt [%d]\r\n", target_pkt.car_batt_volt);
    printf("   >> mdm stat pkt :: mdm_internal_batt_volt [%d]\r\n", target_pkt.mdm_internal_batt_volt);
    printf("   >> mdm stat pkt :: car_start_stop [%d]\r\n", target_pkt.car_start_stop);
    printf("   >> mdm stat pkt :: event_code [%d]\r\n", target_pkt.event_code);
    printf("   >> mdm stat pkt :: over_speed_cnt [%d]\r\n", target_pkt.over_speed_cnt);

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

    //printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
    //printf("------------------------------------------------------\r\n");

    return 0;
}

int parse_pkt__mdm_stat_evt(ALLOC_PKT_RECV__MDM_STAT_EVT* recv_buff)
{
    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    printf("parse_pkt__mdm_stat_evt ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);
    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__mdm_stat_evt ----------------------------------------------------------\r\n");
    return 0;
    */
    int pkt_ret_code = 0;
    
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
#ifdef ALLOC_WEBDM_DBG__SVR_RET_CODE
    {
        static int last_pkt_ret_code = -1;

        if ( last_pkt_ret_code != pkt_ret_code ) 
            devel_webdm_send_log("[server ret : 0x02] RET [%d]\n", pkt_ret_code);

        last_pkt_ret_code = pkt_ret_code;
    }
#endif

    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;
}


// ===============================================================================================
// e_mdm_gps_info = 0x03,  
// ===============================================================================================
int make_pkt__mdm_gps_info(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__MDM_GPS_INFO target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_mdm_gps_info;
    static int send_dm_log = 0;

    gpsData_t gpsdata = {0,}; 

    send_dm_log ++;
    
    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    static int last_total_distance = 0;
    int cur_total_distance = mileage_get_m();

    last_total_distance = cur_total_distance;

	LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

	//gpsData_t* pdata;
	gps_get_curr_data(&gpsdata);

	// printf("[kksworks gps]\t%d\t%f\t%f\t%d\t%f\t\r\n",gpsdata.satellite, gpsdata.lat, gpsdata.lon, gpsdata.speed, gpsdata.hdop); 

    // make header
     _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    target_pkt.gps_time = gpsdata.utc_sec; // (b-4) gps 생성시간 : unix timestamp
    target_pkt.gps_lat = gpsdata.lat * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
    target_pkt.gps_lon = gpsdata.lon * 10000000.0; // (b-4) gps 경도 : WGS84좌표체계 예)127123456
    target_pkt.gps_dir = (int)(gpsdata.angle); // (b-4) gps 방향 : 360도표현
    target_pkt.gps_speed = gpsdata.speed; // b-2 스피드
    target_pkt.total_distance = cur_total_distance; // (b-4) 누적거리 meter
    target_pkt.day_distance = get_daily_info__daily_distance(cur_total_distance); // (b-4) 일일 운행거리 meter
    target_pkt.section_distance = chk_keyon_section_distance(cur_total_distance); // (b-4) 구간운행거리 : ig1 on ~ ig1 off 누적거리
    target_pkt.gps_vector = get_diff_distance_prev(cur_total_distance); // (b-4) 이동거리 : 이전좌표와 현재 좌표와의 거리
//    target_pkt.reserved[2]; // reserved
    target_pkt.car_batt = get_car_batt_level();

#ifdef SERVER_ABBR_ALM2
    target_pkt.gps_invalid = 0;

    if ( gpsdata.active == 0)
    {
        gpsData_t last_gpsdata;
        gps_valid_data_get(&last_gpsdata);

        LOGT(eSVC_MODEL, "[ALLOC2 ALM2 - GPS PKT] gps invalid last gps recovery\r\n");

        target_pkt.gps_lat = last_gpsdata.lat * 10000000.0; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
        target_pkt.gps_lon = last_gpsdata.lon * 10000000.0; // (b-4) gps 경도 : WGS84좌표체계 예)127123456

        target_pkt.gps_invalid = 1;
    }
#endif

    LOGT(eSVC_MODEL, "[ALLOC2 - GPS PKT] lat [%d] / lon [%d]\r\n", target_pkt.gps_lat, target_pkt.gps_lon);

    if (!( send_dm_log % 30 ))
    {
        devel_webdm_send_log("[gps pkt info] total_distance[%d], day_distance[%d], section_distance[%d], gps_vector[%d]\r\n",
                target_pkt.total_distance,
                target_pkt.day_distance,
                target_pkt.section_distance,
                target_pkt.gps_vector);
    }


    LOGI(eSVC_MODEL, "[gps pkt info] total_distance[%d], day_distance[%d], section_distance[%d], gps_vector[%d]\r\n",
            target_pkt.total_distance,
            target_pkt.day_distance,
            target_pkt.section_distance,
            target_pkt.gps_vector);
    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

    //printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
    //mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;
}


int parse_pkt__mdm_gps_info(ALLOC_PKT_RECV__MDM_GPS_INFO* recv_buff)
{
    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    printf("parse_pkt__mdm_gps_info ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__mdm_gps_info ----------------------------------------------------------\r\n");
*/
    int pkt_ret_code = 0;
    //pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1];
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
#ifdef ALLOC_WEBDM_DBG__SVR_RET_CODE
    {
        static int last_pkt_ret_code = -1;

        if ( last_pkt_ret_code != pkt_ret_code ) 
            devel_webdm_send_log("[server ret : 0x03] RET [%d]\n", pkt_ret_code);

        last_pkt_ret_code = pkt_ret_code;
    }
#endif

    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;
}



// ===============================================================================================
// e_obd_dev_info = 0x11
// ===============================================================================================
int make_pkt__obd_dev_info(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__OBD_DEV_INFO target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_obd_dev_info;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
     _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    target_pkt.obd_sn = 1234;

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

    //printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
    //mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
    //printf("------------------------------------------------------\r\n");

    return 0;
}

int parse_pkt__obd_dev_info(ALLOC_PKT_RECV__OBD_DEV_INFO* recv_buff)
{
    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    printf("parse_pkt__obd_dev_info ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->obd_recv_keyon_interval => [%d]\r\n", recv_buff->obd_recv_keyon_interval);
    printf("recv_buff->obd_recv_keyoff_interval => [%d]\r\n", recv_buff->obd_recv_keyoff_interval);
    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__obd_dev_info ----------------------------------------------------------\r\n");
    */
    

    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    set_obd_dev_pkt_info(recv_buff);
    set_cur_status(e_SEND_TO_OBD_INFO_COMPLETE);

    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;

    return 0;
}



// ===============================================================================================
// e_obd_stat = 0x12
// ===============================================================================================
int make_pkt__obd_stat(unsigned char **pbuf, unsigned short *packet_len, ALLOC_PKT_SEND__OBD_STAT_ARG obd_stat_arg)
{
    ALLOC_PKT_SEND__OBD_STAT target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_obd_stat;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
     _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    target_pkt.obd_stat_flag = obd_stat_arg.obd_stat_flag; // (b-1) obd 연결상태 0:ok 1:fail
    target_pkt.obd_stat = obd_stat_arg.obd_stat; // (b-1) obd 상태
    target_pkt.obd_remain_fuel_stat = obd_stat_arg.obd_remain_fuel_stat; // (b-1) 연료잔량 측정상태
    target_pkt.obd_evt_code = obd_stat_arg.obd_evt_code; // (b-1) 이벤트코드  0 : 정상, 1 : 주유, 2 : 인출
    target_pkt.obd_fuel_type = obd_stat_arg.obd_fuel_type; // (b-1) 연료타입 0 : OBD 미지정, 1 : 가솔린, 2 : 디젤, 3 : LPG, 4 : 전기차, 5 : 수소차
    target_pkt.obd_remain_fuel = obd_stat_arg.obd_remain_fuel; // (b-2) 주요/인출/충전량 : 연료단위에 의존적임
    target_pkt.reserved = 0; // (b-1) reserved


    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

    //printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;
}


int parse_pkt__obd_stat(ALLOC_PKT_RECV__OBD_STAT* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;

    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    printf("parse_pkt__obd_stat ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__obd_stat ----------------------------------------------------------\r\n");

    return 0;
*/
}


// ===============================================================================================
// e_obd_data = 0x13
// ===============================================================================================
int make_pkt__obd_data(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__OBD_DATA target_pkt;
    SECO_OBD_DATA_T cur_obd_data = {0,};
    SECO_OBD_INFO_T obd_dev_info = {0,};

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_obd_data;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
     _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    alloc2_obd_mgr__get_cur_obd_data(&cur_obd_data);
    alloc2_obd_mgr__get_obd_dev_info(&obd_dev_info);

    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    target_pkt.daily_driving_time = 0; // (b-4) 일일 운행시간 : 단위 초
    target_pkt.daily_distance = 0; // (b-4) 일을 누적거리 단위 m
    target_pkt.daily_engine_idle = 0; // (b-4) 일일 공회전시간
    {
        if ( strlen(obd_dev_info.fuel_type) < 3 )
            target_pkt.fuel_type = 0; // (b-1) 연료타입 : 0 : OBD 미지정, 1 : 가솔린, 2 : 디젤, 3 : LPG, 4 : 전기차, 5 : 수소차
        printf("   >> obd data pkt : fuel_type [%d]\r\n",target_pkt.fuel_type );
    }
    {
        target_pkt.fuel_remain = cur_obd_data.obd_data_fuel_remain; // (b-2) 현재연료잔량 : 연료단위에 의존적임 (단위0.1L)
        printf("   >> obd data pkt : fuel_remain [%d]\r\n",target_pkt.fuel_remain );
    }
    {
        target_pkt.rpm = cur_obd_data.obd_data_rpm; // (b-2) 현재 rpm
        printf("   >> obd data pkt : rpm [%d]\r\n",target_pkt.rpm );
    }
    {
        target_pkt.cooling_temp = cur_obd_data.obd_data_cot; // (b-2) 냉각수온도,
        printf("   >> obd data pkt : cooling_temp [%d]\r\n",target_pkt.cooling_temp );
    }
    {
        target_pkt.car_batt_volt = cur_obd_data.obd_data_car_volt; // (b-2) 현재 차량배터리 : 단위 0.1v
        printf("   >> obd data pkt : car_batt_volt [%d]\r\n",target_pkt.car_batt_volt );
    }
    {
        target_pkt.speed = cur_obd_data.obd_data_car_speed; // (b-2) 현재 속도
        printf("   >> obd data pkt : speed [%d]\r\n",target_pkt.speed );
    }
    target_pkt.fuel_supply = 0; // (b-4) 누적주유, 충전량 : 연료단위에 의존적임 (단위 0.1L)
    target_pkt.total_fuel_usage = 0; // (b-4) 누적사용량 : 연료 단위에 의존적임 (단위 0.1L)
    {
        target_pkt.total_distance = cur_obd_data.obd_data_total_distance; // (b-4) 총 누적거리 : 단위 m
        printf("   >> obd data pkt : total_distance [%d]\r\n",target_pkt.total_distance );
    }
    {
        target_pkt.brake_stat = cur_obd_data.obd_data_break_signal;  // (b-1) 현재 브레이크상태 : 0 : OFF, 1 : ON, 2 : 미지원
        printf("   >> obd data pkt : brake_stat [%d]\r\n",target_pkt.brake_stat );
    }
    target_pkt.over_accel_cnt = 0; // (b-4) 시동후 급가속 카운트 
    target_pkt.over_deaccel_cnt = 0; // (b-4) 시동후 급감속 카운트 
    target_pkt.car_inclination = 0; // (b-2) 차량기울기 : value/100
    target_pkt.engine_start_cnt = 0; // (b-1) 시동카운트  : 0~255증가 (하루기준)
    target_pkt.section_time = 0; // (b-4) 시동후 누적운행시간 : 단위 초
    {
        target_pkt.section_distance = cur_obd_data.obd_data_total_distance; // (b-4) 시동후 누적거리 : 단위 ?
        printf("   >> obd data pkt : section_distance [%d]\r\n", target_pkt.section_distance );
    }
    target_pkt.section_engine_idle_time = 0; // (b-4) 시동후 공회전시간 : 단위 초
    target_pkt.section_fuel_usage = 0; // (b-4) 시동후 누적연료소모량 ( 단위 0.1L)
    target_pkt.pannel_total_distance = 0; // (b-4) 계기판 누적거리 ( meter)
    target_pkt.pannel_section_distance = 0; // (b-4) 시동후 계기판 누적거리 ( meter)
    target_pkt.pannel_driving_availabe = 0; // (b-4) 주행가능거리 ( meter)
    target_pkt.reserved = 0; // (b-1) reserved

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

   // printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;
}

int parse_pkt__obd_data(ALLOC_PKT_RECV__OBD_DATA* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;

    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);
    
    printf("parse_pkt__obd_data ----------------------------------------------------------\r\n");
    
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__obd_data ----------------------------------------------------------\r\n");

    return 0;
    */
}




// ============================================================================================
// e_bcm_statting_val = 0x42,  // 0x42 : All key 설정 정보
// ============================================================================================

int make_pkt__bcm_statting_val(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__BCM_ALLKEY_DATA target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_bcm_statting_val;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
    _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());
    target_pkt.bcm_allkey_sn = 0; // (b-4) all key sn
    target_pkt.reserved[2] = 0; // (b-2) reserved
   

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

   // printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;
}

int parse_pkt__bcm_statting_val(ALLOC_PKT_RECV__BCM_ALLKEY_DATA* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;
    /*
    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);
    
    printf("parse_pkt__bcm_statting_val ----------------------------------------------------------\r\n");
    printf("recv_buff->bcm_allkey_gps_use [%d]\r\n", recv_buff->bcm_allkey_gps_use);
    printf("recv_buff->bcm_allkey_bt_use [%d]\r\n", recv_buff->bcm_allkey_bt_use);
    printf("recv_buff->bcm_allkey_rfid_use [%d]\r\n", recv_buff->bcm_allkey_rfid_use);
    printf("recv_buff->bcm_allkey_smoke_sensor_use [%d]\r\n", recv_buff->bcm_allkey_smoke_sensor_use);
    printf("recv_buff->bcm_allkey_oil_card_use [%d]\r\n", recv_buff->bcm_allkey_oil_card_use);
    printf("recv_buff->bcm_allkey_beacon_tx_power [%d]\r\n", recv_buff->bcm_allkey_beacon_tx_power);
    printf("recv_buff->bcm_allkey_smoke_co2 [%d]\r\n", recv_buff->bcm_allkey_smoke_co2);
    printf("recv_buff->bcm_allkey_smoke_led [%d]\r\n", recv_buff->bcm_allkey_smoke_led);
    printf("recv_buff->bcm_allkey_smoke_beep [%d]\r\n", recv_buff->bcm_allkey_smoke_beep);
    printf("recv_buff->bcm_allkey_rfid_tag_success_led [%d]\r\n", recv_buff->bcm_allkey_rfid_tag_success_led);
    printf("recv_buff->bcm_allkey_rfid_tag_success_beep [%d]\r\n", recv_buff->bcm_allkey_rfid_tag_success_beep);
    printf("recv_buff->bcm_allkey_rfid_tag_fail_led [%d]\r\n", recv_buff->bcm_allkey_rfid_tag_fail_led);
    printf("recv_buff->bcm_allkey_rfid_tag_fail_beep [%d]\r\n", recv_buff->bcm_allkey_rfid_tag_fail_beep);
    printf("recv_buff->bcm_allkey_oil_card_id [%s]\r\n", recv_buff->bcm_allkey_oil_card_id);
    printf("recv_buff->bcm_allkey_oil_card_out_noti [%d]\r\n", recv_buff->bcm_allkey_oil_card_out_noti);
    printf("recv_buff->bcm_allkey_oil_card_in_led [%d]\r\n", recv_buff->bcm_allkey_oil_card_in_led);
    printf("recv_buff->bcm_allkey_oil_card_in_beep [%d]\r\n", recv_buff->bcm_allkey_oil_card_in_beep);
    printf("recv_buff->bcm_allkey_oil_card_out_led [%d]\r\n", recv_buff->bcm_allkey_oil_card_out_led);
    printf("recv_buff->bcm_allkey_oil_card_out_beep [%d]\r\n", recv_buff->bcm_allkey_oil_card_out_beep);
    printf("recv_buff->reserved [%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__bcm_statting_val ----------------------------------------------------------\r\n");

    return 0;
    */
}







// e_bcm_stat_evt = 0x41,      // 0x41 : All key 상태 정보 (이벤트)





// ============================================================================================
// e_sms_recv_info : // 0xF0 : SMS 수신 정보
// ============================================================================================
int make_pkt__sms_recv_info(unsigned char **pbuf, unsigned short *packet_len, ALLOC_PKT_SEND__SMS_PKT_ARG sms_pkt_arg)
{
    ALLOC_PKT_SEND__SMS_RECV_INFO target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_sms_recv_info;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
    _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());

    target_pkt.sms_cmd_seq = sms_pkt_arg.sms_cmd_seq;
    target_pkt.sms_cmd_code = sms_pkt_arg.sms_cmd_code;
    target_pkt.sms_cmd_success = sms_pkt_arg.sms_cmd_success;
    strcpy(target_pkt.sms_cmd_contents, sms_pkt_arg.sms_cmd_contents);

    printf("target_pkt.sms_cmd_seq => [%d]\r\n", target_pkt.sms_cmd_seq);
    printf("target_pkt.sms_cmd_code => [%d]\r\n", target_pkt.sms_cmd_code);
    printf("target_pkt.sms_cmd_success => [%d]\r\n", target_pkt.sms_cmd_success);
    printf("target_pkt.sms_cmd_contents => [%s]\r\n", target_pkt.sms_cmd_contents);

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

   // printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;

}


int parse_pkt__sms_recv_info(ALLOC_PKT_RECV__SMS_RECV_INFO* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;
    /*
    printf("parse_pkt__sms_recv_info ----------------------------------------------------------\r\n");
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__sms_recv_info ----------------------------------------------------------\r\n");

    return 0;
    */
}



// ============================================================================================
// e_firm_info = 0x71, // 0x71 : 펌웨어 정보
// ============================================================================================
int make_pkt__firmware_info(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__FIRMWARE_INFO target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_firm_info;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
    _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    //target_pkt.mdm_sn = htonl(_get_phonenum_int_type());

    strcpy(target_pkt.firm_mdm_ver, PRG_VER);
    printf(" firm info -> mdm ver : [%s]\r\n", target_pkt.firm_mdm_ver);
    {
        SECO_OBD_INFO_T obd_info;
        alloc2_obd_mgr__get_obd_dev_info(&obd_info);
        strcpy(target_pkt.firm_obd_ver, obd_info.obd_swver);
        printf(" firm info -> obd ver : [%s]\r\n", target_pkt.firm_obd_ver);
    }
    //target_pkt->firm_car_code;
    //target_pkt->firm_car_code_version;
    //target_pkt->firm_allkey_ver;
    //target_pkt->firm_bcm_ver;
    //target_pkt->firm_dtg_ver;
    //target_pkt->reserved;

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

   // printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;

}


int parse_pkt__firm_info(ALLOC_PKT_SEND__FIRMWARE_INFO* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0] + recv_buff->reserved[1] ;
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    if ( pkt_ret_code == 0 )
        return 0;
    else 
        return -1;
    /*
    printf("parse_pkt__firm_info ----------------------------------------------------------\r\n");
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__firm_info ----------------------------------------------------------\r\n");
    */

}


// ============================================================================================
// e_bcm_knocksensor_setting_val = 0x51, // 0x51 : 노크센서 설정 정보
// ============================================================================================
int make_pkt__bcm_knocksensor_setting_val(unsigned char **pbuf, unsigned short *packet_len)
{
    ALLOC_PKT_SEND__KNOCK_SENSOR_INFO_REQ target_pkt;

    int pkt_total_len = sizeof(target_pkt);
    int pkt_id_code = e_bcm_knocksensor_setting_val;

    unsigned char *packet_buf;
    memset(&target_pkt, 0x00, pkt_total_len);

    LOGT(eSVC_MODEL, "%s() START\r\n", __func__);

    // make header
    _make_common_header(&target_pkt.header, pkt_total_len, pkt_id_code);

    // make body..
    // nothing...

    // alloc memory .. ---------------------------------------
    *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);

    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &target_pkt, pkt_total_len);

   // printf("%s() => evt [%d] dump pkt -------------------------------------------------\r\n", __func__, pkt_id_code);    mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // mds_api_debug_hexdump_buff(packet_buf, pkt_total_len);
   // printf("------------------------------------------------------\r\n");

    return 0;

}


int parse_pkt__bcm_knocksensor_setting_val(ALLOC_PKT_RECV__KNOCK_SENSOR_INFO_REQ* recv_buff)
{
    int pkt_ret_code = 0;
    pkt_ret_code = recv_buff->reserved[0];
    
    LOGT(eSVC_MODEL, "%s() PARSE RET [%d]\r\n", __func__, pkt_ret_code);
    
    printf("parse_pkt__bcm_knocksensor_setting_val ----------------------------------------------------------\r\n");
    printf("recv_buff->id => [0x%x]\r\n", recv_buff->id);
    printf("recv_buff->master_number => [0x%x]\r\n", recv_buff->master_number);
    printf("recv_buff->reserved => [0x%x]\r\n", recv_buff->reserved);
    printf("parse_pkt__bcm_knocksensor_setting_val ----------------------------------------------------------\r\n");
    
    if ( pkt_ret_code == 0 )
    {
        set_bcm_knocksensor_val_id_pass(recv_buff->id, recv_buff->master_number);
        return 0;
    }
    else
        return -1;
    /*
    printf("parse_pkt__firm_info ----------------------------------------------------------\r\n");
    printf("recv_buff->header.pkt_total_len => [%d]\r\n", recv_buff->header.pkt_total_len);
    printf("recv_buff->header.mdm_phonenum => [%d]\r\n", recv_buff->header.mdm_phonenum);
    printf("recv_buff->header.time_stamp => [%d]\r\n", recv_buff->header.time_stamp);
    printf("recv_buff->header.msg_type => [%d]\r\n", recv_buff->header.msg_type);
    printf("recv_buff->header.reserved => [0x%x]\r\n", recv_buff->header.reserved);

    printf("recv_buff->reserved => [0x%s]\r\n", recv_buff->reserved);

    printf("parse_pkt__firm_info ----------------------------------------------------------\r\n");
    */

}

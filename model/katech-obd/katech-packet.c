#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <base/config.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <logd/logd_rpc.h>
#include <callback.h>
#include <base/sender.h>
#include <board/modem-time.h>

#include "data-list.h"
#include "config.h"
#include "netcom.h"

#include "katech-tools.h"
#include "katech-packet.h"
#include "seco_obd_1.h"
#include "seco_obd_mgr.h"
#include "katech-data-calc.h"

#define LOG_TARGET eSVC_NETWORK

// --------------------------------------------------------------------
// 1. auth packet api.
// --------------------------------------------------------------------
int katech_pkt_auth_send()
{
	int ret = 0;
	katech_tools__set_svr_stat(KATECH_SVR_STAT_AUTH_RUNNING);
	ret = sender_add_data_to_buffer(KATECH_PKT_ID_AUTH, NULL, ePIPE_1);
	return ret;
}

int katech_pkt_auth_make(unsigned short *size, unsigned char **pbuf)
{
	KATCH_PKT_AUTH_REQ* packet;

	unsigned char *packet_buf;
	int packet_len = sizeof(KATCH_PKT_AUTH_REQ);
	
	// 인증이 안되어있는 상태에서만 유효하다.
	if (!(( katech_tools__get_svr_stat() == KATECH_SVR_STAT_NONE ) || (katech_tools__get_svr_stat() == KATECH_SVR_STAT_AUTH_RUNNING)))
		return KATECH_PKT_RET_FAIL_NO_AUTH;
	
	// alloc packet
	packet_buf = (unsigned char *) malloc (packet_len);
	
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return KATECH_PKT_RET_FAIL_CANNOT_MALLOC;
	}
	
	*pbuf = packet_buf;
	*size = packet_len;
	
	// ----------------------------------------
	// ----------------------------------------
	packet = (KATCH_PKT_AUTH_REQ*) packet_buf;
	memset(packet, 0x00, sizeof(KATCH_PKT_AUTH_REQ));
	
	// header..
	packet->header.pkt_start = KATECH_PKT_START_CHAR;
	packet->header.pkt_num = 0x01;
	packet->header.protocol_ver = 0x01;
	katech_tools__get_dev_id(packet->header.dev_num);
	printf("packet->header.dev_num is [%s]\r\n",packet->header.dev_num);
	memset(packet->header.auth_key, 0x00, 16);
	
	// body..
	packet->body_cnt = 1;
	memset(&packet->auth_body_content, 0x00, 100);

	// tail..
	packet->tail.body_chksum = get_checksum((unsigned char*) packet->auth_body_content, 100);
	packet->tail.pkt_end = KATECH_PKT_LAST_CHAR;
	
	return KATECH_PKT_RET_SUCCESS;
}

int katech_pkt_auth_parse(int res, KATCH_PKT_AUTH_RESP* packet)
{
	char tmp_str[128]= {0,};
    static int auth_fail_cnt = 0;

	if (( res != 0 ) || ( packet->resp_code != 0 ))
	{
		printf("packet->resp_code [0x%x]\r\n", packet->resp_code);
		katech_tools__set_svr_stat(KATECH_SVR_STAT_AUTH_FAIL);
        LOGE(LOG_TARGET, "response err!!! [0x%x] \r\n", packet->resp_code);
        if ( auth_fail_cnt ++ > 50)
        {
            devel_webdm_send_log("auth fail and resend.");
            katech_tools__set_svr_stat(KATECH_SVR_STAT_NONE);
            auth_fail_cnt = 0;
        }
		return -1;
	}
	
    auth_fail_cnt = 0;
	// 여기까지왔으면, 인증성공
	
	printf("packet->header.pkt_start = [%c]\r\n", packet->header.pkt_start);
	printf("packet->header.pkt_num = [%d]\r\n", packet->header.pkt_num);
	printf("packet->header.protocol_ver = [%d]\r\n",packet->header.protocol_ver);

	{
		memset(&tmp_str, 0x00, sizeof(tmp_str));
		memcpy(&tmp_str, packet->header.auth_key, 16);
		LOGI(LOG_TARGET, "packet->header.auth_key = [%s]\r\n", tmp_str);
		// strncpy(g_pkt_stat.auth_key, packet->header.auth_key, 16);
		katech_tools__set_auth_key(tmp_str);
	}

	printf("packet->header.dev_num = [%s]\r\n", packet->header.dev_num);
	printf("packet->body_cnt = [%d]\r\n", packet->body_cnt );
	printf("packet->resp_code = [%d]\r\n", packet->resp_code );
	printf("packet->resp_msg_1 = [%s]\r\n", packet->resp_msg_1 );
	printf("packet->resp_msg_2 = [%s]\r\n", packet->resp_msg_2 );
	printf("packet->encrypt_key = [%s]\r\n", packet->encrypt_key);
	
	{
		memset(&tmp_str, 0x00, sizeof(tmp_str));
		memcpy(&tmp_str, packet->server_ip, 15);
		printf("packet->server_ip = [%s]\r\n",tmp_str);
		katech_tools__set_server_ip(tmp_str);
	}
	{
		memset(&tmp_str, 0x00, sizeof(tmp_str));
		memcpy(&tmp_str, packet->server_port, 5);
		printf("packet->server_port = [%s]\r\n", tmp_str);
		katech_tools__get_server_port (atoi ( tmp_str ));
	}
	
	printf("packet->tail.body_chksum = [%d]\r\n", packet->tail.body_chksum);
	
	printf("packet->tail.pkt_end = [%d]\r\n", packet->tail.pkt_end );

	katech_tools__set_svr_stat(KATECH_SVR_STAT_AUTH_SUCCESS);
    LOGI(LOG_TARGET, "AUTH PARSE SUCCESS!!\r\n");
	return 0;
}

// --------------------------------------------------------------------
// 2. fw check packet
// --------------------------------------------------------------------
int katech_pkt_fw_send()
{
	return sender_add_data_to_buffer(KATECH_PKT_ID_FW_CHK, NULL, ePIPE_1);
}

int katech_pkt_fw_make(unsigned short *size, unsigned char **pbuf)
{
	KATCH_PKT_FW_REQ* packet;

	unsigned char *packet_buf;
	int packet_len = sizeof(KATCH_PKT_AUTH_REQ);
	
	// 인증관련하여 실패되었으면 시도도 하지 않는다.
	if ( katech_tools__get_svr_stat() != KATECH_SVR_STAT_AUTH_SUCCESS )
		return KATECH_PKT_RET_FAIL_NO_AUTH;
	
	// alloc packet
	packet_buf = (unsigned char *)malloc(packet_len);
	
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return KATECH_PKT_RET_FAIL_CANNOT_MALLOC;
	}
	
	*pbuf = packet_buf;
	*size = packet_len;
	
	// ----------------------------------------
	packet = (KATCH_PKT_FW_REQ*) packet_buf;
	memset(packet, 0x00, sizeof(KATCH_PKT_FW_REQ));
	
	// header..
	packet->header.pkt_start = KATECH_PKT_START_CHAR;
	packet->header.pkt_num = 0x02;
	packet->header.protocol_ver = 0x01;
	katech_tools__get_auth_key(packet->header.auth_key);
	katech_tools__get_dev_id(packet->header.dev_num);
	
	// body
	packet->body_cnt = 100;
    // todo : implement
	//packet->obd_version = obd_api_get_cur_version();
	memset(&packet->fw_body_content, 0x00, 99);

	// tail..
	packet->tail.body_chksum = get_checksum((unsigned char*) packet->obd_version, packet->body_cnt);;
	packet->tail.pkt_end = KATECH_PKT_LAST_CHAR;
	
	return 0;
}

int katech_pkt_fw_parse(int res, KATCH_PKT_FW_RESP* packet)
{
	/*
	packet->header.pkt_start;
	packet->header.pkt_num;
	packet->header.protocol_ver;
	packet->header.auth_key;
	packet->header.dev_num;
	
	packet->body_cnt;
	packet->resp_code;
	packet->fw_path_1;
	packet->fw_path_2;
	packet->need_to_update;
	packet->curversion;
	packet->fw_dl_ip;
	packet->fw_dl_port;
	packet->fw_file_name;
	
	packet->tail.body_chksum;
	packet->tail.pkt_end;
	*/
	return 0;
}

// --------------------------------------------------------------------
// 3. report data api : inteval setting
// --------------------------------------------------------------------
int katech_pkt_1_insert_and_send(gpsData_t* p_gpsdata, int force_send)
{
	REPORT_DATA_1_BODY_DATA*   pkt_body = {0,};
	
	int res = 0;
	
	int tmp_int_val = 0;
    float tmp_float_val = 0;

	int collect_cnt = 0; 
	
	SECO_CMD_DATA_SRR_TA1_T srr_data_ta1;
	SECO_CMD_DATA_SRR_TA2_T srr_data_ta2;

    memset(&srr_data_ta1, 0x00, sizeof(srr_data_ta1));
    memset(&srr_data_ta2, 0x00, sizeof(srr_data_ta2));

    collect_cnt = get_report_interval();

	pkt_body = (REPORT_DATA_1_BODY_DATA *) malloc ( sizeof(REPORT_DATA_1_BODY_DATA) );
	
	if (pkt_body == NULL) 
	{
		LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
		return KATECH_PKT_RET_FAIL_CANNOT_MALLOC;
	}

	// GET OBD DATA...
	katech_obd_mgr__get_ta1_obd_info(&srr_data_ta1);
	katech_obd_mgr__get_ta2_obd_info(&srr_data_ta2);

	memset(pkt_body, 0x00, sizeof(REPORT_DATA_1_BODY_DATA));
	
	//printf("%s() - sizeof REPORT_DATA_1_OBD = [%d]\r\n", __func__, sizeof(REPORT_DATA_1_OBD));
	//printf("%s() - sizeof REPORT_DATA_1_MDM = [%d]\r\n", __func__, sizeof(REPORT_DATA_1_MDM));
	
	// memcpy(&mux_body, obd_buff1, sizeof(REPORT_DATA_1_BODY_DATA));
	
    // -------- modem time mux --------------
	{   // modem time mux
		struct tm time = {0,};
		get_modem_time_tm(&time);
		
		pkt_body->mdm_date = (time.tm_year + 1900)*10000 + (time.tm_mon+1)*100 + time.tm_mday;
		// printf("%s() - mux_body.mdm_date is [%d]\r\n", __func__, pkt_body->mdm_date);
		pkt_body->mdm_time = ( time.tm_hour*10000 + time.tm_min*100 + time.tm_sec ) * 100;
		// printf("%s() - mux_body.mdm_time is [%d]\r\n", __func__, pkt_body->mdm_time);
	}
    
    
	// -------- modem gps data  --------------
    {
        // gps mux
        pkt_body->mdm_gps_time = ( p_gpsdata->hour*10000 + p_gpsdata->min*100 + p_gpsdata->sec ) * 100;
        //printf("%s() - mux_body.gps_time is [%d]\r\n", __func__, pkt_body->mdm_gps_time);
        pkt_body->mdm_gps_long =  p_gpsdata->lat*1000000;
        //printf("%s() - mux_body.gps_long is [%d]\r\n", __func__, mux_body.gps_long);
        pkt_body->mdm_gps_lat = p_gpsdata->lon*1000000;
        //printf("%s() - p_gpsdata->lat is [%d]\r\n", __func__, mux_body.gps_lat);
        
        pkt_body->mdm_gps_attitude = p_gpsdata->altitude * 10;
        pkt_body->mdm_gps_heading = p_gpsdata->angle * 100;
        pkt_body->mdm_gps_speed = p_gpsdata->speed * 100;
        pkt_body->mdm_gps_num_of_sat = p_gpsdata->satellite;
    }

    // obd_trip_elapsed : since boot time sec
    tmp_int_val = get_running_time_sec();
    tmp_int_val = tmp_int_val * 1; // 20180123 fix : 100 -> 1
    _pkt_data_convert((int)tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_trip_elapsed) );          // 10 


    // obd_trip_elapsed : since boot time sec
    tmp_int_val = get_dev_boot_time();
    tmp_int_val = tmp_int_val * 100;
    _pkt_data_convert((int)tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_trip_num) );          // 10 

    
	//srr_data_ta1->obd_data[num].idx;
	//srr_data_ta1->obd_data[num].data;
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_CLV].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_calc_load_val) );          // 41

    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_MAP].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_intake_map ) );             // 42

    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_RPM].data;
    tmp_float_val = tmp_float_val * 4;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_rpm ) );                    // 43

    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_SPD].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_speed ) );                  // 44
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_MAF].data;
    //tmp_float_val = tmp_float_val * 0.01;
    tmp_float_val = tmp_float_val * 100; // FIXME: 180308 fix
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_air_flow_rate ) );          // 45
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_TPA].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_abs_throttle_posi ) );      // 46

    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_BS1].data;
    tmp_float_val = tmp_float_val * (65535/2); // FIXME: 180308 need not fix
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_lambda ) );                 // 47
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_BAV].data;
    tmp_float_val = tmp_float_val * 1000;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_ctrl_module_vol ) );        // 48

    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_APD].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_acc_pedal_posi ) );         // 49
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EFR].data;
    tmp_float_val = tmp_float_val * 20;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_engine_fule_rate ) );       // 50
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EAT].data;
    tmp_float_val = tmp_float_val + 125;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_actual_engine ) );          // 51
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_CED].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_command_egr ) );            // 52
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_AED].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_actual_egr_duty ) );        // 53
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EFT].data;
    tmp_float_val = tmp_float_val + 125;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_engine_friction ) );        // 54
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_COT].data;
    tmp_float_val = tmp_float_val + 40;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_engine_coolant_temp ) );    // 55
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_ATS].data;
    tmp_float_val = tmp_float_val + 40;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_intake_air_temp ) );        // 56
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_TB1].data;
    tmp_float_val = tmp_float_val + 40;
    tmp_float_val = tmp_float_val * 10;
    _pkt_data_convert((int)tmp_float_val, SIGNED_2_BYTE , (char*)(&pkt_body->obd_catalyst_temp_1 ) );        // 57
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EST].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_time_engine_start ) );      // 58
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_BRO].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_barometic_press ) );        // 59
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_ABT].data;
    tmp_float_val = tmp_float_val + 40;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_ambient_air_temp ) );       // 60
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA2_ERT].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_ref_torque ) );             // 61
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA2_DTC].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_4_BYTE , (char*)(&pkt_body->obd_mon_st_since_dtc_clr ) );   // 62
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA2_DTM].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_dist_travled_mil ) );       // 63
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA2_DTS].data;
    tmp_float_val = tmp_float_val * 1;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_dist_travled_dtc ) );       // 64
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EGR].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_spare_1_egr_cmd2 ) );       // 65
    
    tmp_float_val = srr_data_ta1.obd_data[eOBD_CMD_SRR_TA1_EGE].data;
    tmp_float_val = tmp_float_val * 2.55;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_1_BYTE , (char*)(&pkt_body->obd_spare_2_egr_err ) );        // 66
    
	// timeserise calc..

	//tmp_int_val = timeserise_calc__fuel_fr(&srr_data_ta1,&srr_data_ta2);
    tmp_float_val = timeserise_calc__fuel_fr(&srr_data_ta1,&srr_data_ta2);
    tmp_float_val = tmp_float_val * 100;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_fuel_flow_rate ) );        // 79

	tmp_int_val = timeserise_calc__engine_break_torque(&srr_data_ta1, &srr_data_ta2);
    tmp_int_val = tmp_int_val * 100;
    _pkt_data_convert(tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_engine_brake_torq ) );        // 80

	tmp_int_val = timeserise_calc__eng_break_pwr(&srr_data_ta1,&srr_data_ta2);
    tmp_int_val = tmp_int_val * 100;
    _pkt_data_convert(tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_engine_brake_pwr ) );        // 81

	tmp_int_val = timeserise_calc__exhaus_gas_mass_fr(&srr_data_ta1,&srr_data_ta2);
    //tmp_int_val = tmp_int_val * 100;
    tmp_int_val = tmp_int_val * 10; // FIXME: 180308 fix
    _pkt_data_convert(tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_exhaust_gas_flowrate ) );     // 82
    
	tmp_int_val = timeserise_calc__accessory_power(&srr_data_ta1,&srr_data_ta2);
    tmp_int_val = tmp_int_val * 100;
    _pkt_data_convert(tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_acc_power ) );              // 83

	tmp_float_val = timeserise_calc__acceleration(&srr_data_ta1,&srr_data_ta2);
    tmp_float_val = tmp_float_val + 0.64;
    tmp_float_val = tmp_float_val * 20;
    _pkt_data_convert((int)tmp_float_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_acceleration ) );       // 87

	tmp_int_val = timeserise_calc__corr_v_speed(&srr_data_ta1,&srr_data_ta2);
    tmp_int_val = tmp_int_val * 100;
    _pkt_data_convert(tmp_int_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->obd_cor_speed ) );          // 88

	tmp_int_val = timeserise_calc__garde(&srr_data_ta1,&srr_data_ta2);
    tmp_int_val = tmp_int_val + 127 ;
    _pkt_data_convert(tmp_int_val, SIGNED_1_BYTE , (char*)(&pkt_body->obd_road_gradient ) );      // 89
	//_debug_print_report1_pkt(pkt_body);
	// cpy buffer.
	//memcpy( pkt_body->obd_raw_data, &mux_body, 200);
	
    // debug test
    /*
    {
        int j = 0 ;
        for (j =0 ; j < 100 ; j ++)
            printf("data aa [%d] [0x%02x] \r\n", j, pkt_body->obd_raw_data[j] );
    }
    */  
	// buffer insert to list
	if( list_add(&katech_packet_list_1, (void *)pkt_body) < 0 )
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
		free(pkt_body);
		
		return KATECH_PKT_RET_FAIL;
	}
	
	// check auth status.
	if ( katech_tools__get_svr_stat() != KATECH_SVR_STAT_AUTH_SUCCESS )
		return KATECH_PKT_RET_FAIL_NO_AUTH;
	
	LOGI(LOG_TARGET, "DATA 1 PKT - make pkt time [%d]/[%d]\r\n", katech_packet_list_cnt_1(), collect_cnt);
    
	// interval atteribute
	if (( force_send == KATECH_PKT_INTERVAL_SEND ) && ( katech_packet_list_cnt_1() >= collect_cnt) )
	{
        int force_send_arg = force_send;
		LOGT(LOG_TARGET, "DATA 1 PKT - SEND TIME \r\n");
		res = sender_add_data_to_buffer(KATECH_PKT_ID_REPORT_1, &force_send_arg, ePIPE_1);
	}
	
	if ( force_send == KATECH_PKT_IMMEDIATELY_SEND )
	{
        int force_send_arg = force_send;
		res = sender_add_data_to_buffer(KATECH_PKT_ID_REPORT_1, &force_send_arg, ePIPE_1);
	}


	return KATECH_PKT_RET_SUCCESS;
}


int katech_pkt_report_data_1_make(unsigned short *size, unsigned char **pbuf, int pkt_att)
{
	unsigned char *packet_buf;
    
	int packet_len = 0; 
    unsigned short body_count = 0;
	int collect_cnt = 0;
	
    KATCH_PKT_HEADER    pkt_header = {0,};
    KATCH_PKT_TAIL      pkt_tail = {0,};
    
    int pkt_data_idx = 0;
    int i = 0 ;
    int res = 0;
    
	// 인증관련하여 실패되었으면 전송 시도도 하지 않는다.
	if ( katech_tools__get_svr_stat() != KATECH_SVR_STAT_AUTH_SUCCESS )
		return KATECH_PKT_RET_FAIL_NO_AUTH;
    
    collect_cnt = get_report_interval();
    
    if ( katech_packet_list_cnt_1() >= collect_cnt )
        body_count = collect_cnt;
    else
        body_count = katech_packet_list_cnt_1();
    
    packet_len = sizeof(KATCH_PKT_HEADER);
    packet_len += sizeof(body_count);
    packet_len += sizeof(REPORT_DATA_1_BODY_DATA) * body_count;
    packet_len += sizeof(KATCH_PKT_TAIL);
    
	packet_buf = (unsigned char *)malloc(packet_len);
	
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return -1;
	}
	
	memset(packet_buf, 0x00, packet_len);
	
    pkt_data_idx = 0;
    
    // make header ..
	pkt_header.pkt_start = KATECH_PKT_START_CHAR;
	pkt_header.pkt_num = 0x10;
	pkt_header.protocol_ver = 0x01;
	katech_tools__get_auth_key(pkt_header.auth_key);
	katech_tools__get_dev_id(pkt_header.dev_num);
	
    memcpy( (packet_buf + pkt_data_idx), &pkt_header, sizeof(KATCH_PKT_HEADER) );
    pkt_data_idx += sizeof(KATCH_PKT_HEADER);
    
    // make body count
    memcpy( (packet_buf + pkt_data_idx), &body_count, sizeof(body_count) );
    pkt_data_idx += sizeof(body_count);
    
    // make body contents
    for ( i = 0; i < body_count ; i++ )
    {
        REPORT_DATA_1_BODY_DATA* p_tmp_body;
        res = list_pop(&katech_packet_list_1, (void *)(&p_tmp_body));
        
        if ( ( res < 0 ) || ( p_tmp_body == NULL ) )
        {
			//printf("%s() - %d\r\n", __func__, __LINE__ );
			memset ( p_tmp_body, 0x00, sizeof(REPORT_DATA_1_BODY_DATA) );
		}

        // memset ( p_tmp_body, 0x00, sizeof(REPORT_DATA_1_BODY_DATA) );
		//printf("send pkt -------------------------------------\r\n");
		//_debug_print_report1_pkt(p_tmp_body);

        memcpy( (packet_buf + pkt_data_idx), p_tmp_body, sizeof(REPORT_DATA_1_BODY_DATA));
        pkt_data_idx += sizeof(REPORT_DATA_1_BODY_DATA);
        
        if ( p_tmp_body != NULL )
			free( p_tmp_body );
        
    }
    
	// make tail...
	pkt_tail.body_chksum = get_checksum(packet_buf + sizeof(KATCH_PKT_HEADER) + sizeof(body_count), (sizeof(REPORT_DATA_1_BODY_DATA) * body_count)) ;
	//printf("%s() - %d :: check sum is [%d]\r\n", __func__, __LINE__, packet->tail.body_chksum );
	pkt_tail.pkt_end = KATECH_PKT_LAST_CHAR;
	
    memcpy( (packet_buf + pkt_data_idx), &pkt_tail, sizeof(KATCH_PKT_TAIL));
    pkt_data_idx += sizeof(KATCH_PKT_TAIL);
    
    printf("DATA 1 PKT MAKE :: pkt_data_idx is [%d]/[%d]\r\n",pkt_data_idx, packet_len);
    
	*pbuf = packet_buf;
	*size = packet_len;
    
	return KATECH_PKT_RET_SUCCESS;
}

int katech_pkt_report_data_1_resp(int res, KATCH_PKT_REPORT_DATA_1_RESP* packet)
{
    LOGI(LOG_TARGET, "report 1 :: res = [%d]\r\n", res);
    LOGI(LOG_TARGET, "report 1 :: packet->resp_code = [%d]\r\n", packet->resp_code);

    if (( res != 0 ) || ( packet->resp_code != 0x00))
    {
        LOGE(LOG_TARGET, "report 1 :: packet->resp_code = ERR \r\n");
        return KATECH_PKT_RET_FAIL;
    }

    printf("report 1 :: packet->body_cnt = [%d]\r\n", packet->body_cnt);
    //printf("report 1 :: packet->resp_msg_1 = [%s]\r\n", packet->resp_msg_1);
    //printf("report 1 :: packet->resp_msg_2 = [%s]\r\n", packet->resp_msg_2);

	return KATECH_PKT_RET_SUCCESS;
}


// --------------------------------------------------------------------
// 
// --------------------------------------------------------------------

int katech_pkt_2_insert_and_send()
{
	REPORT_DATA_2_BODY_DATA*   pkt_body = {0,};
	
    float tmp_float_val = 0;

	pkt_body = (REPORT_DATA_2_BODY_DATA *) malloc ( sizeof(REPORT_DATA_2_BODY_DATA) );
	
	if (pkt_body == NULL) 
	{
		LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
		return KATECH_PKT_RET_FAIL_CANNOT_MALLOC;
	}

	memset(pkt_body, 0x00, sizeof(REPORT_DATA_2_BODY_DATA));
	
	pkt_body->mdm_dev_id = tripdata__get_dev_id();;
	tripdata__get_car_vin((char*)pkt_body->mdm_char_vin);
	pkt_body->tripdata_payload = tripdata__get_pay_load() * 1/5;
	pkt_body->tripdata_total_time = tripdata__get_total_time_sec();
	pkt_body->tripdata_driving_time = tripdata__get_driving_time_sec();
	pkt_body->tripdata_stop_time = tripdata__get_stoptime_sec();
	pkt_body->tripdata_driving_dist = tripdata__get_driving_distance_km() * 100;
	pkt_body->tripdata_num_of_stop = tripdata__get_stop_cnt();
	pkt_body->tripdata_mean_spd_w_stop = tripdata__get_total_speed_avg() * 100;
	pkt_body->tripdata_mean_spd_wo_stop = tripdata__get_run_speed_avg() * 100;
	pkt_body->tripdata_acc_rate = tripdata__get_accelation_rate() * 255/100;
	pkt_body->tripdata_dec_rate = tripdata__get_deaccelation_rate() * 255/100;
	pkt_body->tripdata_cruise_rate = tripdata__get_cruise_rate() * 255/100;
	pkt_body->tripdata_stop_rate = tripdata__get_stop_rate() * 255/100;
	pkt_body->tripdata_pke = tripdata__get_PKE() * 25;
	pkt_body->tripdata_rpa = tripdata__get_RPA() * 25;
	pkt_body->tripdata_mean_acc = tripdata__get_acc_avg() * 25;
	pkt_body->tripdata_cold_rate = tripdata__get_cold_rate() * 255/100;
	pkt_body->tripdata_warm = tripdata__get_warm_rate() * 255/100;
	pkt_body->tripdata_hot = tripdata__get_hot_rate() * 255/100;
	pkt_body->tripdata_fuel_usage = tripdata__get_fuel_useage() * 100;
	tmp_float_val = tripdata__get_fuel_economy();
    tmp_float_val = tmp_float_val * 100.0;
    _pkt_data_convert((int)tmp_float_val, UNSIGNED_2_BYTE , (char*)(&pkt_body->tripdata_fuel_eco ) ); 

    devel_webdm_send_log("tripdata__get_fuel_economy() => [%d] [%f]\r\n",pkt_body->tripdata_fuel_eco, tripdata__get_fuel_economy());
	// pkt_body->tripdata_trip_spare_1;
	// pkt_body->tripdata_trip_spare_2;
	// pkt_body->tripdata_trip_spare_3;
	// pkt_body->tripdata_trip_spare_4;
	// pkt_body->tripdata_trip_spare_5;
	// pkt_body->tripdata_trip_spare_6;
	// pkt_body->tripdata_trip_spare_7;
	// pkt_body->tripdata_trip_spare_8;
	// pkt_body->tripdata_trip_spare_9;
	// pkt_body->tripdata_trip_spare_10;
	// pkt_body->tripdata_trip_spare_11;
	// pkt_body->tripdata_trip_spare_12;
	// pkt_body->tripdata_trip_spare_13;
	// pkt_body->tripdata_trip_spare_14;
	// pkt_body->tripdata_trip_spare_15;
	// pkt_body->tripdata_trip_spare_16;
	// pkt_body->tripdata_trip_spare_17;
	// pkt_body->tripdata_trip_spare_18;
	pkt_body->trip_start_date = tripdata__get_start_date();
	pkt_body->trip_start_time = tripdata__get_start_time();
	pkt_body->trip_end_date = tripdata__get_end_date();
	pkt_body->trip_end_time = tripdata__get_end_time();
	// pkt_body->trip_spare_23;
	// pkt_body->trip_spare_24;

	if( list_add(&katech_packet_list_2, (void *)pkt_body) < 0 )
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
		free(pkt_body);
		
		return KATECH_PKT_RET_FAIL;
	}
	
	// check auth status.
	if ( katech_tools__get_svr_stat() != KATECH_SVR_STAT_AUTH_SUCCESS )
		return KATECH_PKT_RET_FAIL_NO_AUTH;
	
    {
        int pkt_arg = KATECH_PKT_IMMEDIATELY_SEND;
	    sender_add_data_to_buffer(KATECH_PKT_ID_REPORT_2, &pkt_arg, ePIPE_1);
    }

	return KATECH_PKT_RET_SUCCESS;
}



int katech_pkt_report_data_2_make(unsigned short *size, unsigned char **pbuf, int pkt_att)
{
	unsigned char *packet_buf;
    
	int packet_len = 0; 
    unsigned short body_count = 0;
	//int collect_cnt = 0;
	
    KATCH_PKT_HEADER    pkt_header = {0,};
    KATCH_PKT_TAIL      pkt_tail = {0,};
    
    int pkt_data_idx = 0;
    int i = 0 ;
    int res = 0;

	// 인증관련하여 실패되었으면 전송 시도도 하지 않는다.
	if ( katech_tools__get_svr_stat() != KATECH_SVR_STAT_AUTH_SUCCESS )
		return KATECH_PKT_RET_FAIL_NO_AUTH;
    
    if ( katech_packet_list_cnt_2() >= 1 )
        body_count = 1;
    else
        return 0;
	
    packet_len = sizeof(KATCH_PKT_HEADER);
    packet_len += sizeof(body_count);
    packet_len += sizeof(REPORT_DATA_2_BODY_DATA) * body_count;
    packet_len += sizeof(KATCH_PKT_TAIL);
    
	packet_buf = (unsigned char *)malloc(packet_len);
	
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return -1;
	}
	
	memset(packet_buf, 0x00, packet_len);
	
    pkt_data_idx = 0;
	
    // make header ..
	pkt_header.pkt_start = KATECH_PKT_START_CHAR;
	pkt_header.pkt_num = 0x11;
	pkt_header.protocol_ver = 0x01;
	katech_tools__get_auth_key(pkt_header.auth_key);
	katech_tools__get_dev_id(pkt_header.dev_num);
	
    memcpy( (packet_buf + pkt_data_idx), &pkt_header, sizeof(KATCH_PKT_HEADER) );
    pkt_data_idx += sizeof(KATCH_PKT_HEADER);
    
    // make body count
    memcpy( (packet_buf + pkt_data_idx), &body_count, sizeof(body_count) );
    pkt_data_idx += sizeof(body_count);
    
    // make body contents
    for ( i = 0; i < body_count ; i++ )
    {
        REPORT_DATA_2_BODY_DATA* p_tmp_body;
        res = list_pop(&katech_packet_list_2, (void *)(&p_tmp_body));
        
        if ( ( res < 0 ) || ( p_tmp_body == NULL ) )
        {
			//printf("%s() - %d\r\n", __func__, __LINE__ );
			memset ( p_tmp_body, 0x00, sizeof(REPORT_DATA_2_BODY_DATA) );
		}

        // memset ( p_tmp_body, 0x00, sizeof(REPORT_DATA_1_BODY_DATA) );
		//printf("send pkt -------------------------------------\r\n");
		_debug_print_report2_pkt(p_tmp_body);

        memcpy( (packet_buf + pkt_data_idx), p_tmp_body, sizeof(REPORT_DATA_2_BODY_DATA));
        pkt_data_idx += sizeof(REPORT_DATA_2_BODY_DATA);
        
        if ( p_tmp_body != NULL )
			free( p_tmp_body );
        
    }
	
	// make tail...
	pkt_tail.body_chksum = get_checksum(packet_buf + sizeof(KATCH_PKT_HEADER) + sizeof(body_count), (sizeof(REPORT_DATA_2_BODY_DATA) * body_count)) ;
	//printf("%s() - %d :: check sum is [%d]\r\n", __func__, __LINE__, packet->tail.body_chksum );
	pkt_tail.pkt_end = KATECH_PKT_LAST_CHAR;
	
    memcpy( (packet_buf + pkt_data_idx), &pkt_tail, sizeof(KATCH_PKT_TAIL));
    pkt_data_idx += sizeof(KATCH_PKT_TAIL);
	
    printf("DATA 2 PKT MAKE :: pkt_data_idx is [%d]/[%d]\r\n",pkt_data_idx, packet_len);
    
	*pbuf = packet_buf;
	*size = packet_len;
    
	return KATECH_PKT_RET_SUCCESS;
}

int katech_pkt_report_data_2_resp(int res, KATCH_PKT_REPORT_DATA_2_RESP* packet)
{
    printf("report 2 :: res = [%d]\r\n", res);

    printf("report 2 :: packet->body_cnt = [%d]\r\n", packet->body_cnt);
    printf("report 2 :: packet->resp_code = [%d]\r\n", packet->resp_code);
//    printf("report 2 :: packet->resp_msg_1 = [%s]\r\n", packet->resp_msg_1);
 //   printf("report 2 :: packet->resp_msg_2 = [%d]\r\n", packet->resp_msg_2);
    if ( ( res != 0 ) || ( packet->resp_code != 0x00) )
    {
        LOGE(LOG_TARGET, "report 2 :: packet->resp_code = ERR \r\n");
        return KATECH_PKT_RET_FAIL;
    }

	return 0;
}





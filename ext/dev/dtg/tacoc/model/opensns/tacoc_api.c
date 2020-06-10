/*
 * tacoc_api.c
 *
 *  Created on: 2013. 8. 28.
 *      Author: gbuddha
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>

#include <util/nettool.h>

#include <board/board_system.h>
#include <standard_protocol.h>
#include "dtg_data_manage.h"
#include "mdt_data_manage.h"
#include "dtg_net_com.h"
#include "sms_msg_process.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include "parsing.h"
#include "server_packet.h"
#include "dtg_ini_utill.h"
#include <base/dmmgr.h>


int breakdown_report(int integer)
{
	return 0;
}

int process_result_data(int env_info_check, unsigned char *data, int data_len)
{
	int idx = 0;
	int i;
	int j;
	int err_len;
	message_define_t *p_mdf;
	message_header_t *p_mhdr;
	unsigned char alert_code = 0;
	unsigned char alert_length = 0;
	char msg_buffer[512];
	message_env_pack_t *p_env_pack;

	if(data_len < sizeof(message_define_t) + sizeof(message_header_t))
		return -1;

	idx = 0;
	p_mdf = (message_define_t *)&data[0];
	idx += sizeof(message_define_t);
	p_mhdr = (message_header_t *)&data[idx];
	idx += sizeof(message_header_t);

	p_mhdr->return_code = ntohs(p_mhdr->return_code);

	if(p_mhdr->return_code != 0)
	{
		//response error
		err_len = data_len - ( sizeof(message_define_t) + sizeof(message_header_t) );
		printf("p_mhdr->return_code = [%d]\n", p_mhdr->return_code);
		DTG_LOGE("error response code(%d), err_len[%d]", p_mhdr->return_code, err_len);
		if(err_len > 0)
		{
			j = 0;
			for(i = 0; i < err_len; i++) {
				if(data[i] != 0x00)
					msg_buffer[j++] = data[i];
			}
			msg_buffer[j] = 0x00;
			printf("error msg = [%s]\n", msg_buffer);
		}
		return -2;
	}

	alert_code = data[idx++];
	memcpy(&alert_length, &data[idx], 2);
	idx += 2;
	alert_length = ntohs(alert_length);
	printf("alert_code = [%d], alert_length = [%d]\n", alert_code, alert_length);
	if(alert_length > 0)
	{
		memcpy(&msg_buffer, &data[idx], alert_length);
		idx += alert_length;
	}

	if(env_info_check == 1 && (data_len-idx) >= sizeof(message_env_pack_t))
	{
		p_env_pack = (message_env_pack_t *)&data[idx];
		idx += sizeof(message_env_pack_t);

		p_env_pack->state_send_cycle = ntohs(p_env_pack->state_send_cycle);
		p_env_pack->data_send_cycle = ntohs(p_env_pack->data_send_cycle);

		if(p_env_pack->state_send_cycle != get_mdt_report_period() || p_env_pack->gps_interval != get_mdt_create_period())
		{
			set_mdt_report_period(p_env_pack->state_send_cycle);
			set_mdt_create_period(p_env_pack->gps_interval);
			save_ini_mdt_period_info();

			set_dtg_mdt_period_update_enable(1); //dtg config data refress(gps interal)
		}

		if(p_env_pack->data_send_cycle*60 != get_dtg_report_period())
		{
			set_dtg_report_period(p_env_pack->data_send_cycle*60);
			save_ini_dtg_period_info();
		}

		
		
		DTG_LOGI("Response Success : %d, %d, %d, %d, %d, %d, %d",	p_env_pack->dev_ctrl, 
																p_env_pack->state_sned_ctrl,
																p_env_pack->state_send_cycle,
																p_env_pack->data_send_ctrl,
																p_env_pack->data_send_cycle,
																p_env_pack->gps_ctrl,
																p_env_pack->gps_interval);
/*
		printf("dev_ctrl = [%d]\n", p_env_pack->dev_ctrl);
		printf("state_sned_ctrl = [%d]\n", p_env_pack->state_sned_ctrl);
		printf("state_send_cycle = [%d]\n", p_env_pack->state_send_cycle);
		printf("data_send_ctrl = [%d]\n", p_env_pack->data_send_ctrl);
		printf("data_send_cycle = [%d]\n", p_env_pack->data_send_cycle);
		printf("gps_ctrl = [%d]\n", p_env_pack->gps_ctrl);
		printf("gps_interval = [%d]\n", p_env_pack->gps_interval);
*/
	}

	return 0;
}

int status_data_send(unsigned char* stream, int stream_size, int code_line, char *msg)
{
	int sock_fd;
	int ret;
	unsigned char recv_buf[1024];

#if (0) //for debug send data dump ++
	int i;
	int j;
	j = 1;
	for(i = 0, j = 1; i < stream_size; i++, j++) {
		printf("%02x ", stream[i]);
		if( (j % 20) == 0)
			printf("\n");
	}
#endif //for debug send data dump --
	
	sock_fd = connect_to_server(get_mdt_server_ip_addr(), get_mdt_server_port());
	if (sock_fd < 0) {
		DTG_LOGE("server connection fail %s/%d", get_mdt_server_ip_addr(), get_mdt_server_port());
		return -1;
	}
	DTG_LOGI("server connection success %s/%d", get_mdt_server_ip_addr(), get_mdt_server_port());

	ret = send_to_dtg_server(sock_fd, stream, stream_size, (char *)__func__, code_line, msg);
	if(ret < 0)
	{
		DTG_LOGE("++++++Network Sending Error");
		close(sock_fd);
		return -2;
	}

	DTG_LOGI("waiting server response....");
	ret = receive_response(sock_fd, recv_buf, sizeof(recv_buf));
	if(ret <= 0) {
		DTG_LOGE("++++++Network Receive Error");
		close(sock_fd);
		return -3;
	}
	else {
		DTG_LOGI("Network received length = [%d]\n", ret);
		ret = process_result_data(1, recv_buf, ret);
		if(ret < 0) {
			close(sock_fd);
			return -4;
		}
	}

	close(sock_fd);

	return 0;	
}


int dtg_record_data_send(unsigned char* stream, int stream_size, char *packet_buf, int packet_buf_len, int code_line, char *msg)
{
	int sock_fd;
	int ret;
	unsigned char recv_buf[1024];
	int send_pack_size;
	unsigned short dtg_record_cnt;

	
	sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
	if (sock_fd < 0) {
		DTG_LOGE("server connection fail %s/%d", get_server_ip_addr(), get_server_port());
		return -1;
	}
	DTG_LOGI("DTG server connection success %s/%d key_status[%d]", get_server_ip_addr(), get_server_port(), get_dtg_key_status());

	
	//int dtg_data_header_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, unsigned short *dtg_record_pack_cnt)
	send_pack_size = dtg_data_header_parse(stream, stream_size, packet_buf, packet_buf_len, &dtg_record_cnt);
#if (0) //for debug send data dump ++
	int i;
	int j;
	j = 1;
	printf("dtg header packet....\n");
	for(i = 0, j = 1; i < send_pack_size; i++, j++) {
		printf("%02x ", packet_buf[i]);
		if( (j % 20) == 0)
			printf("\n");
	}
#endif //for debug send data dump --
	ret = send_to_dtg_server(sock_fd, packet_buf, send_pack_size, (char *)__func__, code_line, msg);
	if(ret < 0)
	{
		DTG_LOGE("++++++Network Sending Error#1");
		close(sock_fd);
		return -2;
	}

	DTG_LOGI("waiting response about dtg header packet response....");
	ret = receive_response(sock_fd, recv_buf, sizeof(recv_buf));
	if(ret <= 0) {
		DTG_LOGE("++++++Network Receive Error#2");
		close(sock_fd);
		return -3;
	}
	else {
		DTG_LOGI("Network received length = [%d]#3\n", ret);
		ret = process_result_data(0, recv_buf, ret);
		if(ret < 0) {
			close(sock_fd);
			return -4;
		}
	}
	DTG_LOGI("DTG Header Response Success : Record Count[%d]", dtg_record_cnt);
	send_pack_size = dtg_data_pack_parse(stream, stream_size, packet_buf, packet_buf_len, dtg_record_cnt);
#if (0) //for debug send data dump ++
	int l;
	int m;
	m = 1;
	printf("dtg data packet....\n");
	for(l = 0, m = 1; l < send_pack_size; l++, m++) {
		printf("%02x ", packet_buf[l]);
		if( (m % 20) == 0)
			printf("\n");
	}
#endif //for debug send data dump --
	
	ret = send_to_dtg_server(sock_fd, packet_buf, send_pack_size, (char *)__func__, code_line, msg);
	if(ret < 0)
	{
		DTG_LOGE("++++++Network Sending Error#4");
		close(sock_fd);
		return -2;
	}

	DTG_LOGI("waiting response about DTG Body Record packet response....");
	ret = receive_response(sock_fd, recv_buf, sizeof(recv_buf));
	if(ret <= 0) {
		DTG_LOGE("++++++Network Receive Error#5");
		close(sock_fd);
		return -3;
	}
	else {
		DTG_LOGI("Network received length = [%d]#6\n", ret);
		ret = process_result_data(0, recv_buf, ret);
		if(ret < 0) {
			close(sock_fd);
			return -4;
		}
	}
	DTG_LOGI("DTG Body Record Response Success");

	close(sock_fd);

	return 0;	
}

//message_define_t		 : 14 bytes
//message_header_t		 : 59 bytes
//gps_track_info_pack_t  : 12 bytes;
//gps track repeat count : 2 * MAX_GPS_TRACK_COUNT(500) (1000bytes)
//status resource data	 : RESOURCE_COUNT(4)*MAX_RESOURCE_DATA_LEN(15) -> 60bytes
//reserved				 : 4bytes
//sum                     1500 bytes
	
static unsigned char g_status_info_buf[4096];

int tx_data_to_tacoc(int type, char *stream, int len)
{
	int cnt;
	int pack_buffer_size = 0 ;
	int send_pack_size = 0;
	int key_status;
	int dtg_record_cnt = 0;
	unsigned char *dtg_pack_buf;
	
	DTG_LOGD("Tacoc Data Reception Success, Type[%d], length[%d]", type, len);
	printf("%s:%d> Tacoc Data Reception Success, Type[%d], length[%d]\n", __func__, __LINE__, type, len);
	if(len <= 0) 
	{
		return 0;
	}

	if (type == 1) //DTG DATA (fixed value)
	{

		cnt = len / sizeof(tacom_std_data_t) + 1;
		pack_buffer_size = sizeof(message_define_t) + sizeof(message_header_t) + sizeof(dtg_header_t) + sizeof(dtg_record_pack_t)*cnt;
		DTG_LOGD("pack_buffer malloc size = [%d]\n", pack_buffer_size);
		dtg_pack_buf = (unsigned char *)malloc(pack_buffer_size);
		if(dtg_pack_buf == NULL) {
			DTG_LOGE("dtg_buf mallock Error");
			return -2222;
		}

		if(dtg_record_data_send(stream, len, dtg_pack_buf, pack_buffer_size, __LINE__, "DTG Header/Record Msg") < 0) {
			free(dtg_pack_buf);
			return -2222; //network error
		}

		if(power_get_ignition_status() == POWER_IGNITION_ON) 
		{
			if(get_dtg_key_status() != eDTG_KeyOn)
				set_dtg_key_status(eDTG_KeyOn);
		}

		free(dtg_pack_buf);
		
	} 
	else if (type == 101) //MDT DATA (fixed value)
	{
		if(power_get_ignition_status() == POWER_IGNITION_ON)
			send_pack_size = status_data_parse(stream, len, g_status_info_buf, sizeof(g_status_info_buf), eDS_Key_On_Running);
		else
			send_pack_size = status_data_parse(stream, len, g_status_info_buf, sizeof(g_status_info_buf), eDS_KeyOff);
		
		if(send_pack_size <= 0) //data remove
			return 0;

		DTG_LOGI("==================================================\n");
		DTG_LOGI("1.send_pack_size = [%d]\n", send_pack_size);
		DTG_LOGI("==================================================\n");

		if(status_data_send(g_status_info_buf, send_pack_size, __LINE__, "MDT Packet DATA") < 0)
			return -2222; //network error

		return 0;
	}
	else if (type == 2) //Key On/Off Data
	{
		printf("max buffer size = [%d]\n", sizeof(g_status_info_buf));
		if(power_get_ignition_status() == POWER_IGNITION_ON)
			send_pack_size = status_data_parse(stream, len, g_status_info_buf, sizeof(g_status_info_buf), eDS_KeyOn);
		else
			send_pack_size = status_data_parse(stream, len, g_status_info_buf, sizeof(g_status_info_buf), eDS_KeyOff);

		DTG_LOGI("==================================================\n");
		DTG_LOGI("2.send_pack_size = [%d]\n", send_pack_size);
		DTG_LOGI("==================================================\n");
		if(status_data_send(g_status_info_buf, send_pack_size, __LINE__, "Key On/Off Msg") < 0)
			return -2222; //network error
	}
	else if (type == 3) //power on/off message
	{
		/*
		//ePOWER_SOURCE_CHANGE_EVT
		if(g_mdt_power_flag == 0)
		{
			parse_mdt_msg(stream, g_mdt_packet_buffer, ePOWER_SOURCE_CHANGE_EVT);
			send_pack_size = mdt_enc_msg(g_mdt_packet_buffer, g_mdt_enc_packet_buffer, sizeof(mdt800_packet_t));
			g_mdt_enc_packet_buffer[send_pack_size] = MDT800_PACKET_END_FLAG;
			if(mdt_send_record_data_msg(g_mdt_enc_packet_buffer, send_pack_size+1, __LINE__, "MDT Power Source") < 0) {
				return -2222; //network error
			}
			g_mdt_power_flag = 1;
		}

		if(g_dtg_power_flag == 0) 
		{
			send_pack_size = current_dtg_parsing(stream, len, g_packet_buffer, ePower_Event);
			DTG_LOGD("trasfer packet size = [%d]\n", send_pack_size);
			if(send_record_data_msg(g_packet_buffer, send_pack_size, __LINE__, "DTG Power Source") < 0) {
				return -2222; //network error
			}
		}
		g_dtg_power_flag = 0;
		g_mdt_power_flag = 0;
		*/
	}
	else if (type == 4) //Just Get Current DTG Data
	{
		//set_current_dtg_data(stream, len);
	}
	else if (type == 5) //MDT Report Data
	{

	}
		
	return 0;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
/*
	int i;
	int idx = 0;
	char sms_buf[128];
	int start_idx = 0;
	int message_len = 0;
	DTG_LOGI("sender = [%s], smsdata = [%s]\n", sender, smsdata);
#ifdef MDT_NOT_SEND_TEST_ENABLE
	if(!strcmp(smsdata, "abcd")) {
		if(g_mdt_send_flag == 0) 
			g_mdt_send_flag = 1;
		else
			g_mdt_send_flag = 0;
	}
#endif

	message_len = strlen(smsdata);
	if(message_len > 4)
	{
		if(!strncmp(smsdata, "[Web", 4)) {
			start_idx = 11;
		}
	}

	DTG_LOGI("start_idx = [%d]\n", start_idx);
	if(start_idx > 0) {
		message_len = strlen(&smsdata[start_idx]);
	}
	DTG_LOGI("real sms message length = [%d]\n", message_len);
	memset(sms_buf, 0x00, sizeof(sms_buf));
	for(i = start_idx; i < start_idx+message_len; i++) {
		if(smsdata[i] != 0x20)
			sms_buf[idx++] = smsdata[i];
	}
	sms_buf[idx] = 0x00;


	DTG_LOGD("patched sms_buf = [%s]\n", sms_buf);
	if(!strcmp(sms_buf, "$czASstsreq*"))
	{
		DTG_LOGD("SMS Vehicle Setting Value Request");
		sms_device_status_req(sender);
		return 0;
	}

	if(sms_buf[0] != '&')
		return 0;

	if(sms_buf[1] == 'C' && sms_buf[2] == '1' && sms_buf[3] == ',') //MDT IP/Port Config
	{
		DTG_LOGD("SMS Set MDT IP/Port Config");
		sms_set_mdt_svrip_info(&sms_buf[4]);
	}
	else if(sms_buf[1] == 'D' && sms_buf[2] == '1' && sms_buf[3] == ',') //DTG IP/Port Config
	{
		DTG_LOGD("SMS Set DTG IP/Port Config");
		sms_set_dtg_svrip_info(&sms_buf[4]);
	}
	else if(sms_buf[1] == '1' && sms_buf[2] == '0' && sms_buf[3] == ',') //Device Reset
	{
		DTG_LOGD("SMS Request Device Reset");
		sms_set_device_reset(&sms_buf[4]);
	}
	else if(sms_buf[1] == 'C' && sms_buf[2] == '2' && sms_buf[3] == ',') //MDT Report Time Config
	{
		DTG_LOGD("SMS Set MDT Report Time Config");
		sms_set_mdt_period(&sms_buf[4]);
	}
	else if(sms_buf[1] == 'D' && sms_buf[2] == '2' && sms_buf[3] == ',')	//DTG Report Time Config
	{
		DTG_LOGD("SMS Set DTG Report Time Config");
		sms_set_dtg_period(&sms_buf[4]);
	}
	else if(sms_buf[1] == 'e' && sms_buf[2] == '1' && sms_buf[3] == ',')	//DTG Setting Value Request #1
	{
		DTG_LOGD("SMS DTG Setting Value Request #1");
		sms_set_dtg_setting_value1(&sms_buf[4]);
	}
	else if(sms_buf[1] == 'e' && sms_buf[2] == '2' && sms_buf[3] == ',')	//DTG Setting Value Request #2
	{
		DTG_LOGD("SMS DTG Setting Value Request #2");
		sms_set_dtg_setting_value2(&sms_buf[4]);
	}
*/
	return 0;
}

//#define MAX_LAST_SENDING_SIZE	10*1024
#define MAX_LAST_SENDING_DATA_COUNT	600 //10 min
void mdmc_power_off()
{
	int ready_cnt = 0;
	int err_cnt = 0;
	int ret;
	int mdt_cnt = 0;
	int dtg_cnt = 0;
	int mdt_request_cnt = 0;
	int remain_count = 0;
	int mdt_report_flag = 0;
	int dtg_data_sent_count = 0;

	DTG_LOGI("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
	DTG_LOGI("mdmc_power_off---------------------->1");
	DTG_LOGI("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");


	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
		DTG_LOGE("tacoc : mdmc_power_off wait [%d][%d]\n", is_working_action(), ready_cnt);
	}

	DTG_LOGE("tacoc : mdmc_power_off #1\n");

	//dtg data ++
	while(1)
	{
		remain_count = tacom_unreaded_records_num();
		dtg_cnt = (remain_count & 0x0000FFFF);

		if(dtg_cnt <= 10)
			break;

		if(err_cnt > 5)
			break;

		if(dtg_data_sent_count > 3)
			break;

		if(dtg_cnt >= 10) {
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, MAX_LAST_SENDING_DATA_COUNT, 0, 2);
			if(ret == 0) //success
			{
				ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
				dtg_data_sent_count += 1;
				err_cnt = 0;
			}
			else
			{
				err_cnt += 1;
			}
		}
	}
	//dtg data --

	
	err_cnt = 0;
	while(1)
	{
		remain_count = tacom_unreaded_records_num();
		mdt_cnt = ((remain_count & 0xFFFF0000) >> 16);
		if(mdt_cnt <= 2)
			break;

		if(err_cnt > 5)
			break;

		//mdt data ++
		if(mdt_cnt >= 2)
		{
			mdt_request_cnt = (0x40000000 | (MAX_LAST_SENDING_DATA_COUNT & 0x0000FFFF));
			ret = data_req_to_taco_cmd(ACCUMAL_MDT_DATA, mdt_request_cnt, 0, 2);
			if(ret == 0) //success
			{
				ret = data_req_to_taco_cmd(CLEAR_DATA, 2, 0, 1); //mdt data clear
				err_cnt = 0;
				mdt_report_flag = 1;
			}
			else
			{
				err_cnt += 1;
			}
		}
	}
	//mdt data --


	remain_count = tacom_unreaded_records_num();
	mdt_cnt = ((remain_count & 0xFFFF0000) >> 16);
	dtg_cnt = (remain_count & 0x0000FFFF);

	if(dtg_cnt >= 10) //remain count
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save


	if(mdt_report_flag == 0)
		data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off

	send_device_de_registration();
	// dmmgr_send(eEVENT_PWR_OFF, NULL, 0);	
}


void ignition_off_send_server_data()
{
	int ready_cnt = 0;
	int err_cnt = 0;
	int ret;
	int mdt_cnt = 0;
	int dtg_cnt = 0;
	int mdt_request_cnt = 0;
	int remain_count = 0;
	int mdt_report_flag = 0;

	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
	}

	DTG_LOGI("tacoc : key off #1\n");
	//data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off

	while(1) {
		if(power_get_ignition_status() == POWER_IGNITION_ON)
			break;

		if(err_cnt > 5)
			break;

		remain_count = tacom_unreaded_records_num();
		mdt_cnt = ((remain_count & 0xFFFF0000) >> 16);
		dtg_cnt = (remain_count & 0x0000FFFF);
		DTG_LOGT("%s> mdt_cnt[%d], dtg_cnt[%d] err_cnt[%d]", __func__, mdt_cnt, dtg_cnt, err_cnt);

		if(mdt_cnt < 2 && dtg_cnt < 10)
			break;

		//dtg data ++
		if(dtg_cnt >= 10) {
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, MAX_LAST_SENDING_DATA_COUNT, 0, 2);
			if(ret == 0) //success
			{
				ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
				err_cnt = 0;
			}
			else
			{
				err_cnt += 1;
			}
		}
		//dtg data --

		//mdt data ++
		if(mdt_cnt >= 2)
		{
			mdt_request_cnt = (0x40000000 | (MAX_LAST_SENDING_DATA_COUNT & 0x0000FFFF));
			ret = data_req_to_taco_cmd(ACCUMAL_MDT_DATA, mdt_request_cnt, 0, 2);
			if(ret == 0) //success
			{
				ret = data_req_to_taco_cmd(CLEAR_DATA, 2, 0, 1); //mdt data clear
				err_cnt = 0;
				mdt_report_flag = 1;
			}
			else
			{
				err_cnt += 1;
			}
		}
		//mdt data --
	}

	remain_count = tacom_unreaded_records_num();
	mdt_cnt = ((remain_count & 0xFFFF0000) >> 16);
	dtg_cnt = (remain_count & 0x0000FFFF);

	if(dtg_cnt > 10) //remain count
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save

	if(mdt_report_flag == 0)
		data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off
}

void tacoc_ignition_off()
{
	ignition_off_send_server_data();

	send_device_de_registration();
}

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
#include <taco_rpc.h>

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
#include <base/dmmgr.h>

//etrace_ternminal_info_t g_dtg_info;

#define MAX_END_DTG_SIZE	8192
int breakdown_report(int integer)
{
	return 0;
}


int send_record_data_msg(unsigned char* stream, int stream_size, int line, char *msg)
{
	int sock_fd;
	int ret;
	unsigned char recv_buf[512];
	int packet_id;

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
	
	sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
	if (sock_fd < 0) {
		DTG_LOGE("server connection fail %s/%d", get_server_ip_addr(), get_server_port());
		return -1;
	}
	DTG_LOGI("server connection success %s/%d", get_server_ip_addr(), get_server_port());

	ret = send_to_dtg_server(sock_fd, stream, stream_size, (char *)__func__, line, msg);
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
		if(ret != 5) {
			DTG_LOGE("Network received length Error");
			close(sock_fd);
		}
		memcpy(&packet_id, recv_buf, 4);
		if(recv_buf[4] == '1')
			DTG_LOGI("Result Success> packet_id = [%d], resp code[%c]", packet_id, recv_buf[4]);
		else
			DTG_LOGE("Result Fail> packet_id = [%d], resp code[%c]", packet_id, recv_buf[4]);
	}

	close(sock_fd);
	return 0;	
}

int mdt_send_record_data_msg(unsigned char* stream, int stream_size, int line, char *msg)
{
	int sock_fd;
	int ret;

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

	ret = send_to_dtg_server(sock_fd, stream, stream_size, (char *)__func__, line, msg);
	if(ret < 0)
	{
		DTG_LOGE("++++++Network Sending Error");
		close(sock_fd);
		return -2;
	}
/*
	DTG_LOGI("waiting server response....");
	ret = receive_response(sock_fd, recv_buf, sizeof(recv_buf));
	if(ret <= 0) {
		DTG_LOGE("++++++Network Receive Error");
		close(sock_fd);
		return -3;
	}
	else {
		DTG_LOGI("Network received length = [%d]\n", ret);
		if(ret != 5) {
			DTG_LOGE("Network received length Error");
			close(sock_fd);
		}
		memcpy(&packet_id, recv_buf, 4);
		if(recv_buf[4] == '1')
			DTG_LOGI("Result Success> packet_id = [%d], resp code[%c]", packet_id, recv_buf[4]);
		else
			DTG_LOGE("Result Fail> packet_id = [%d], resp code[%c]", packet_id, recv_buf[4]);
	}
*/
	close(sock_fd);

	return 0;	
}

//#define MDT_NOT_SEND_TEST_ENABLE
#ifdef MDT_NOT_SEND_TEST_ENABLE
	int g_mdt_send_flag = 0;
#endif

#define ERROR_RETURN_VALUE		(-2222)
#define MDT_ONCE_SEND_PACKET_CNT	(40)
static int g_dtg_key_flag = 0;
static int g_mdt_key_flag = 0;
static int g_dtg_power_flag = 0;
static int g_mdt_power_flag = 0;
unsigned char g_packet_buffer[512];
unsigned char g_mdt_packet_buffer[sizeof(mdt800_packet_t)*MDT_ONCE_SEND_PACKET_CNT];
unsigned char g_mdt_enc_packet_buffer[sizeof(mdt800_packet_t)*2*MDT_ONCE_SEND_PACKET_CNT];

void clear_key_flag()
{
	g_dtg_key_flag = 0;
	g_mdt_key_flag = 0;
}

void clear_power_flag()
{
	g_dtg_power_flag = 0;
	g_mdt_power_flag = 0;
}

int tx_data_to_tacoc(int type, char *stream, int len)
{
	int cnt;
	int pack_buffer_size = 0 ;
	int send_pack_size = 0;
	int key_status;
	unsigned char *dtg_pack_buf;
	
	DTG_LOGD("Tacoc Data Reception Success, Type[%d], length[%d]", type, len);
	printf("%s:%d> Tacoc Data Reception Success, Type[%d], length[%d]\n", __func__, __LINE__, type, len);
	if(len <= 0) 
	{
		return 0;
	}

	if (type == 1) //DTG DATA
	{
		cnt = (len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t) + 1;
		pack_buffer_size = sizeof(gtrace_packet_body_t) + sizeof(gtrace_dtg_user_data_hdr_t) + sizeof(gtrace_dtg_user_data_summary_t) + (sizeof(gtrace_dtg_user_data_payload_t) * (cnt+5)) + 2;
		//DTG_LOGD("pack_buffer malloc size = [%d]\n", pack_buffer_size);
		dtg_pack_buf = (unsigned char *)malloc(pack_buffer_size);
		if(dtg_pack_buf == NULL) {
			DTG_LOGE("dtg_buf mallock Error");
			return -2222;
		}

		send_pack_size = bulk_dtg_parsing(stream, len, dtg_pack_buf);

		if(send_record_data_msg(dtg_pack_buf, send_pack_size, __LINE__, "Sending DTG DATA") < 0) {
			free(dtg_pack_buf);
			return -2222; //network error
		}

		free(dtg_pack_buf);

	} 
	else if (type == 2) //Key On/Off Data
	{
		int event_code;
		int mdt_event_code;
		key_status = power_get_ignition_status();
		if(key_status == POWER_IGNITION_ON) {
			event_code = eKeyOn_Event;
			mdt_event_code = eIGN_ON_EVT;
		}
		else {
			event_code = eKeyOff_Event;
			mdt_event_code = eIGN_OFF_EVT;
		}

		
		if(g_mdt_key_flag == 0)
		{
			parse_mdt_msg(stream, g_mdt_packet_buffer, mdt_event_code);
			send_pack_size = mdt_enc_msg(g_mdt_packet_buffer, g_mdt_enc_packet_buffer, sizeof(mdt800_packet_t));
			g_mdt_enc_packet_buffer[send_pack_size] = MDT800_PACKET_END_FLAG;
			if(mdt_send_record_data_msg(g_mdt_enc_packet_buffer, send_pack_size+1, __LINE__, "MDT Key on/off Data") < 0) {
				return -2222; //network error
			}
			g_mdt_key_flag = 1;
		}
		

		if(g_dtg_key_flag == 0) 
		{
			send_pack_size = current_dtg_parsing(stream, len, g_packet_buffer, event_code);
			DTG_LOGD("trasfer packet size = [%d]\n", send_pack_size);
			if(send_record_data_msg(g_packet_buffer, send_pack_size, __LINE__, "DTG Key on/off Data") < 0) {
				return -2222; //network error
			}
		}
		g_mdt_key_flag = 0;
		g_dtg_key_flag = 0;
	}
	else if (type == 3) //power on/off message
	{
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
	}
	else if (type == 4) //Just Get Current DTG Data
	{
		set_current_dtg_data(stream, len);
	}
	else if (type == 5) //MDT Report Data
	{
#ifdef MDT_NOT_SEND_TEST_ENABLE
		DTG_LOGE("current MDT packet count = [%d]", get_mdt_count());
#endif
		
		if(get_mdt_count() <= 0) {
			parse_mdt_msg(stream, g_mdt_packet_buffer, eCYCLE_REPORT_EVC);
			send_pack_size = mdt_enc_msg(g_mdt_packet_buffer, g_mdt_enc_packet_buffer, sizeof(mdt800_packet_t));
			g_mdt_enc_packet_buffer[send_pack_size] = MDT800_PACKET_END_FLAG;

#ifdef MDT_NOT_SEND_TEST_ENABLE
			if(g_mdt_send_flag == 0)
			{
				save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
				return -2222; //network error
			}
#endif
			if(mdt_send_record_data_msg(g_mdt_enc_packet_buffer, send_pack_size+1, __LINE__, "MDT Period Report #1") < 0) {
				save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
				return -2222; //network error
			}
		}
		else
		{
			int mdt_get_cnt = 0;

			mdt_get_cnt = get_mdt_data(g_mdt_packet_buffer, MDT_ONCE_SEND_PACKET_CNT);
			send_pack_size = mdt_enc_msg(g_mdt_packet_buffer, g_mdt_enc_packet_buffer, sizeof(mdt800_packet_t)*mdt_get_cnt);
			DTG_LOGI("total saved data info mdt_get_cnt/send_pack_size = [%d/%d]", mdt_get_cnt, send_pack_size);
			g_mdt_enc_packet_buffer[send_pack_size] = MDT800_PACKET_END_FLAG;
#ifdef MDT_NOT_SEND_TEST_ENABLE
			if(g_mdt_send_flag == 0)
			{
				parse_mdt_msg(stream, g_mdt_packet_buffer, eCYCLE_REPORT_EVC);
				save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
				//current mdt packet save --
				return -2222; //network error
			}
#endif
			if(mdt_send_record_data_msg(g_mdt_enc_packet_buffer, send_pack_size+1, __LINE__, "MDT Period Report #2") < 0) {
				//current mdt packet save ++
				parse_mdt_msg(stream, g_mdt_packet_buffer, eCYCLE_REPORT_EVC);
				save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
				//current mdt packet save --
				return -2222; //network error
			}
			clear_mdt_data(mdt_get_cnt);
			parse_mdt_msg(stream, g_mdt_packet_buffer, eCYCLE_REPORT_EVC);

			if(get_mdt_count() <= 0) {
				send_pack_size = mdt_enc_msg(g_mdt_packet_buffer, g_mdt_enc_packet_buffer, sizeof(mdt800_packet_t));
				g_mdt_enc_packet_buffer[send_pack_size] = MDT800_PACKET_END_FLAG;
				if(mdt_send_record_data_msg(g_mdt_enc_packet_buffer, send_pack_size+1, __LINE__, "MDT Period Report #3") < 0) {
					save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
					return -2222; //network error
				}
			}
			else {
				save_mdt_data((mdt800_packet_t *)g_mdt_packet_buffer);
			}
		}
	}
		
	return 0;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
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

	return 0;
}

//#define MAX_LAST_SENDING_SIZE	10*1024
#define MAX_LAST_SENDING_DATA_COUNT	600 //10 min
void mdmc_power_off()
{
	int ret;
	int ready_cnt = 0;
	DTG_LOGI("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
	DTG_LOGI("mdmc_power_off---------------------->1");
	DTG_LOGI("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");


	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
		DTG_LOGE("tacoc : mdmc_power_off wait [%d][%d]\n", is_working_action(), ready_cnt);
	}

	DTG_LOGE("tacoc : mdmc_power_off #1\n");
	data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off
	data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //power off

	ret = tacom_unreaded_records_num();
	if(ret < MAX_LAST_SENDING_DATA_COUNT) {
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, MAX_LAST_SENDING_DATA_COUNT, 0, 2);
		if(ret == 0) //success
		{
			ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
		}
	}

	ret = tacom_unreaded_records_num();
	if(ret > 10) //remain count
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save

	send_device_de_registration();
	// dmmgr_send(eEVENT_PWR_OFF, NULL, 0);	
}


void ignition_off_send_server_data()
{
	int ready_cnt = 0;
	//int key_status;
	int err_cnt = 0;
	int ret;

	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
	}

	DTG_LOGE("tacoc : key_off #1\n");
	data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off

	while(1) {
		if(power_get_ignition_status() == POWER_IGNITION_ON)
			break;

		if(err_cnt > 5)
			break;

		ret = tacom_unreaded_records_num();
		if(ret < 10)
			break;

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

	ret = tacom_unreaded_records_num();
	if(ret > 10) //remain count
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save
}

void tacoc_ignition_off()
{
	ignition_off_send_server_data();

	send_device_de_registration();
}

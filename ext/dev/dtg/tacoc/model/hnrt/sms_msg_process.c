<<<<<<< HEAD
/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: gbuddha
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>


#include <dtg_net_com.h>
#include <wrapper/dtg_log.h>

#include <util/crc16.h>

#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"
#include "rpc_clnt_operation.h"


// warnning fix
void device_reset();
int save_ini_dtg_period_info();
int save_ini_server_setting_info();
int save_ini_mdt_period_info();

extern mdt_pck_t end_mdt_buf; 
extern resp_pck_t msg_resp[];

int response_mdt(int event_code)
{
	DTG_LOGD("+++++sms: %s", __func__);
	int ret = 0;
	int try_cnt = 0;
	unsigned short crc16_value;
	int sock_fd;

	mdt_pck_t *tmp_buf = malloc(sizeof(mdt_pck_t));
	memcpy(tmp_buf, &end_mdt_buf, sizeof(mdt_pck_t));
	tmp_buf->header.msg_len_mark[2] |= 0x1;
	tmp_buf->body.event_code = event_code;
	tmp_buf->body.report_period = htons(get_mdt_report_period());
	tmp_buf->body.create_period = htons(get_mdt_create_period());

	crc16_value = crc16_get(&tmp_buf->body, sizeof(msg_mdt_t) - 3);
	tmp_buf->body.crc_val = htons(crc16_value);
	tmp_buf->body.endflag = 0x7e;
	try_cnt = 0;
	do {
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		if (ret < 0) {
			return -1;
		}

		ret = send_to_dtg_server(sock_fd, tmp_buf, sizeof(mdt_pck_t), __func__, __LINE__, "response data");
		if (ret < 0) {
			DTG_LOGE("MDT message error");
			disconnect_to_server(sock_fd);
			sock_fd = -1;
			return -1;
		}
		ret = receive_response(sock_fd, 0);
		if (ret < 0){
			DTG_LOGE("Error recive response.");
			disconnect_to_server(sock_fd);
			sock_fd = -1;
			return ret;
		} 
		disconnect_to_server(sock_fd);
		sleep(3);
		try_cnt++;
	} while(msg_resp[0].result == 102 && try_cnt < 5);
	free(tmp_buf);
	return ret;
}

int sms_set_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char tmp_str[50] = {0,};
	int port = -1;
	int resp_flag = 0;
	char *psms;
	//char mdn[20];

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	if(strcmp(psms,"#ipSet#@"))
		return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	port = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%s %d\n", ip, port);

	set_server_ip_addr(ip);
	set_server_port(port);

	save_ini_server_setting_info();

	if(resp_flag == 1)
		response_mdt(1);

	return 0;
}

int sms_set_mdt_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char tmp_str[50];
	int report_period = -1;
	int create_period = -1;
	int power_save_period_set = 0;
	int resp_flag = 0;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	if(!strcmp(psms,"&11"))
		power_save_period_set = 1;

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_period = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	create_period = strtol(tmp_str, NULL, 10);

	if (power_save_period_set == 1){
		psms=strtok(sms_msg,token_1);
		if(psms==0) return -1;
	}

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%d %d\n", report_period, create_period);

	if(report_period < create_period || report_period%create_period!= 0) {
		return -1;
	} else {
		set_mdt_report_period(report_period);
		set_mdt_create_period(create_period);
	}

	save_ini_mdt_period_info();

	if(resp_flag == 1){
		if(power_save_period_set == 1)
			response_mdt(32);
		else
			response_mdt(2);
	}
	return 0;
}

int sms_set_dtg_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char tmp_str[50] = {0,};
	int report_period = -1;
	int resp_flag = 0;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_period = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%d\n", report_period);

	if(report_period < 60) {
		return -1;
	} else {
		set_dtg_report_period(report_period);
	}

	save_ini_dtg_period_info();
	if(resp_flag == 1)
		response_mdt(201);
	alarm(get_dtg_report_period());
	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	DTG_LOGD("sms_set_device_reset\n");

	char token_1[]=",";
	//char token_2[]="\0";

	char pwd[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);

	if(strcmp(pwd, "m2m%eksakffltpt*@"))
		return -1;

	device_reset();
	return 0;
}

int sms_device_status_req(char* sms_msg)
{
	DTG_LOGD("sms_device_status_req");

	char token_1[]=",";
	char token_2[]="\0";

	//int c_count = 0;
	char resp[50] = {0,};
	//char dest_info[100] = {0,};
	//char contents[100] = {0,};
	char *psms;
	//char *phonenum;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(strcmp(resp, "0"))
	{
		DTG_LOGD("resp: %s", resp);
		DTG_LOGD("SEND RESPONSE (TCP)");
		response_mdt(4);
		return 0;
	}
		
	return 0;
}
=======
/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: gbuddha
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>


#include <dtg_net_com.h>
#include <wrapper/dtg_log.h>

#include <util/crc16.h>

#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"
#include "rpc_clnt_operation.h"


// warnning fix
void device_reset();
int save_ini_dtg_period_info();
int save_ini_server_setting_info();
int save_ini_mdt_period_info();

extern mdt_pck_t end_mdt_buf; 
extern resp_pck_t msg_resp[];

int response_mdt(int event_code)
{
	DTG_LOGD("+++++sms: %s", __func__);
	int ret = 0;
	int try_cnt = 0;
	unsigned short crc16_value;
	int sock_fd;

	mdt_pck_t *tmp_buf = malloc(sizeof(mdt_pck_t));
	memcpy(tmp_buf, &end_mdt_buf, sizeof(mdt_pck_t));
	tmp_buf->header.msg_len_mark[2] |= 0x1;
	tmp_buf->body.event_code = event_code;
	tmp_buf->body.report_period = htons(get_mdt_report_period());
	tmp_buf->body.create_period = htons(get_mdt_create_period());

	crc16_value = crc16_get(&tmp_buf->body, sizeof(msg_mdt_t) - 3);
	tmp_buf->body.crc_val = htons(crc16_value);
	tmp_buf->body.endflag = 0x7e;
	try_cnt = 0;
	do {
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		if (ret < 0) {
			return -1;
		}

		ret = send_to_dtg_server(sock_fd, tmp_buf, sizeof(mdt_pck_t), __func__, __LINE__, "response data");
		if (ret < 0) {
			DTG_LOGE("MDT message error");
			disconnect_to_server(sock_fd);
			sock_fd = -1;
			return -1;
		}
		ret = receive_response(sock_fd, 0);
		if (ret < 0){
			DTG_LOGE("Error recive response.");
			disconnect_to_server(sock_fd);
			sock_fd = -1;
			return ret;
		} 
		disconnect_to_server(sock_fd);
		sleep(3);
		try_cnt++;
	} while(msg_resp[0].result == 102 && try_cnt < 5);
	free(tmp_buf);
	return ret;
}

int sms_set_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char tmp_str[50] = {0,};
	int port = -1;
	int resp_flag = 0;
	char *psms;
	//char mdn[20];

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	if(strcmp(psms,"#ipSet#@"))
		return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	port = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%s %d\n", ip, port);

	set_server_ip_addr(ip);
	set_server_port(port);

	save_ini_server_setting_info();

	if(resp_flag == 1)
		response_mdt(1);

	return 0;
}

int sms_set_mdt_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char tmp_str[50];
	int report_period = -1;
	int create_period = -1;
	int power_save_period_set = 0;
	int resp_flag = 0;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	if(!strcmp(psms,"&11"))
		power_save_period_set = 1;

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_period = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	create_period = strtol(tmp_str, NULL, 10);

	if (power_save_period_set == 1){
		psms=strtok(sms_msg,token_1);
		if(psms==0) return -1;
	}

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%d %d\n", report_period, create_period);

	if(report_period < create_period || report_period%create_period!= 0) {
		return -1;
	} else {
		set_mdt_report_period(report_period);
		set_mdt_create_period(create_period);
	}

	save_ini_mdt_period_info();

	if(resp_flag == 1){
		if(power_save_period_set == 1)
			response_mdt(32);
		else
			response_mdt(2);
	}
	return 0;
}

int sms_set_dtg_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char tmp_str[50] = {0,};
	int report_period = -1;
	int resp_flag = 0;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_period = strtol(tmp_str, NULL, 10);

	memset(tmp_str, 0, 50);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(tmp_str, psms);
	resp_flag = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%d\n", report_period);

	if(report_period < 60) {
		return -1;
	} else {
		set_dtg_report_period(report_period);
	}

	save_ini_dtg_period_info();
	if(resp_flag == 1)
		response_mdt(201);
	alarm(get_dtg_report_period());
	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	DTG_LOGD("sms_set_device_reset\n");

	char token_1[]=",";
	//char token_2[]="\0";

	char pwd[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);

	if(strcmp(pwd, "m2m%eksakffltpt*@"))
		return -1;

	device_reset();
	return 0;
}

int sms_device_status_req(char* sms_msg)
{
	DTG_LOGD("sms_device_status_req");

	char token_1[]=",";
	char token_2[]="\0";

	//int c_count = 0;
	char resp[50] = {0,};
	//char dest_info[100] = {0,};
	//char contents[100] = {0,};
	char *psms;
	//char *phonenum;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(strcmp(resp, "0"))
	{
		DTG_LOGD("resp: %s", resp);
		DTG_LOGD("SEND RESPONSE (TCP)");
		response_mdt(4);
		return 0;
	}
		
	return 0;
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

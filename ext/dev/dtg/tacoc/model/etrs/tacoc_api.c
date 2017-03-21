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
#include "dtg_net_com.h"
#include "sms_msg_process.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include "parsing.h"
#include <base/dmmgr.h>

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_send.h>
#endif

//etrace_ternminal_info_t g_dtg_info;

#define MAX_END_DTG_SIZE	8192
int breakdown_report(int integer)
{
	return 0;
}


int send_record_data_msg(unsigned char* stream, int stream_size, int line, char *msg)
{
	int i;
	int sock_fd;
	int ret;
	char config_code;
	unsigned char recv_buf[512];

#if (0) //for debug send data dump ++
	int j;
	j = 1;
	for(i = 0, j = 1; i < stream_size; i++, j++) {
		printf("%02x ", stream[i]);
		if( (j % 20) == 0)
			printf("\n");
	}
#endif //for debug send data dump --

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
	{
		return -1;
	}
	
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
		DTG_LOGI("respnse code = [%c]", recv_buf[0]);
		if(recv_buf[0] == 'E')
		{
			DTG_LOGI("server error = [%c]", recv_buf[0]);
			close(sock_fd);
			return -4;
		}

		if(ret > 1) {
			config_code = recv_buf[1];
			DTG_LOGI("configuration code = [%c]", config_code);
			switch(config_code) {
				case 'D':		//DTG ID
					break;
				case 'A':		//IP Address And Port
					break;
				case 'S':		//Server Address(DNS) And Port
					break;
				case 'F':		//Firmware upgrade over the Air
					break;
				case 'L':		//accumulative distance
					break;
				case 'V':		//vehicle identification number
					break;
				case 'N':		//none
				default:
					break;
			}
		}
	}

	close(sock_fd);
	return 0;	
}

#define ERROR_RETURN_VALUE		(-2222)
unsigned char g_devce_status_info[512] = {0, };
unsigned char g_devce_voltage_info[512] = {0, };
int g_buffer_temp_len = 0;
unsigned char g_buffer_temp[512] = {0, };

int key_off_volt_report()
{
	int send_pack_size;
	send_pack_size = term_info_parsing(g_buffer_temp, g_buffer_temp_len, g_devce_voltage_info, eVOLT);
	if(send_record_data_msg((unsigned char *)g_devce_voltage_info, send_pack_size, __LINE__, "Voltage Data") < 0) {
		return -1; //network error
	}
	return 0;
}

int dtg_error_report()
{
	int send_pack_size;
	eTrace_vehicle_status_t vs;

	#ifdef ENABLE_VOLTAGE_USED_SCINARIO
		vs = eVOLT_DTG_NOTWORKING;
	#else
		vs = eDTG_NOT_WORKING;
	#endif

	send_pack_size = term_info_parsing(NULL, 0, g_devce_status_info, vs);	
	if(send_record_data_msg((unsigned char *)g_devce_status_info, send_pack_size, __LINE__, "Dtg Error Report Data") < 0) {
		return -2222; //network error
	}

	return 0;
}


int tx_data_to_tacoc(int type, char *stream, int len)
{
	int cnt;
	int pack_buffer_size = 0 ;
	int send_pack_size = 0;
	int key_status;
	unsigned char *dtg_pack_buf;
	eTrace_vehicle_status_t vs;
	DTG_LOGD("Tacoc Data Reception Success, Type[%d], length[%d]", type, len);
	printf("%s:%d> Tacoc Data Reception Success, Type[%d], length[%d]\n", __func__, __LINE__, type, len);
	//DTG_LOGD("sizeof(etrace_packet_hdr_t) = [%d]\n", sizeof(etrace_packet_hdr_t));
	//DTG_LOGD("sizeof(etrace_dtg_hdr_t) = [%d]\n", sizeof(etrace_dtg_hdr_t));
	//DTG_LOGD("sizeof(etrace_dtg_body_t) = [%d]\n", sizeof(etrace_dtg_body_t));
	//DTG_LOGD("sizeof(etrace_ternminal_info_t) = [%d]\n", sizeof(etrace_ternminal_info_t));
	if(len <= 0) 
	{
		return 0;
	}

	if (type == 1) //DTG DATA
	{
		//2 is crc16
		cnt = (len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t) + 1;
		pack_buffer_size = sizeof(etrace_packet_hdr_t) + sizeof(etrace_dtg_hdr_t) + (sizeof(etrace_dtg_body_t) * (cnt+5)) + 2;
		//DTG_LOGD("pack_buffer malloc size = [%d]\n", pack_buffer_size);
		dtg_pack_buf = (unsigned char *)malloc(pack_buffer_size);
		if(dtg_pack_buf == NULL) {
			DTG_LOGE("dtg_buf mallock Error");
			return -2222;
		}

		send_pack_size = etrs_dtg_parsing(stream, len, dtg_pack_buf, 0x44); //0x44 is DTG DATA ID
		//DTG_LOGD("trasfer packet size = [%d]\n", send_pack_size);

		if(send_record_data_msg(dtg_pack_buf, send_pack_size, __LINE__, "Sending DTG DATA") < 0) {
			free(dtg_pack_buf);
			return -2222; //network error
		}

		free(dtg_pack_buf);

	} 
	else if (type == 2) //Key On/Off Data
	{
		key_status = power_get_ignition_status();
#ifdef ENABLE_VOLTAGE_USED_SCINARIO
		if(key_status == POWER_IGNITION_ON)
			vs = eVOLT_KEY_ON;
		else
			vs = eVOLT_KEY_OFF;
#else
		if(key_status == POWER_IGNITION_ON)
			vs = eKEY_ON;
		else
			vs = eKEY_OFF;
#endif
		send_pack_size = term_info_parsing(stream, len, g_devce_status_info, vs);
		

		if(send_record_data_msg((unsigned char *)g_devce_status_info, send_pack_size, __LINE__, "Key On/Off Data") < 0) {
			return -2222; //network error
		}
	}
	else if (type == 3) //one data send
	{
		unsigned char packet_buffer[512];
		send_pack_size = etrs_dtg_parsing(stream, len, packet_buffer, 0x47); //0x47 is Simple One DTG Data ID
		//DTG_LOGD("trasfer packet size = [%d]\n", send_pack_size);
		send_record_data_msg(packet_buffer, send_pack_size, __LINE__, "Sending ONE DTG DATA");
	}
	else if (type == 4) //voltage data send
	{
		g_buffer_temp_len = len;
		memcpy(g_buffer_temp, stream, len);
		send_pack_size = term_info_parsing(stream, len, g_devce_voltage_info, eVOLT);
		if(send_record_data_msg((unsigned char *)g_devce_voltage_info, send_pack_size, __LINE__, "Voltage Data") < 0) {
			return -2222; //network error
		}
	}
		
	return 0;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
#if defined(BOARD_TL500K)
	//KT DEVICE FOTA SERVICE
	if(sender == NULL)
	{
		if(!strcmp("KT_DEV_FOTA_SVC_FOTA_REQ_PUSH", smsdata))//
		{
			send_fota_req_packet(ePUSH_MODE, eHTTP, 30);
			return 0;
		}
		else if(!strcmp("KT_DEV_FOTA_SVC_DEVICE_RESET", smsdata))
		{
			////////////////////////////////////////////////////
			//Device Reset
			////////////////////////////////////////////////////
			//void device_reset(); //clear data
			while(1) {
				system("poweroff");
				DTG_LOGI("%s> kt fota reset wait powerorff...\n", __func__);
				sleep(1);		
			}
			return 0;
		}
		else if(!strcmp("KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET", smsdata))
		{
			////////////////////////////////////////////////////
			//modem in-direct Reset
			////////////////////////////////////////////////////
			//void device_reset(); //clear data
			while(1) {
				system("poweroff");
				DTG_LOGI("%s> kt fota reset wait powerorff...\n", __func__);
				sleep(1);		
			}
			return 0;
		}
		else if(!strcmp("KT_DEV_FOTA_SVC_DEVICE_QTY", smsdata))
		{
			send_qty_packet(ePUSH_MODE, eDEVICE_MODEM_QTY_MODE, 30);
			return 0;
		}
		else if(!strcmp("KT_DEV_FOTA_SVC_MODEM_QTY", smsdata))
		{
			send_qty_packet(ePUSH_MODE, eMODEM_QTY_MODE, 30);
			return 0;
		}
		return -1;
	}
#endif

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

#if defined(BOARD_TL500K)
	#if ! (defined(DEVICE_MODEL_LOOP) || defined(DEVICE_MODEL_LOOP2) || defined(DEVICE_MODEL_CHOYOUNG) )
		send_device_off_packet(30);
	#endif
#endif


	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
		DTG_LOGE("tacoc : mdmc_power_off wait [%d][%d]\n", is_working_action(), ready_cnt);
	}

	DTG_LOGE("tacoc : mdmc_power_off #1\n");
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

	data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //key off
	send_device_de_registration();
	// dmmgr_send(eEVENT_PWR_OFF, NULL, 0);
	
}


void ignition_off_send_server_data()
{
	int ready_cnt = 0;
	int key_status;
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

#if defined(BOARD_TL500K)
	#if ! (defined(DEVICE_MODEL_LOOP) || defined(DEVICE_MODEL_LOOP2) || defined(DEVICE_MODEL_CHOYOUNG) )
		send_device_off_packet(30);
	#endif
#endif

	send_device_de_registration();
}

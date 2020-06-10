/*
 * tacoc_api.c
 *
 *  Created on: 2013. 3. 8.
 *      Author: ongten
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

#include <board/board_system.h>
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "sms_msg_process.h"
#include "standard_protocol.h"
#include "rpc_clnt_operation.h"

//#ifndef RECORD_HDR_LEN
	#define RECORD_HDR_LEN	79
//#endif

//#ifndef RECORD_DATA_LEN
	#define RECORD_DATA_LEN	103
//#endif

//#ifndef RECORD_DATA_LEN
	#define MAX_RECORD_NUM 60
//#endif

//#ifndef MAX_RECORD_DATA_LEN
	#define MAX_RECORD_DATA_LEN (MAX_RECORD_NUM*RECORD_DATA_LEN)
//#endif

extern int last_event;
int mode_change_to_ut_default_mode()
{
	DTG_LOGD("+++++mode_change_to_ut_default_mode");

	set_serial_port_mode(UT_DEFAULT_MODE);
	set_normal_trans_period(600);
	set_normal_create_period(600);
	save_ini_report1_info();
	
	set_tripo_mode(0);
	save_ini_tripo_mode();
	
	set_current_ctime();
	return 0;
}

int mode_change_to_ut1_2_mode()
{
	DTG_LOGD("+++++mode_change_to_ut1_2_mode");

	set_serial_port_mode(UT1_2_MODE);
	set_normal_trans_period(60);
	set_normal_create_period(60);
	save_ini_report1_info();
	
	set_tripo_mode(1);
	save_ini_tripo_mode();
	
	set_current_ctime();
	return 0;
}

int breakdown_report(int integer)
{
	int serial_mode = get_serial_port_mode(); //UT_DEFAULT_MODE

	if(integer == 0)
	{
		DTG_LOGW("breakdw msg: dtg problem occur");

		if(serial_mode != UT_DEFAULT_MODE)
		{
			send_saved_report_msg();
			mode_change_to_ut_default_mode();
		}
	}
	else if(integer == 1)
	{	
		DTG_LOGI("breakdw msg: dtg working");
		if(serial_mode == UT_DEFAULT_MODE)
		{
			send_saved_report_msg();
			mode_change_to_ut1_2_mode();
		}
	}
	else
		DTG_LOGE("breakdw msg: incorrect code");
	
	return 0;
}

int send_zero_data_msg(void)
{
	int i, e_cnt;
	int data_size = 0;
	int idx = 0;
	int ret = 0;
	unsigned short tmp_short;
	char *phonenum = NULL;

	unsigned char *tmp_msg = (unsigned char *)malloc(35);
	unsigned char *enc_msg = (unsigned char *)malloc(70);


	tmp_msg[idx++] = 0x11;
	tmp_msg[idx++] = 0x99;

	memset(&tmp_msg[idx],0x20, 15);
	phonenum  = atcmd_get_phonenum();
	memcpy(&tmp_msg[idx], phonenum, strlen(phonenum));
	idx +=15;

	set_current_ctime();
	tmp_short = get_ctime_year();   // set ctime
	memcpy(&tmp_msg[idx], &tmp_short, 2);
	idx +=2;
	tmp_msg[idx++] = get_ctime_mon();
	tmp_msg[idx++] = get_ctime_day();
	tmp_msg[idx++] = get_ctime_hour();
	tmp_msg[idx++] = get_ctime_min();
	tmp_msg[idx++] = get_ctime_sec();

	memset(&tmp_msg[idx], 0x20, 10);
	memcpy(&tmp_msg[idx], get_company_code(), 5); // company code
	idx += 10;

	for(i=0; i<idx; i++)
	{
		if(tmp_msg[i] == 0x7e){
			enc_msg[e_cnt++] = 0x7d;
			enc_msg[e_cnt++] = 0x5e;
			continue;
		} else {
			enc_msg[e_cnt++] = tmp_msg[i];
		}
	}
	enc_msg[e_cnt++] = 0x7e;

	ret = g_send_to_server(enc_msg, e_cnt);

	free(tmp_msg);
	free(enc_msg);

	if(ret < 0)
	{
		DTG_LOGD("++++++Data Sending Error");
		return -1;
	}
	return 0;
}

int send_record_data_msg(char* tmp_hdr, char* stream, int data_num)
{
	int tmp_msg_len;
	int encode_msg_len;
	int tmp_data_len;
	int data_size = data_num*RECORD_DATA_LEN;
	int ret;

	unsigned char *tmp_data = (unsigned char *)malloc(RECORD_HDR_LEN+MAX_RECORD_DATA_LEN);
	unsigned char *tmp_msg = (unsigned char *)malloc(RECORD_HDR_LEN+MAX_RECORD_DATA_LEN+100);
	unsigned char *encode_msg = (unsigned char *)malloc(RECORD_HDR_LEN+(MAX_RECORD_DATA_LEN*2)+100);

	tmp_data_len = 0;
	memcpy(tmp_data+tmp_data_len, tmp_hdr, RECORD_HDR_LEN);
	tmp_data_len +=  RECORD_HDR_LEN;

	memcpy(tmp_data+tmp_data_len, stream, data_size);
	stream += data_size;
	tmp_data_len +=  data_size;
	
	memset (tmp_msg, 0, RECORD_HDR_LEN+MAX_RECORD_DATA_LEN+100);

	tmp_msg_len = build_report_msg(tmp_msg, data_num, tmp_data, 5);

	encode_msg_len = encode_report_msg(encode_msg, tmp_msg, tmp_msg_len);
	DTG_LOGD("++++++ TRIPOS STEP_103_2(%d)", encode_msg_len);
	ret = g_send_to_server(encode_msg, encode_msg_len);

	free(tmp_data);
	free(tmp_msg);
	free(encode_msg);
	
	DTG_LOGD("++++++ TRIPOS STEP_103_3");

	if(ret < 0)
	{
		DTG_LOGD("++++++Data Sending Error");
		return -1;
	}
	return 0;
}

#define MIN(a,b) (((a)<(b))?(a):(b))

msg_mdt_t mdt_buf;
int tx_data_to_tacoc(int type, char *stream, int len)
{
	int i;
	int ret;
	unsigned char tmp_hdr[RECORD_HDR_LEN] = {0,};
	int num_group = (len-RECORD_HDR_LEN) / MAX_RECORD_DATA_LEN;
	int rest_data_count = ((len-RECORD_HDR_LEN)/RECORD_DATA_LEN) % MAX_RECORD_NUM;
	char *stream_ptr = stream;

	char *gps_x;
	char *gps_y;
	char temp_gps[9];
	int gps_idx;


	DTG_LOGD("Tacoc Data Reception Success, Type[%d]", type);

	if (type == 0) {		/* summary */
		DTG_LOGD("Summary!!");
		send_to_summary_server(stream, len);
		return 0;

	} else if (type == 1) {
		DTG_LOGD("Tripos!! len: %d, ng: %d, rdc: %d", len, num_group, rest_data_count);

		if(len < RECORD_HDR_LEN)
		{
			DTG_LOGE("Message Size Error");
			return -1;
		}
		
		memcpy(tmp_hdr, stream, RECORD_HDR_LEN);
		stream_ptr += RECORD_HDR_LEN;

		// Swap gps x/y for triphos server.
		gps_idx = RECORD_HDR_LEN + 33;
		while (gps_idx < len){
			gps_x = stream + gps_idx;
			gps_y = stream + gps_idx + 9;
			memcpy(temp_gps, gps_x, 9);
			memcpy(gps_x, gps_y, 9);
			memcpy(gps_y, temp_gps, 9);
			gps_idx += sizeof(tacom_std_data_t);
		}

		DTG_LOGD("++++++ TRIPOS STEP_103(%d)", num_group);

		for(i=0; i<=num_group; i++) {
			DTG_LOGD("++++++ TRIPOS STEP_103_1(%d)", i);
			if (i == num_group){
				if(rest_data_count > 0) {
					if (i % 5 == 0) {
						if(i != 0) {
							DTG_LOGI("Delay Connecting [%d] sec", 2 + MIN(3, (i/5)));
							sleep(2 + MIN(3, (i/5)));
						}
						g_connect_to_server();
					}
					ret = send_record_data_msg(tmp_hdr, stream_ptr, rest_data_count);
				}
			} else {
				if (i % 5 == 0) {
					if(i != 0) {
						DTG_LOGI("Delay Connecting [%d] sec", 2 + MIN(3, (i/5)));
						sleep(2 + MIN(3, (i/5)));
					}
					g_connect_to_server();
				}
				ret = send_record_data_msg(tmp_hdr, stream_ptr, MAX_RECORD_NUM);
			}
			if(ret < 0) {
				DTG_LOGE("Message Trans Error - send_record_data_msg");
				g_disconnect_to_server();
				return -1;
			}
			if(i % 5 == 4) {
				g_disconnect_to_server();
				usleep(500*1000);
			}
		}
		DTG_LOGD("++++++ TRIPOS STEP_104(%d)", rest_data_count);
		g_disconnect_to_server();
		DTG_LOGD("++++++ TRIPOS STEP_105");
	}
	else
	{
		if(len > 0) {
			DTG_LOGD("store mdt message");
			mdt_parsing(&mdt_buf, stream+RECORD_HDR_LEN);
//			memcpy(&mdt_buf, stream, sizeof(msg_mdt_t));
		} else {
			DTG_LOGE("Message Size Error : RC");
			return -1;
		}
	}
	
	return 0;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
	int ret = 0;
	DTG_LOGD("+++++sms(2): %s", smsdata);

	ret = common_sms_parser(smsdata, sender);
	if (ret == 0)
		return ret;

	if(strlen(smsdata) < 3 || smsdata[0] != '&')
		return -1;

	switch(smsdata[1])
	{
		case '1':
			if(smsdata[2] == ',')
				ret = sms_set_svrip_info(smsdata);
			else if(smsdata[2] == '0')
				ret = sms_set_device_reset(smsdata);
			else if(smsdata[2] == '1')
				ret = sms_set_report_period2(smsdata);
			else if(smsdata[2] == '2')
				ret = sms_set_geo_fence(smsdata);
			break;
		case '2':
			ret = sms_set_report_period1(smsdata);
			break;
		case '3':
			ret = sms_set_cumulative_distance(smsdata);
			break;
		case '4':
			ret = sms_request_device_status(smsdata);
			break;
		case '5':
			ret = sms_set_gpio_mode(smsdata);
			break;
		case '6':
			ret = sms_set_gpio_output(smsdata);
			break;
		case 'c':
			if(smsdata[2] == 'c')
				ret = sms_set_company_code(smsdata);
			break;
		case 'm':
			if(smsdata[2] == 's')
				ret = sms_device_status_req(smsdata);
			break;
		default:
			break;
	}

	return ret;
}

#define MAX_SEDING_BUF_SIZE		100*1024
#define MAX_LAST_SENDING_SIZE	10*1024

void mdmc_power_off()
{
	int tmp_msg_len;
	int encode_msg_len;
	int fd, ret, result;
	int type = 0;
	int len = 0;
	int retry = 0;
	char *stream;

fprintf(stderr, "tacoc : mdmc_power_off #1\n");
	ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2);
fprintf(stderr, "tacoc : mdmc_power_off #2\n");
	if (ret < 0) {
		fprintf(stderr, "tacoc : mdmc_power_off last_event_msg_sending #1\n");
		goto last_event_msg_sending;
	}
	else {
		stream = malloc(MAX_SEDING_BUF_SIZE);

		if(stream == NULL) {
			fprintf(stderr, "tacoc : mdmc_power_off last_event_msg_sending #2\n");
			goto last_event_msg_sending;
		}

		memset(stream, 0, MAX_SEDING_BUF_SIZE);
	
		while(retry < 5) {

			do {
				fd = open("/var/stored_records", O_RDONLY, 0644 );
				sleep(1);
			} while (fd < 0 && errno != ENOENT);

			if(fd <= 0) {
				fprintf(stderr, "tacoc : mdmc_power_off last_event_msg_sending #3\n");
				goto last_event_msg_sending;
			}

			read(fd, &len, sizeof(int));
			if(len > MAX_LAST_SENDING_SIZE) {
				fprintf(stderr, "tacoc : mdmc_power_off last_event_msg_sending #4 > len[%d]KB\n", (len/1024));
				goto last_event_msg_sending;
			}

			ret = read(fd, stream, MAX_SEDING_BUF_SIZE);
			if (ret != len) {
				retry++;
				close(fd);
				sleep(2);
				//file saving flush don't complete yet.
				continue;
			}

			ret = tx_data_to_tacoc(1, stream, len);
			if (ret >= 0)
				unlink("/var/stored_records");
			
			goto last_event_msg_sending; //server sending NOT repeatly, as power off step.
		}

	}

last_event_msg_sending:
	if (stream != NULL)
		free(stream);

	if(fd > 0)
		close(fd);

	if (last_event == 0) {
		fprintf(stderr, "tacoc : mdmc_power_off last_event = 0\n");
		return;
	}

	unsigned char *tmp_msg = (unsigned char *)malloc(100);
	unsigned char *encode_msg = (unsigned char *)malloc(200);

	memset (tmp_msg, 0, 100);
	set_working_action(1);
	g_connect_to_server();

	set_serial_port_mode(UT_DEFAULT_MODE);
	tmp_msg_len = build_report_msg(tmp_msg, 0, NULL, 27);
	encode_msg_len = encode_report_msg(encode_msg, tmp_msg, tmp_msg_len);
	ret = g_send_to_server(encode_msg, encode_msg_len);

	g_disconnect_to_server();
	set_working_action(0);

	free(tmp_msg);
	free(encode_msg);
	
	if(ret < 0)
		DTG_LOGD("++++++SIGOFF Data Sending Error");
}


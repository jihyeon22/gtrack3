/*
 * tacoc_api.c
 *
 *  Created on: 2013. 3. 8.
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

#include <board/board_system.h>
#include <standard_protocol.h>
#include <format_protocol.h>
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "sms_msg_process.h"
#include "rpc_clnt_operation.h"

ctrl_pkt_t end_ctrl_buf; 
int end_ctrl_buf_size = 0;

char end_dtg_buf[8*1024] = {0, };
int end_dtg_buf_size = 0;

int breakdown_report(int integer)
{
	return 0;
}

unsigned int dtg_sigon_stat = 0;
int send_record_data_msg(char* stream, int stream_size)
{
	int ret = 0;
#if 0
	if (power_get_power_source() == POWER_SRC_BATTERY) {
		DTG_LOGE("DC Line cutted!!");
		return -1;
	}
#endif

	if (dtg_sigon_stat == 0) {
		build_dtg_report_msg(stream, 26);
		unsigned short data_len = 12;
		memcpy(stream + 56, &data_len, 2);
		ret = g_send_to_server(stream, stream_size);
		if(ret < 0)
		{
			DTG_LOGD("++++++Data Sending Error");
			return -1;
		}
		dtg_sigon_stat = 26;
	} else {
		build_dtg_report_msg(stream, 5);
		ret = g_send_to_server(stream, stream_size);
		if(ret < 0)
		{
			DTG_LOGD("++++++Data Sending Error");
			return -1;
		}
	}

	
	return 0;
}

extern ctrl_strt_t ctrl_buf[];
extern int ctrl_index;
extern int ctrl_end;
extern int ctrl_cnt;
extern int ctrl_free;

unsigned int ctrl_sigon_stat = 0;
int sent_group = 0;
int tx_data_to_tacoc(int type, char *stream, int len)
{
	int i;
	int ret;
	int pack_size = 0 ;
	int num_group = 0;
	int rest_data_count = 0;
	char *ctrl_tmp_buf;
	char *dtg_buf;

	set_working_action(1);

	DTG_LOGD("Tacoc Data Reception Success, Type[%d], length[%d]", type, len);

	if (type == 0)	/* Summary */
	{
		DTG_LOGD("Summary!!");
		send_to_summary_server(stream, len);
		set_working_action(0);
		return 0;
	} 
	else if (type == 1)
	{
		pack_size = sizeof(mdt_packet_t) + sizeof(user_hdr_t) + sizeof(dtg_hdr_t) +
					(sizeof(dtg_data_t) * get_dtg_report_period());
		dtg_buf = malloc(pack_size * ((((len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t)) / get_dtg_report_period()) + 1));
		len = dsic_dtg_record_parsing(stream, len, dtg_buf, get_dtg_report_period());
		num_group = len / pack_size;
		rest_data_count = len % pack_size;
		DTG_LOGD("Daishin dtg report!! len: %d, ng: %d, pack_size: %d, rdc: %d", len, num_group, pack_size, rest_data_count);

		if(num_group > 0){
			for(i = sent_group; i<num_group; i++)
			{
				if (dtg_sigon_stat == 0) {
					g_connect_to_server();
					ret = send_record_data_msg(dtg_buf+(pack_size * i), 
							sizeof(mdt_packet_t) + sizeof(user_hdr_t));
					g_disconnect_to_server();
				}
				g_connect_to_server();
				ret = send_record_data_msg(dtg_buf+(pack_size * i), pack_size);
				g_disconnect_to_server();
				if(ret < 0)
				{
					DTG_LOGE("Message Trans Error");
					free(dtg_buf);
					set_working_action(0);
					return -1;
				}
				sent_group = i;
			}
		}
		if(rest_data_count > 0)
		{
			g_connect_to_server();
			ret = send_record_data_msg(dtg_buf+(pack_size * num_group), rest_data_count);
			g_disconnect_to_server();
			if(ret < 0) {
				DTG_LOGE("Message Trans Error");
				free(dtg_buf);
				set_working_action(0);
				return -1;
			}
			build_ctrl_report_msg(dtg_buf+(pack_size * num_group), 27);
			end_ctrl_buf_size = encode_ctrl_msg(&end_ctrl_buf, 
					dtg_buf+(pack_size * num_group), sizeof(mdt_packet_t) -1);

			build_dtg_report_msg(dtg_buf+(pack_size * num_group), 27);
			unsigned short data_len = 12;
			memcpy(stream + 56, &data_len, 2);
			memcpy(end_dtg_buf, dtg_buf+(pack_size * num_group), 
					sizeof(mdt_packet_t) + sizeof(user_hdr_t));
			//memcpy(end_dtg_buf, dtg_buf+(pack_size * num_group), rest_data_count);
			end_dtg_buf_size = sizeof(mdt_packet_t) + sizeof(user_hdr_t);
		}
		free(dtg_buf);
	} else {
		char *dtg_tmp_buf = malloc(sizeof(mdt_packet_t) + sizeof(user_hdr_t) + 
						sizeof(dtg_hdr_t) + sizeof(dtg_data_t));
		dsic_dtg_record_parsing(stream, len, dtg_tmp_buf, get_dtg_report_period());
		build_ctrl_report_msg(dtg_tmp_buf, 27);
		end_ctrl_buf_size = encode_ctrl_msg(&end_ctrl_buf, dtg_tmp_buf, 
											sizeof(mdt_packet_t) - 1);

		build_dtg_report_msg(dtg_tmp_buf, 27);
		unsigned short data_len = 12;
		memcpy(stream + 56, &data_len, 2);
		memcpy(end_dtg_buf, dtg_tmp_buf, sizeof(mdt_packet_t)+sizeof(user_hdr_t));
		end_dtg_buf_size = sizeof(mdt_packet_t)+sizeof(user_hdr_t);
		free(dtg_tmp_buf);

		ctrl_tmp_buf = malloc(sizeof(mdt_packet_t));
		len = dsic_ctrl_record_parsing(stream, len, ctrl_tmp_buf);
		DTG_LOGD("Daishin ctrl report!! len: %d", len);
		int ctrl_buf_size;
		int ctrl_buf_cnt;
		int i = 0;
		int tmp_index;

		if (ctrl_sigon_stat == 0) {
			ctrl_buf_cnt = save_ctrl_buf(ctrl_tmp_buf, len, 26);
			ctrl_sigon_stat = 26;
		} else {
			ctrl_buf_cnt = save_ctrl_buf(ctrl_tmp_buf, len, 5);
		}
		if (ctrl_buf_cnt >= (get_ctrl_report_period() / get_ctrl_create_period())) {
			tmp_index = ctrl_index;
			for (i = 0; i < ctrl_buf_cnt; i++) {
				ret = send_to_ctrl_server(&(ctrl_buf[tmp_index].buf), 
					ctrl_buf[tmp_index].buf_size);
				if (ret < 0) {
					DTG_LOGE("cotrol message error");
					set_working_action(0);
					free(ctrl_tmp_buf);
					return -1;
				}
				tmp_index++;
				if(tmp_index == CTRL_MAX)
					tmp_index = 0;
			}
		}
		set_working_action(0);
		clear_ctrl_buf(i);
		free(ctrl_tmp_buf);
		return 100;
	}
	
	set_working_action(0);
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
		case 'C':
			if(smsdata[2] == '1')
				ret = sms_set_ctrl_svrip_info(smsdata);
			else if(smsdata[2] == '2')
				ret = sms_set_ctrl_period(smsdata);
			break;
		case 'D':
			if(smsdata[2] == '1')
				ret = sms_set_dtg_svrip_info(smsdata);
			else if(smsdata[2] == '2')
				ret = sms_set_dtg_period(smsdata);
			break;
		case '1':
			if(smsdata[2] == '0')
				ret = sms_set_device_reset(smsdata);
			break;
		default:
			break;
	}

	return ret;
}

void mdmc_power_off()
{
	int fd, ret, result;
	int type = 0;
	int len = 0;
	int retry = 0;
	char *stream;

	if (is_working_action() == 2 ) {
		sleep(15);
	}

	ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2);
	if (ret < 0)
		return;
	else {
		do {
			fd = open("/var/stored_records", O_RDONLY, 0644 );
		} while (fd < 0 && errno != ENOENT);
		if (fd > 0) {
			stream = malloc(905*1024);
			memset(stream, 0, 905*1024);
			do{
				read(fd, &len, 4);
				ret = read(fd, stream, 905 * 1024);
				if (ret != len) {
					retry++;
					close(fd);
					sleep(2);
					do {
						fd = open("/var/stored_records", O_RDONLY, 0644 );
					} while (fd < 0 && errno != ENOENT);
				} else {
					retry = 0;
				}
			} while(ret != len && retry < 5);
			close(fd);
			ret = tx_data_to_tacoc(1, stream, len);
			if (ret >= 0){
				unlink("/var/stored_records");
			}
		}
	}
	if (stream != NULL)
		free(stream);

	if(end_ctrl_buf_size > 0){
		send_to_ctrl_server(&end_ctrl_buf, end_ctrl_buf_size);
	}
}


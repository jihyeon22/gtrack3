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
#include <errno.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>
#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "dtg_utill.h"
#include "rpc_clnt_operation.h"

char send_buf[512 * 1024] = {0,};
int send_buf_len = 0;

int tx_data_to_tacoc(int type, char *stream, int len)
{
	set_working_action(1, __func__, __LINE__);
	int net_ret=0;
	int fd = 0;

	DTG_LOGD("Tacoc Data Reception Success\n");

	if (type == 0) {
		/* Summary */
		send_to_summary_server(stream, len);
		
		set_working_action(0, __func__, __LINE__);
		alarm(get_interval());

		return 0;
	} else if (type == 1) {
		memset(send_buf, 0, 512 * 1024);
		send_buf_len = xml_wrapping(stream, send_buf, len);
		/* Compress CIP */
		if(gzlib_compress(send_buf, send_buf_len) >= 0) {
			net_ret = send_to_server(MSG_TYPE_DTG_DATA, get_encode_buffer(), get_encode_length(), send_buf_len, NULL,CIP_IP);
			if(net_ret == NET_SUCCESS_OK || net_ret == RETURN_OK) {
				// send ack_msg to taco
				set_working_action(0, __func__, __LINE__);
DTG_LOGI("Next Delivery Packet Interval : [%d] sec\n", get_interval());
				alarm(get_interval());
				memset(send_buf, 0, 512 * 1024);
				send_buf_len = 0;
				return 0;
			}
		} else {
			DTG_LOGE("gzip_compress error:length %d\n",send_buf_len);
		}
	}
	// send nack_msg to taco
	set_working_action(0, __func__, __LINE__);
	alarm(get_interval());
	return -1;
}


int tx_sms_to_tacoc(char *sender, char* smsdata)
{
	common_sms_parser(smsdata, sender);
	return 1;
}

int breakdown_report(int integer)
{
	int ret;
	char status;

	if(integer == 0)
	{
		status = '0';
		DTG_LOGD("breakdw msg: dtg problem occur\n");
	}
	else if(integer == 1)
	{	
		status = '1';
		DTG_LOGD("breakdw msg: dtg working\n");
		alarm(get_interval());
	}
	else {
		printf("breakdw msg: incorrect code\n");
		return 0;
	}
	
	ret = send_to_server(MSG_TYPE_DTG_BREAKDOWN, &status, sizeof(unsigned char), sizeof(unsigned char), NULL,MDS_IP);
	if(ret == RETURN_OK || ret == NET_SUCCESS_OK) {
		DTG_LOGD("send_reporting_dtg_breadown ok[%d]\n", ret);
	} else {
		DTG_LOGE("send_reporting_dtg_breadown fail[%d]\n", ret);
	}
	
	return 0;
}

#define MAX_SEDING_BUF_SIZE		100*1024
#define MAX_LAST_SENDING_SIZE	10*1024
void mdmc_power_off()
{
	int fd, ret, result;
	int type = 0;
	int len = 0;
	int retry = 0;
	char *stream;

	fprintf(stderr, "tacoc : mdmc_power_off #1\n");
	ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2);
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
}


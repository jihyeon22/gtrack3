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

// wrapper api
#include <wrapper/dtg_log.h>

// tacoc api
#include <standard_protocol.h>
#include <vehicle_msg.h>



// gtrack base code
#include <board/board_system.h>
#include <base/dmmgr.h>
#include <util/nettool.h>
#include <board/power.h>

#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "sms_msg_process.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include "parsing.h"

#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_send.h>
#endif

#define MAX_END_DTG_SIZE	8192

// warnnig fix
int common_sms_parser(char *sms_msg, char *caller);
int sms_set_mdt_period(char * sms_msg);
int sms_set_dtg_period(char * sms_msg);
int sms_device_status_req(char *sender);


tacom_std_hdr_t end_mdt_hdr;
mdt_pck_t end_mdt_buf; 
int end_mdt_buf_size = 0;


//#define HNURI_RESP_ERR_UNKOWN_DEVICE	-1 //for test

int breakdown_report(int integer)
{
	return 0;
}

int show_mdt_terminal_id(unsigned char *buf, char *debug_msg)
{
	int i;
	unsigned char ch;
	char tmp_buf[20];
	mdt_pck_t *mdt_pack_dump;
	mdt_pack_dump = (mdt_pck_t *)buf;

	if(sizeof(mdt_pack_dump->body.terminal_id) >= 20) {
		DTG_LOGE("%s:%d> buf length is over", __func__, __LINE__);
		return -1;
	}
	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	for(i = 0; i < sizeof(mdt_pack_dump->body.terminal_id); i++) {
		ch = mdt_pack_dump->body.terminal_id[i];
		if( !(ch == 0x20 || (ch >= '0' && ch <= '9'))) {
			DTG_LOGE("%s MDT terminal ID Fail : ch[%d]=[0x%02x]", debug_msg, i, ch);
			return -1;
		}
	}
	memcpy(tmp_buf, mdt_pack_dump->body.terminal_id, sizeof(mdt_pack_dump->body.terminal_id));
	DTG_LOGT("MDT-TERMINAL_ID : %s", tmp_buf);

	return 0;
}

int send_record_data_msg(int sock_fd, char* stream, int stream_size)
{
	char tmp_vrn[15];;
	int ret = 0;
	char *src_idx;
	dtg_data_t *tmp_dtg_data;
	dtg_pck_t * tmp_dtg;

	if(sock_fd < 0) {
		DTG_LOGD("send_record_data_msg: socket error");
		return -1;
	}

#if 0
	if (power_get_power_source() == POWER_SRC_BATTERY) {
		DTG_LOGE("DC Line cutted!!");
		return -1;
	}
#endif
	
	tmp_dtg = (dtg_pck_t *)stream;
	tmp_dtg->header.prtc_id = htons(tmp_dtg->header.prtc_id);
	tmp_dtg->body.trip_num = get_trip_number();
	tmp_dtg->body.dtg_num = htons(tmp_dtg->body.dtg_num);
	tmp_dtg->body.tid	  = htonl(tmp_dtg->body.tid);

	src_idx = stream + sizeof(dtg_pck_t);

	while (src_idx < (stream + stream_size)) {
		tmp_dtg_data = (dtg_data_t *)src_idx;

		tmp_dtg_data->timestamp = htonl(tmp_dtg_data->timestamp);
		tmp_dtg_data->distance_a_day = htonl(tmp_dtg_data->distance_a_day);
		tmp_dtg_data->distance_all = htonl(tmp_dtg_data->distance_all);
		tmp_dtg_data->distance_trip = htonl(tmp_dtg_data->distance_trip);
		tmp_dtg_data->rpm = htons(tmp_dtg_data->rpm);
		tmp_dtg_data->gps_x = htonl(tmp_dtg_data->gps_x);
		tmp_dtg_data->gps_y = htonl(tmp_dtg_data->gps_y);
		tmp_dtg_data->azimuth = htons(tmp_dtg_data->azimuth);
		tmp_dtg_data->accelation_x = htons(tmp_dtg_data->accelation_x);
		tmp_dtg_data->accelation_y = htons(tmp_dtg_data->accelation_y);

#if defined(DEVICE_MODEL_LOOP2)
		tmp_dtg_data->rtUsedFuelAday = htonl(tmp_dtg_data->rtUsedFuelAday);
		tmp_dtg_data->rtUsedFuelAll = htonl(tmp_dtg_data->rtUsedFuelAll);
#endif
		src_idx += sizeof(dtg_data_t);
	}
	memset(tmp_vrn, 0x00, sizeof(tmp_vrn));
	strncpy(tmp_vrn, tmp_dtg->body.dtg_hdr.vrn, 12);
	DTG_LOGI("DTG : VRN [%s]\n", tmp_vrn);
	ret = send_to_dtg_server(sock_fd, stream, stream_size, (char *)__func__, __LINE__, "dtg record_data_msg");
	
	if(ret < 0)
	{
		DTG_LOGD("++++++Data Sending Error");
		return -1;
	}
	return 0;
}

term_pck_t term_info_buf = {0,};
extern mdt_strt_t mdt_buf[];
extern int mdt_index;
extern int mdt_end;
extern int mdt_cnt;
extern int mdt_free;
extern unsigned int mdt_sigon_stat;

extern resp_pck_t msg_resp[];

#if defined(BOARD_TL500K)
	void add_server_no_ack_count();
#endif

//#define ERROR_RETURN_VALUE	222
#define ERROR_RETURN_VALUE		(-2222)
int tx_data_to_tacoc(int type, char *stream, int len)
{
	int i;
	int ret;
	int retry = 0;
	int pack_size = 0 ;
	int num_group = 0;
	int rest_data_count = 0;
	//unsigned char *ctrl_tmp_buf = NULL;
	unsigned char ctrl_tmp_buf[512];// = NULL;
	char *dtg_buf;
	int sock_fd;


	DTG_LOGD("Tacoc Data Reception Success, Type[%d], length[%d]", type, len);
	printf("%s:%d> Tacoc Data Reception Success, Type[%d], length[%d]\n", __func__, __LINE__, type, len);
	if(len <= 0) {
		return 0;
	}


	if (type == 0)	/* Summary */
	{
		DTG_LOGD("Summary!!");
//		send_to_summary_server(stream, len);
		return 0;
	} 
	else if (type == 1) //DTG DATA
	{
//printf("%s:%d> type #1\n", __func__, __LINE__);
		pack_size = sizeof(dtg_pck_t) +	
				(sizeof(dtg_data_t) * (get_dtg_report_period() + 100));
				//(sizeof(dtg_data_t) * (get_dtg_report_period()+(get_dtg_report_period()/2)));

		dtg_buf = malloc(pack_size * ((((len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t)) / (get_dtg_report_period()+100)) + 1));
		if(dtg_buf == NULL) {
			DTG_LOGE("dtg_buf mallock Error");
			return 0;
		}


//printf("%s:%d> type #1\n", __func__, __LINE__);
		len = hnrt_dtg_parsing(stream, len, dtg_buf, (get_dtg_report_period()+100));

		num_group = len / pack_size;
		rest_data_count = len % pack_size;
		DTG_LOGD("Hanuritien dtg report!! len: %d, ng: %d, pack_size: %d, rdc: %d", len, num_group, pack_size, rest_data_count);

//printf("%s:%d> type #1\n", __func__, __LINE__);
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		if (sock_fd < 0) {
/*
#if defined(BOARD_NEO_W200K)
			LOGE("DTG connection fail");
			if(is_found_ppp_device("ppp0") > 0) {
				LOGE("server no ack add\n");
				add_server_no_ack_count();
			}
#endif
*/
			free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
			return ERROR_RETURN_VALUE;
		}
		for(i = 0; i<num_group; i++) {
//printf("%s:%d> type #1\n", __func__, __LINE__);

RETRY_DTG:
			//dtg_buf[(pack_size * i)+6] &= 0xfe;
			//if ((i % 5 == 4) || (i+1 == num_group)) {
			//if ( ((i % 2) == 0) || (i+1 == num_group)) {
				dtg_buf[(pack_size * i)+6] |= 0x1;
			//}
//msg_hdr_t *hnrt_msg_hdr_temp;
//hnrt_msg_hdr_temp = (msg_hdr_t *)&dtg_buf[(pack_size * i)];
//DTG_LOGI("hnrt_msg_hdr_temp->msg_len_mark = [%02x][%02x][%02x]\n", hnrt_msg_hdr_temp->msg_len_mark[0], hnrt_msg_hdr_temp->msg_len_mark[1], hnrt_msg_hdr_temp->msg_len_mark[2]);
//DTG_LOGI("i % 5 = [%d]\n", (i % 5));


//printf("%s:%d> type #1\n", __func__, __LINE__);
			ret = send_record_data_msg(sock_fd, dtg_buf+(pack_size * i), pack_size);
//printf("%s:%d> type #1 ==> send_record_data_msg[%d]\n", __func__, __LINE__, ret);
			if(ret < 0) {
				DTG_LOGE("Message Trans Error");
				disconnect_to_server(sock_fd);
				sock_fd = -1;
				free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
				return ERROR_RETURN_VALUE;
			}
			sleep(1);
//printf("%s:%d> type #1 => i[%d] : num_group[%d]\n", __func__, __LINE__, i, num_group);
			//if ((i % 5 == 4) || (i+1 == num_group)) {
			if(1) {
//printf("%s:%d> type #1\n", __func__, __LINE__);
				ret = receive_response(sock_fd, 1);
//printf("%s:%d> type #1 ==> receive_response[%d]\n", __func__, __LINE__, ret);

				if (ret < 0){
					DTG_LOGE("Error recive response.");
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
					//return ret;
					return ERROR_RETURN_VALUE;
				} 
//printf("%s:%d> type #1\n", __func__, __LINE__);
				disconnect_to_server(sock_fd);
				sock_fd = -1;
//printf("%s:%d> type #1\n", __func__, __LINE__);
				if (msg_resp[0].result == HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE)
				{
					sleep(1);
					DTG_LOGE("%s:%d> Service No subscriber device\n", __func__, __LINE__);
					send_server_error_report("DTG DATA", HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE);
				}
				else if (msg_resp[0].result == HNURI_RESP_ERR_UNKOWN_DEVICE) {
					retry++;
					if (retry < 5) {
						sleep(10);
						i -= 4;
						goto RETRY_DTG;
					}
					DTG_LOGE("DATA format error recive response.");
					free(dtg_buf);
					send_server_error_report("DTG DATA", HNURI_RESP_ERR_UNKOWN_DEVICE);
//printf("%s:%d> type #1\n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				} else if (msg_resp[0].result != 1) {
					free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
					send_server_error_report("DTG DATA", msg_resp[0].result);
					return ERROR_RETURN_VALUE;
				}
				sleep(1);
				if (i+1 != num_group || rest_data_count > 0) {
//printf("%s:%d> type #1\n", __func__, __LINE__);
					sock_fd = connect_to_server(get_server_ip_addr(), 
											get_server_port());
					if (sock_fd < 0) {
						free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
						return ERROR_RETURN_VALUE;
					}
				}
			}
		}
		retry = 0;
		if(rest_data_count > 0 ) {
//printf("%s:%d> type #1\n", __func__, __LINE__);
RETRY_DTG_REST:
			dtg_buf[(pack_size * num_group)+6] |= 0x1;
			ret = send_record_data_msg(sock_fd, dtg_buf+(pack_size*num_group), rest_data_count);
			if(ret < 0) {
				DTG_LOGE("Message Trans Error");
				disconnect_to_server(sock_fd);
				sock_fd = -1;
				free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
				return ERROR_RETURN_VALUE;
			}
			sleep(1);
			ret = receive_response(sock_fd, 1);
			if (ret < 0){
				DTG_LOGE("Error recive response.");
				disconnect_to_server(sock_fd);
				sock_fd = -1;
				free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
				return ERROR_RETURN_VALUE;
			} 
			disconnect_to_server(sock_fd);
			sock_fd = -1;
			
			if (msg_resp[0].result == HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE)
			{
				sleep(1);
				DTG_LOGE("%s:%d> Service No subscriber device\n", __func__, __LINE__);
				send_server_error_report("DTG DATA", HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE);
			}
			else if (msg_resp[0].result == HNURI_RESP_ERR_UNKOWN_DEVICE) {
				retry++;
				if (retry < 5) {
					sleep(10);
					goto RETRY_DTG_REST;
				}
				DTG_LOGE("DATA format error receive response.");
				free(dtg_buf);
				send_server_error_report("DTG DATA", HNURI_RESP_ERR_UNKOWN_DEVICE);
//printf("%s:%d> type #1\n", __func__, __LINE__);
				return ERROR_RETURN_VALUE;
			} else if (msg_resp[0].result != 1) {
				free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
				send_server_error_report("DTG DATA", msg_resp[0].result);
				return ERROR_RETURN_VALUE;
			}
		}
		free(dtg_buf);
//printf("%s:%d> type #1\n", __func__, __LINE__);
	} else if (type == 2){
		DTG_LOGD("Hanuritien mdt report!! len: %d", len);

//printf("%s:%d> type #2\n", __func__, __LINE__);

		int mdt_buf_size;
		int mdt_buf_cnt;
		int i = 0;
		int tmp_index;
		int try_cnt = 0;

		//ctrl_tmp_buf = (unsigned char *)malloc(sizeof(term_pck_t)+sizeof(mdt_pck_t));
		//if(ctrl_tmp_buf == NULL) {
		//	printf("=====================================>\n");
		//	printf("ctrl_tmp_buf is NULL\n");
		//}
		memset(ctrl_tmp_buf, 0x00, sizeof(ctrl_tmp_buf));
		len = term_info_parsing(ctrl_tmp_buf, (tacom_std_hdr_t *)stream);
		printf("term_infor_parsing lenth = [%d]\n", len);
		len += mdt_parsing(&ctrl_tmp_buf[len], stream + sizeof(tacom_std_hdr_t));

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		memcpy(&end_mdt_hdr, stream, sizeof(tacom_std_hdr_t));
#endif

//extern void dump_packet(unsigned char *buf, int len, char *msg);

		build_mdt_report_msg(&end_mdt_buf, (msg_mdt_t*)&ctrl_tmp_buf[sizeof(term_pck_t)], 27); //27 is sigoff
		end_mdt_buf_size = sizeof(mdt_pck_t);

//printf("%s:%d> type #2\n", __func__, __LINE__);
		if (mdt_sigon_stat == 0) {
			memcpy(&term_info_buf, ctrl_tmp_buf, sizeof(term_pck_t));
			term_info_buf.header.prtc_id = 
				htons(term_info_buf.header.prtc_id);
			mdt_pck_t tmp_buf;// = malloc(sizeof(mdt_pck_t));
			memset(&tmp_buf, 0x00, sizeof(tmp_buf));
			build_mdt_report_msg(&tmp_buf, (msg_mdt_t*)&ctrl_tmp_buf[sizeof(term_pck_t)], 26); //26 is sigon
			tmp_buf.header.msg_len_mark[2] |= 0x1;
			//printf("sizeof(msg_mdt_t) = [%d]\n", sizeof(msg_mdt_t));
			//printf("sizeof(term_pck_t) = [%d]\n", sizeof(term_pck_t));
			//msg_mdt_t *test = (msg_mdt_t*) (ctrl_tmp_buf+sizeof(term_pck_t));
			//dump_packet(test, sizeof(msg_mdt_t), "test #1");
			//dump_packet(tmp_buf, sizeof(mdt_pck_t), "tmp_buf #1");
			//dump_packet( (ctrl_tmp_buf+sizeof(term_pck_t)), sizeof(msg_mdt_t), "ctrl_msg_mdt #2");			
			do {
//printf("%s:%d> type #2\n", __func__, __LINE__);
				sock_fd = connect_to_server(get_server_ip_addr(), 
										get_server_port());
				if (sock_fd < 0) {
					mdt_sigon_stat = 0;
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2\n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				}
				ret = send_to_dtg_server(sock_fd, (unsigned char *)&term_info_buf, sizeof(term_pck_t), (char *)__func__, __LINE__, "MDT terminal info message");
				if (ret < 0) {
					DTG_LOGE("terminal info message error");
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					mdt_sigon_stat = 0;
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2\n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				}
				sleep(1);
				show_mdt_terminal_id((unsigned char *)&tmp_buf, "MDT cotrol message");
				ret = send_to_dtg_server(sock_fd, (unsigned char *)&tmp_buf, sizeof(mdt_pck_t), (char *)__func__, __LINE__, "MDT cotrol message");
				if (ret < 0) {
					DTG_LOGE("cotrol message error #1");
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					mdt_sigon_stat = 0;
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2\n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				}
				sleep(1);
				ret = receive_response(sock_fd, 0);
				if (ret < 0){
					DTG_LOGE("Error recive response.");
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2\n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				} 
				disconnect_to_server(sock_fd);
				sock_fd = -1;
				if (msg_resp[0].result == 1)
					mdt_sigon_stat = 1;
				else if (msg_resp[0].result == HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE)
				{
					sleep(1);
					mdt_sigon_stat = 1;
					DTG_LOGE("%s:%d> Service No subscriber device\n", __func__, __LINE__);
					send_server_error_report("MDT", HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE);
				}
				else if (msg_resp[0].result == HNURI_RESP_ERR_UNKOWN_DEVICE) {
					send_server_error_report("MDT", HNURI_RESP_ERR_UNKOWN_DEVICE);
					sleep(10);
				}
				try_cnt++;
			} while(msg_resp[0].result == HNURI_RESP_ERR_UNKOWN_DEVICE && try_cnt < 5);
			//free(tmp_buf);
			//free(ctrl_tmp_buf);
//printf("%s:%d> type #2 \n", __func__, __LINE__);
			return 100;
		}

		mdt_buf_cnt = save_mdt_buf(&ctrl_tmp_buf[sizeof(term_pck_t)], 
									len - sizeof(term_pck_t), 5);
		if (mdt_buf_cnt >= (get_mdt_report_period() / get_mdt_create_period())) {
			/*
			sock_fd = connect_to_server(get_server_ip_addr(), 
										get_server_port());
			if (sock_fd < 0) {
//printf("%s:%d> type #2 \n", __func__, __LINE__);
				return ERROR_RETURN_VALUE;
			}
			*/
			tmp_index = mdt_index;
			for (i = 0; i < mdt_buf_cnt; i++) {
//printf("%s:%d> type #2 \n", __func__, __LINE__);
				sock_fd = connect_to_server(get_server_ip_addr(), 
											get_server_port());
				if (sock_fd < 0) {
					//free(ctrl_tmp_buf);
					return ERROR_RETURN_VALUE;
				}

				//mdt_buf[tmp_index].buf.header.msg_len_mark[2] &= 0xfe;
				//if ((i+1) == mdt_buf_cnt)
				mdt_buf[tmp_index].buf.header.msg_len_mark[2] |= 0x1; //set server response flag


				show_mdt_terminal_id((unsigned char *)&(mdt_buf[tmp_index].buf), "MDT cotrol message#2");
				ret = send_to_dtg_server(sock_fd, (unsigned char *)&(mdt_buf[tmp_index].buf), mdt_buf[tmp_index].buf_size, (char *)__func__, __LINE__, "MDT cotrol message#2");
				if (ret < 0) {
					DTG_LOGE("cotrol message error #2 : tmp_index[%d], buf_size[%d]", tmp_index, mdt_buf[tmp_index].buf_size);
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2 \n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				}
				tmp_index++;
				if(tmp_index == MDT_MAX)
					tmp_index = 0;
				sleep(1);

				ret = receive_response(sock_fd, 0);
				if (ret < 0){
					DTG_LOGE("Error recive response.");
					disconnect_to_server(sock_fd);
					sock_fd = -1;
					//free(ctrl_tmp_buf);
	//printf("%s:%d> type #2 \n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				} 

				disconnect_to_server(sock_fd);
				sock_fd = -1;

				if (msg_resp[0].result != 1) {
					//free(ctrl_tmp_buf);
//printf("%s:%d> type #2 \n", __func__, __LINE__);
					return ERROR_RETURN_VALUE;
				}

			} //end for

		}
		clear_mdt_buf(i);
		//free(ctrl_tmp_buf);
//printf("%s:%d> type #2 \n", __func__, __LINE__);
		return 100;
	} else {
		//printf("%s:%d> Tacoc Data Reception Success, Type[%d], length[%d]\n", __func__, __LINE__, type, len);
		//ctrl_tmp_buf = malloc(sizeof(mdt_pck_t));
		memset(ctrl_tmp_buf, 0x00, sizeof(ctrl_tmp_buf));
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		memcpy(&end_mdt_hdr, stream, sizeof(tacom_std_hdr_t));
#endif
		len = mdt_parsing(ctrl_tmp_buf, stream + sizeof(tacom_std_hdr_t));
		build_mdt_report_msg(&end_mdt_buf, (msg_mdt_t*)ctrl_tmp_buf, 27); //27 is sigoff
		end_mdt_buf_size = sizeof(mdt_pck_t);
		//free(ctrl_tmp_buf);
	}
	
	
	return 0;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
	int ret = 0;
	DTG_LOGD("+++++sms(2): %s", smsdata);

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
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
			gpio_set_value(15, 0);
#endif

			while(1) {
				//system("poweroff");
				DTG_LOGI("%s> kt fota reset wait powerorff...\n", __func__);
				poweroff(NULL,0);
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
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
			gpio_set_value(15, 0);
#endif

			while(1) {
				//system("poweroff");
				DTG_LOGI("%s> kt fota reset wait powerorff...\n", __func__);
				poweroff(NULL,0);
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
	ret = common_sms_parser(smsdata, sender);
	if (ret == 0)
		return ret;

	if(strlen(smsdata) < 3 || smsdata[0] != '&')
		return -1;

	DTG_LOGI("%s> smsdata[1]------------>[%c]\n", __func__, smsdata[1]);
	switch(smsdata[1])
	{
		case '1':
			if(smsdata[2] == ',')
				ret = sms_set_svrip_info(smsdata);
			else if(smsdata[2] == '0')
				ret = sms_set_device_reset(smsdata);
			else if(smsdata[2] == '1')
				ret = sms_set_mdt_period(smsdata);
			break;
		case '2':
			if(smsdata[2] == ',')
				ret = sms_set_mdt_period(smsdata);
			else if(smsdata[2] == '0')
				DTG_LOGW("Fermware upgrade SMS");
			else if(smsdata[2] == '1')
				ret = sms_set_dtg_period(smsdata);
			else if(smsdata[2] == '2')
				DTG_LOGW("Proofreading factor SMS");
			else if(smsdata[2] == '3')
				DTG_LOGW("Vehichle infomation SMS");
			else if(smsdata[2] == '4')
				DTG_LOGW("DTG file requirement SMS");
			else if(smsdata[2] == '5')
				ret = sms_set_svrip_info(smsdata);
			break;
		case '3':
			DTG_LOGW("cumulated distance SMS");
			break;
		case '4':
			ret = sms_device_status_req(smsdata);
			break;
		default:
			break;
	}

	return ret;
}

//#define MAX_LAST_SENDING_SIZE	10*1024
#define MAX_LAST_SENDING_DATA_COUNT	600 //10 min
void mdmc_power_off()
{
	int ready_cnt = 0;
	//int ret, result;
	int ret;
	int fd = 0;
	//int type = 0;
	//int len = 0;
	//int retry = 0;
	char *stream = NULL;
	int sock_fd;

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

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	if(end_mdt_buf_size != 0)
		data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(mdt_pck_t), 1); //mdt data create
#endif

	DTG_LOGE("tacoc : mdmc_power_off #1\n");
	ret = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
	if(ret < MAX_LAST_SENDING_DATA_COUNT) {
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
		if(ret == 0) //success
		{
			ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
		}
	}

	ret = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
	if(ret > 10) //remain count
	{ 
		ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save
		if (ret < 0) 
		{
			DTG_LOGE("tacoc : mdmc_power_off last_event_msg_sending [%d]\n", ret);
			goto last_event_msg_sending;
		}
	}

last_event_msg_sending:
	if (stream != NULL)
		free(stream);

	if(fd > 0)
		close(fd);

	if(end_mdt_buf_size != 0)
	{
		end_mdt_buf.header.msg_len_mark[2] |= 0x1;
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		DTG_LOGI("mdmc_power_off> end_mdt_buf data send..");
		if(sock_fd > 0) {

			show_mdt_terminal_id((unsigned char *)&end_mdt_buf, "power off msg");
			send_to_dtg_server(sock_fd, &end_mdt_buf, end_mdt_buf_size, (char *)__func__, __LINE__, "power off msg");
			disconnect_to_server(sock_fd);
		}
	}

	send_device_de_registration();
	// dmmgr_send(eEVENT_PWR_OFF, NULL, 0);
}


void mdmc_power_off_()
{
	int ret;
	int ready_cnt = 0;
	int sock_fd;
	int remain_count = 0;
	//int trasf_cnt = 0;

	while (is_working_action() == 2 && ready_cnt < 6) {
		sleep(5);
		ready_cnt++;
	}

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	if(end_mdt_buf_size != 0)
		data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(mdt_pck_t), 1); //mdt data create
#endif

	if(end_mdt_buf_size != 0)
	{
		end_mdt_buf.header.msg_len_mark[2] |= 0x1;
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		DTG_LOGI("mdmc_power_off_> end_mdt_buf data send..");
		if(sock_fd > 0) {
			show_mdt_terminal_id((unsigned char *)&end_mdt_buf, "IGN Off message");
			send_to_dtg_server(sock_fd, &end_mdt_buf, end_mdt_buf_size, (char *)__func__, __LINE__, "IGN Off message");
			disconnect_to_server(sock_fd);
		}
	}

	ready_cnt = 0;
	while(ready_cnt < 3) {

		if(power_get_ignition_status() == POWER_IGNITION_ON)
			break;

		remain_count = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
		DTG_LOGI("%s:%d remain data count [%d]\n", __func__, __LINE__, remain_count);
		if(remain_count < 10) {
			break;
		}

		ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
		DTG_LOGI("%s:%d data_req_to_taco_cmd [%d]\n", __func__, __LINE__, ret);
		if (ret < 0) {
			DTG_LOGE("dtg data read error.");
			ready_cnt += 1;
			sleep(5);
			continue;
		} else if (ret == 101){
			DTG_LOGD("not enough data records.");
			ready_cnt += 1;
			sleep(5);
			continue;
		} else {
			ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
			DTG_LOGI("%s:%d data_req_to_taco_cmd [%d]\n", __func__, __LINE__, ret);
		}

	}
}
void tacoc_ignition_off()
{
	mdt_sigon_stat = 0; //meaning is sig off

	mdmc_power_off_();

#if defined(BOARD_TL500K)
	#if ! (defined(DEVICE_MODEL_LOOP) || defined(DEVICE_MODEL_LOOP2) || defined(DEVICE_MODEL_CHOYOUNG) )
		send_device_off_packet(30);
	#endif
#endif
	send_device_de_registration();

	save_vgps_info(NULL);
}

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <time.h>

#include <util/nettool.h>
#include <util/crc16.h>

#include <wrapper/dtg_log.h>

#include <standard_protocol.h>
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_packet.h"

extern msg_mdt_t mdt_buf;

unsigned long last_gps = 1;

int mdt_parsing(char *destbuf, tacom_std_data_t *srcbuf)
{
	DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
	char *phonenum = NULL;
	int tmp_int = 0;

	msg_mdt_t *mdt_data = (msg_mdt_t *) destbuf;

	mdt_data->protocol_id = 0x11;
	memset(mdt_data->terminal_id, 0x20, 15);
	phonenum  = atcmd_get_phonenum();
	memcpy(mdt_data->terminal_id, phonenum, strlen(phonenum));

	mdt_data->year = 2000 + char_mbtol(srcbuf->date_time, 2);
	mdt_data->month = char_mbtol(srcbuf->date_time+2, 2);
	mdt_data->day = char_mbtol(srcbuf->date_time+4, 2);
	mdt_data->hour = char_mbtol(srcbuf->date_time+6, 2);
	mdt_data->min = char_mbtol(srcbuf->date_time+8, 2);
	mdt_data->sec = char_mbtol(srcbuf->date_time+10, 2);

	mdt_data->position_type = 1;
	mdt_data->gps_x = char_mbtol(srcbuf->gps_x, 9);
	mdt_data->gps_y = char_mbtol(srcbuf->gps_y, 9);

	if (mdt_data->gps_x == 0 || mdt_data->gps_y == 0) {
		mdt_data->gps_x = 0;
		mdt_data->gps_y = 0;
		last_gps = 0;
	} else {
		if(last_gps == 0) {
			last_gps = 1;
			mdt_data->event_code = 202;
		}
	}

	tmp_int = char_mbtol(srcbuf->azimuth, 9);

	if (tmp_int >= 338 || tmp_int < 23)
		mdt_data->azimuth = 0;
	else if (tmp_int >= 23 && tmp_int < 68)
		mdt_data->azimuth = 1;
	else if (tmp_int >= 68 && tmp_int < 113)
		mdt_data->azimuth = 2;
	else if (tmp_int >= 113 && tmp_int < 158)
		mdt_data->azimuth = 3;
	else if (tmp_int >= 158 && tmp_int < 203)
		mdt_data->azimuth = 4;
	else if (tmp_int >= 203 && tmp_int < 248)
		mdt_data->azimuth = 5;
	else if (tmp_int >= 248 && tmp_int < 293)
		mdt_data->azimuth = 6;
	else if (tmp_int >= 293 && tmp_int < 338)
		mdt_data->azimuth = 7;

	mdt_data->speed = char_mbtol(srcbuf->speed, 3);

	mdt_data->accumulative_distance = char_mbtol(srcbuf->cumul_run_dist, 7) * 1000;

	mdt_data->temperature_a = char_mbtol(srcbuf->temperature_A, 5);
	mdt_data->temperature_b = char_mbtol(srcbuf->temperature_B, 5);
	mdt_data->temperature_c = -5555;
	mdt_data->gpio_input = 0;
	mdt_data->power_type = 0;

	DTG_LOGD("Complete MDT message parsing.");
	return sizeof(msg_mdt_t);
}

int last_event = 0;
unsigned char logon_time[7] = {0};
int build_report_msg(char* msg, int record_num, char* record, unsigned char event_code)
{
	int msg_count = 0;
	unsigned short tmp_word;
	struct tm *struct_time;
	time_t current_time = time(NULL);
	struct_time = localtime( &current_time);

	int record_len = RECORD_HDR_LEN+record_num*RECORD_DATA_LEN;
	int serial_mode = get_serial_port_mode(); //UT_DEFAULT_MODE

	if(serial_mode == UT_DEFAULT_MODE || record_num == 0) {
		memcpy(msg, &mdt_buf, sizeof(msg_mdt_t));
		msg_count += sizeof(msg_mdt_t);
		memset(msg+1, 0x64, 1);
		tmp_word = (unsigned short)get_trans_period();
		memcpy(msg+47, &tmp_word, 2);
		tmp_word = (unsigned short)get_create_period();
		memcpy(msg+51, &tmp_word, 2);
	} else {
		memcpy(msg, &mdt_buf, sizeof(msg_mdt_t) - 12);
		msg_count += (sizeof(msg_mdt_t) - 12);
		memcpy(msg+msg_count, (&mdt_buf)+msg_count+6, 6);
		msg_count += 6;
		memset(msg+1, 0x81, 1);
		tmp_word = (unsigned short)get_trans_period();
		memcpy(msg+41, &tmp_word, 2);
		tmp_word = (unsigned short)get_create_period();
		memcpy(msg+45, &tmp_word, 2);

		memcpy(msg+msg_count, "VER", 3); // device version
		msg_count += 3;

		memset(msg+msg_count, record_num, 1);  // data len
		msg_count += 1;
		memcpy(msg+msg_count, record, record_len); // data
		msg_count += record_len;
	}

	memset(msg+msg_count, 0x0f, 1); // car pw
	msg_count ++;
	memset(msg+msg_count, 0x27, 1);
	msg_count ++;

	memcpy(msg+msg_count, get_company_code(), 5); // company code
	msg_count += 5;
	memset(msg+msg_count, 0x20, 5);
	msg_count += 5;

	if (event_code == 27)
		memset(msg+msg_count, 1, 1);
	else
		memset(msg+msg_count, 0, 1);
	msg_count ++;

	if(last_event == 0 || event_code == 27){
		set_current_ctime();
		tmp_word = get_ctime_year();   // set ctime
		memcpy(logon_time, &tmp_word, 2);
		tmp_word = get_ctime_mon();   // set ctime
		memcpy(&logon_time[2], &tmp_word, 1);
		tmp_word = get_ctime_day();   // set ctime
		memcpy(&logon_time[3], &tmp_word, 1);
		tmp_word = get_ctime_hour();   // set ctime
		memcpy(&logon_time[4], &tmp_word, 1);
		tmp_word = get_ctime_min();   // set ctime
		memcpy(&logon_time[5], &tmp_word, 1);
		tmp_word = get_ctime_sec();   // set ctime
		memcpy(&logon_time[6], &tmp_word, 1);
	}
	memcpy(msg+msg_count, logon_time, 7);
	msg_count += 7;

	if(last_event == 0) {
		memset(msg+17, 26, 1); //event code
		last_event = 26;
	} else {
		memset(msg+17, event_code, 1); //event code
	}
	tmp_word = struct_time->tm_year+1900;
	memcpy(msg+18, &tmp_word, 2); // year
	memset(msg+20, struct_time->tm_mon+1, 1); //mon
	memset(msg+21, struct_time->tm_mday, 1); //day
	memset(msg+22, struct_time->tm_hour, 1); //hour
	memset(msg+23, struct_time->tm_min, 1); //min
	memset(msg+24, struct_time->tm_sec, 1); //sec

	return msg_count;
}

int encode_report_msg(char* report_msg, char* tmp_report, int tmp_report_len)
{
	int i;
	int e_count = 0;
	unsigned char ec_value = 0x20;
	unsigned char end_flag = 0x7e;
	unsigned char escape_flag = 0x7d;
	unsigned short crc16_value = crc16_get(tmp_report, tmp_report_len);

	memcpy(tmp_report+tmp_report_len, &crc16_value, sizeof(unsigned short));
	tmp_report_len += sizeof(unsigned short);

	for(i=0; i<tmp_report_len; i++)
	{
		if(tmp_report[i] == end_flag || tmp_report[i] == escape_flag)
		{
			report_msg[e_count++] = escape_flag;
			report_msg[e_count++] = tmp_report[i] ^ ec_value;
			continue;
		}
		report_msg[e_count++] = tmp_report[i];
	}
	memset(report_msg+e_count, end_flag, 1);
	e_count++;
	//DTG_LOGD("++++++ TRIPOS STEP_10.6: %d\n", e_count);
	return e_count;
}

int save_report_msg()
{	
	reset_report_msg_info();
	int* count = get_report_msg_count();
	int* msg_len = get_report_msg_len();
	char (*report_msg)[MAX_REPORT_MSG_LEN] = get_report_msg();
	char tmp_report[100] = {0,};
	int tmp_report_len;

	tmp_report_len = build_report_msg(tmp_report, 0, NULL, 5);
	msg_len[*count] = encode_report_msg(report_msg[*count], tmp_report, tmp_report_len);
	(*count)++;
	DTG_LOGD("++++++ TRIPOS STEP_10");
	return 0;
}

int send_saved_report_msg()
{
	int i;
	int* count = get_report_msg_count();
	int* msg_len = get_report_msg_len();
	char (*report_msg)[MAX_REPORT_MSG_LEN] = get_report_msg();

	for(i=0; i<*count; i++)
	{
		send_to_server(report_msg[i], msg_len[i]);
	}

	reset_report_msg_info();
	DTG_LOGD("++++++ TRIPOS STEP_14");
	return 0;
}

int g_hsock = 0;

int g_disconnect_to_server()
{
	if(g_hsock > 0)
	{
		close(g_hsock);
		g_hsock = 0;
		DTG_LOGI("Server close");
	}
	return 0;
}

int g_connect_to_server()
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

	int host_port = 0;
	char* host_name= NULL;
	struct sockaddr_in my_addr;
	int bytecount;
	int err = 0;
	int flag;
	int reuse = 1;
	int received_max_time_out = 1;
	int retry = 0;

	host_name = get_server_ip_addr();
	host_port = get_server_port();

	g_hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(g_hsock == -1) {
		DTG_LOGE("socket creat err: %s", strerror(errno));
		return -1;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);

	for(retry = 0; retry < 3; retry++) {
		my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
		if(my_addr.sin_addr.s_addr != 0)
			break;
		else if(my_addr.sin_addr.s_addr == 0) {
			DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
			err = -1;
			if(retry == 2)
				g_disconnect_to_server();
		}
	}

	for(retry = 0; retry < 3; retry++) {
		DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);
		if (nettool_connect_timeo(g_hsock, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
			DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
			err = -1;
			if(retry == 2)
				g_disconnect_to_server();
		} else {
			DTG_LOGD("Server connected");
			break;
		}
	}

	return err;
}

int g_send_to_server(unsigned char *buffer_in, int buffer_len)
{
	int bytecount;
	int err = 0;

	if(g_hsock <= 0) {
		DTG_LOGI("server socket handle not ready : g_send_to_server skip");
		return -1;
	}

	DTG_LOGI("Server sending.....");
	//net_packet_dump("triphos packet dump", buffer_in, buffer_len);
G_SEND_RETRY:
	bytecount = nettool_send_timedwait(g_hsock, (const char *)buffer_in, buffer_len, 0, 20);
	if (bytecount < 0) {
		DTG_LOGE("nettool_send_timedwait err: %s", strerror(errno));
		if (bytecount == EINPROGRESS){
			DTG_LOGW("got G_SEND_RETRY by jwrho.");
			goto G_SEND_RETRY;
		}
		err = -1;
		goto FINISH2;
	} else if (bytecount != buffer_len) {
		DTG_LOGE("nettool_send_timedwait err: %s , sent length[%d], sending length[%d]", strerror(errno), bytecount, buffer_len);
		//err = -1;
		buffer_in = buffer_in + bytecount;
		buffer_len = buffer_len - bytecount;
		goto G_SEND_RETRY;
	}

	DTG_LOGI("Triphos Server send success!! : sent length[%d], sending length[%d]", bytecount, buffer_len);

FINISH2:
;
	return err;
}

int send_to_server(unsigned char *buffer_in, int buffer_len)
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

	int host_port = 0;
	char* host_name= NULL;
	struct sockaddr_in my_addr;
	int bytecount;
	int hsock = 0;
	int err = 0;
	int flag;
	int reuse = 1;
	int received_max_time_out = 1;

	host_name = get_server_ip_addr();
	host_port = get_server_port();

#if 0
	int i =0;
	short tmp_short;
	int tmp_int;

	DTG_LOGD("+++++Sending Len: %d", buffer_len);

	for(i; i < 2; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // id, type
	}


	for(i; i < 17; i++)
	{
		DTG_LOGD("%d(%d):  %c", i, buffer_len, buffer_in[i]); // pn
	}
	for(i; i < 18; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // event code
	}

	memcpy(&tmp_short, buffer_in+i, 2);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_short); // year
	i += 2;

	for(i; i < 26; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // time ~ gps status
	}

	memcpy(&tmp_int, buffer_in+i, 4);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_int); // gpsx
	i += 4;

	memcpy(&tmp_int, buffer_in+i, 4);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_int); // gpsy
	i += 4;

	for(i; i < 35; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // dir
	}

	memcpy(&tmp_short, buffer_in+i, 2);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_short); // speed
	i += 2;

	memcpy(&tmp_int, buffer_in+i, 4);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_int); // cul dist
	i += 4;

	memcpy(&tmp_short, buffer_in+i, 2);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_short); // report period
	i += 2;

	for(i; i < 45; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // gpio, pw type
	}

	memcpy(&tmp_short, buffer_in+i, 2);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_short); // create period
	i += 2;

	int ttt_ccc = i;

	for(i; i < ttt_ccc + 3; i++)
	{
		DTG_LOGD("%d(%d):  %c", i, buffer_len, buffer_in[i]); // car pw
	}


	DTG_LOGD("%d(%d):  %d", i, buffer_len, buffer_in[i]); // data num
	i++;

	//i += (NUM_DD*LEN_BODY+LEN_HDR);
	i += (buffer_len-74);

	ttt_ccc = i;
	for(i; i < ttt_ccc + 2; i++)
	{
		DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // car pw
	}

	ttt_ccc = i;
	for(i; i < ttt_ccc + 10; i++)
	{
		DTG_LOGD("%d(%d):  %c", i, buffer_len, buffer_in[i]); // company code
	}

	DTG_LOGD("%d(%d):  %x", i, buffer_len, buffer_in[i]); // status
	i++;

	memcpy(&tmp_short, buffer_in+i, 2);
	DTG_LOGD("%d(%d):  %d", i, buffer_len, tmp_short); // year
	i += 2;

	ttt_ccc = i;
	for(i; i < ttt_ccc + 5; i++)
	{
		DTG_LOGD("%d(%d):  %d", i, buffer_len, buffer_in[i]);
	}
#endif

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1)
	{
		err = -1;
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto FINISH;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		err = -1;
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto FINISH;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		err = -1;
		goto FINISH;
	}

	//net_packet_dump("triphos packet dump", buffer_in, buffer_len);
	DTG_LOGI("Server sending.....");

SEND_RETRY:
	bytecount = nettool_send_timedwait(hsock, (const char *)buffer_in, buffer_len, 0, 25);
	if (bytecount < 0) {
		DTG_LOGE("nettool_send_timedwait err: %s", strerror(errno));
		err = -1;
		goto FINISH;
	} else if (bytecount != buffer_len) {
		DTG_LOGE("nettool_send_timedwait err: sent length[%d], sending length[%d]", bytecount, buffer_len);
		buffer_in = buffer_in + bytecount;
		buffer_len = buffer_len - bytecount;
		goto SEND_RETRY;
	}

	DTG_LOGI("Triphos Server send success!! : sent length[%d], sending length[%d]", bytecount, buffer_len);

FINISH:
;
	if(hsock > 0) {
		close(hsock);
		DTG_LOGI("Server[%s:%d] close", host_name, host_port);
	}
	return err;
}

int send_to_summary_server(unsigned char  *buffer_in, int buffer_len)
{
	DTG_LOGD("%s: %s ++\n", __FILE__, __func__);


	int packet_legnth = 0;
	int send_retry_cnt = 0;
	int host_port = 0;
	char* host_name= NULL;
	int rev_cnt = 0;
	int i;
	int packet_sent_size = 0;
	int sent_len = 0;
	int sending_len = 0;

	struct sockaddr_in my_addr;
	int bytecount;
	int hsock = 0;
	int * p_int;
	int err = NET_SUCCESS_OK;
	char resp_code = 0;
	int connect_cnt = 0;
	int flag;
	char recv_buf[4];
	int recv_len;
	int recved_len;

	host_name = get_summary_server_ip_addr();
	host_port = get_summary_server_port();

	DEBUG_INFO("Summary host_addr = <%s>\n", host_name);
	DEBUG_INFO("Summary host_port = <%d>\n", host_port);

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		err = NET_SOCKET_OPEN_FAIL;
		DTG_LOGE("Summary Error initializing socket %d: %s",errno, strerror(errno));
		goto FINISH_SUMMARY;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);	//for linux server
	//my_addr.sin_port = ntohl(host_port);	//for windows server
	//my_addr.sin_port = host_port;
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		err = NET_SOCKET_OPEN_FAIL;
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto FINISH_SUMMARY;
	}

	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

	while(1) {
		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5)
				DTG_LOGW("Summary server connect err[%d]: %s", errno, strerror(errno));
		} else {
			DTG_LOGI("Summary server connect success");
			break;
		}

		if(connect_cnt > (RECEIVED_MAX_TIME_OUT/2)) {
			err = NET_SERVER_CONNECT_ERROR;
			DTG_LOGE("Summary server connect err[%d]: %s", errno, strerror(errno));
			goto FINISH_SUMMARY;
		}
		connect_cnt += 1;
		sleep(1);
	}

	sent_len = 0;
	send_retry_cnt = 0;
	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	DTG_LOGD("Summary sending length[%d]", buffer_len);
	//sending_len = buffer_len;
	while(1) {
		bytecount = 0;
		if( (bytecount=send(hsock, (const char *)&buffer_in[sent_len], sending_len,0)) <= 0 )
			send_retry_cnt += 1;
		else {
			DTG_LOGI("Summary sent length[%d]", bytecount);
			sent_len += bytecount;
			if(buffer_len <= sent_len) {
				break;
			}
			else {
				if( (buffer_len - sent_len) >= 1024 )
					sending_len = 1024;
				else
					sending_len = (buffer_len - sent_len);
			}
		}

		if(send_retry_cnt > 300) {
			DTG_LOGE("Summary Send Packet Time out");
			err = NET_SEND_PACKET_ERROR;
			goto FINISH_SUMMARY;
		}
		usleep(1000*200); //300ms
	}
	DTG_LOGW("Summary sending send_retry_cnt[%d]\n", send_retry_cnt);

	recv_len = 4;
	recved_len = 0;
	memset(recv_buf, 0x00, sizeof(recv_buf));
	while(1) {	
		//recv�� length�� 0���� �ָ� non_blocking �Լ��� �ȴ�.
		if((bytecount = recv(hsock, &recv_buf[recved_len], recv_len, 0)) <= 0){
			DTG_LOGD(".");

			rev_cnt++;
			sleep(1);
		} else {
			rev_cnt = 0;
			DTG_LOGD("#");

			recved_len += bytecount;
			if(recved_len == 4) {
				DTG_LOGD("Summary Recv [%c][%c][%c][%c]\n", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);
				break;
			}
			else {
				recv_len = (4 - recved_len);
			}
			usleep(1000*200);
		}

		if(rev_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			DTG_LOGE("Summary Recv Time Out[%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			goto FINISH_SUMMARY;
		}
		
	}//end of while

	if( !((recved_len == 4) && (!memcmp(recv_buf, "TRUE", 4))) ) {
		DTG_LOGE("Summary Received Packet Error[%d]", errno);
		err = NET_RECV_PACKET_ERROR;
	}

FINISH_SUMMARY:
;
	if(hsock > 0)
		close(hsock);
	DTG_LOGD("%s: %s -- : err[%d]\n", __FILE__, __func__, err);

	return err;
}


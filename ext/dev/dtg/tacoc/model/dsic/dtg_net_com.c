<<<<<<< HEAD
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

#include <format_protocol.h>
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_packet.h"

unsigned int last_dtg_packet_id = 0;

void build_dtg_report_msg
(char* stream, unsigned char event_num)
{
	unsigned short crc16_value;
	time_t c_time = time(NULL);
	unsigned short tmp_word = 0;

	memcpy(stream + 17, &event_num, 1);
	tmp_word = get_dtg_report_period();
	memcpy(stream + 47, &tmp_word, 2);
	tmp_word = get_dtg_create_period();
	memcpy(stream + 51, &tmp_word, 2);
	crc16_value = crc16_get(stream, sizeof(mdt_packet_t) - 3);
	memcpy(stream + 53, &crc16_value, 2);
	memcpy(stream + 58, &c_time, 4);
	memset(stream + 62, 0x20, 4);
}

int g_hsock = 0;

int g_disconnect_to_server()
{
	if(g_hsock > 0)
	{
		close(g_hsock);
		DTG_LOGI("Server close");
		g_hsock = 0;
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

	host_name = get_dtg_server_ip_addr();
	host_port = get_dtg_server_port();

	g_hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(g_hsock == -1)
	{
		err = -1;
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto FINISH1;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		err = -1;
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto FINISH1;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(g_hsock, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		err = -1;
		goto FINISH1;
	}

		return err;

FINISH1:
;
	if(g_hsock > 0) {
		close(g_hsock);
		g_hsock = 0;
	}

	return -1;
	
}

int g_send_to_server(unsigned char *buffer_in, int buffer_len)
{
	int bytecount;
	int err = 0;
	int recv_len = 5;
	int recv_cnt = 0;
	int recved_len = 0;
	struct recv_struct {
		unsigned long packet_id;
		unsigned char ret;
	} recv_buf;

	int sent_len = 0;
	int sending_len = 0;
	int send_retry_cnt = 0;

	memset(&last_dtg_packet_id, 0, 4);
	memcpy(&last_dtg_packet_id, &buffer_in[58], 4);

	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	net_packet_dump("daishin packet dump", buffer_in, buffer_len);
	DTG_LOGI("Server sending.....");
	while(1) {
	bytecount = nettool_send_timedwait(g_hsock, (const char *)buffer_in, buffer_len, 0, 20);
		if(bytecount <= 0)
			send_retry_cnt += 1;
		else {
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

		if(send_retry_cnt > 10) {
			err = NET_SEND_PACKET_ERROR;
			goto FINISH2;
		}
		usleep(1000*200); //300ms
	}

	DTG_LOGI("Daishin Server send success!! : sent length[%d], sending length[%d]", bytecount, buffer_len);

	memset(&recv_buf, 0x00, sizeof(recv_buf));
	while(1) {	
		if((bytecount = recv(g_hsock, &recv_buf, recv_len, 0)) <= 0){
			DTG_LOGD(".");
			recv_cnt++;
			sleep(1);
		} else {
			recv_cnt = 0;
			DTG_LOGD("#");
			recved_len += bytecount;
			if(recved_len == 5){
				DTG_LOGD("Server Recv [%lx][%d]\n", 
					recv_buf.packet_id, recv_buf.ret);
				break;
			}
			else {
				recv_len = (5 - recved_len);
			}
			usleep(1000*200);
		}

		if(recv_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			DTG_LOGE("Server Recv Time Out[%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			goto FINISH2;
		}
		
	}//end of while

	if((recved_len != 5) || (recv_buf.packet_id != last_dtg_packet_id) || (recv_buf.ret != 1)) {
		DTG_LOGE("Server Received Packet Error[%d]", errno);
		DTG_LOGE("=========Server Received Packet=========");
		DTG_LOGE("packet_id [%d] ret [%d]", recv_buf.packet_id, recv_buf.ret);
		DTG_LOGE("last_dtg_packet_id [%d]", last_dtg_packet_id);
		err = NET_RECV_PACKET_ERROR;
	}
FINISH2:
;
	return err;
}

ctrl_strt_t ctrl_buf[CTRL_MAX] = {0, };
int ctrl_index = 0;
int ctrl_end = 0;
int ctrl_cnt = 0;
int ctrl_free = CTRL_MAX;

void build_ctrl_report_msg
(char* stream, unsigned char event_num)
{
	unsigned short crc16_value;
	unsigned short tmp_word = 0;

	memcpy(stream + 17, &event_num, 1);
	tmp_word = get_ctrl_report_period();
	memcpy(stream + 47, &tmp_word, 2);
	tmp_word = get_ctrl_create_period();
	memcpy(stream + 51, &tmp_word, 2);
	crc16_value = crc16_get(stream, sizeof(mdt_packet_t) - 3);
	memcpy(stream + 53, &crc16_value, 2);
}

int encode_ctrl_msg
(char* report_msg, char* tmp_report, int tmp_report_len)
{
	int i;
	int e_count = 0;
	unsigned char ec_value = 0x20;
	unsigned char end_flag = 0x7e;
	unsigned char escape_flag = 0x7d;

	for(i=0; i<tmp_report_len; i++)
	{
		if(tmp_report[i] == end_flag || tmp_report[i] == escape_flag) {
			report_msg[e_count++] = escape_flag;
			report_msg[e_count++] = tmp_report[i] ^ ec_value;
		} else {
			report_msg[e_count++] = tmp_report[i];
		}
	}
	memset(report_msg+e_count, end_flag, 1);
	e_count++;

	return e_count;
}

int save_ctrl_buf
(unsigned char *stream, int stream_size, int event_code)
{
	int offset = 0;
	build_ctrl_report_msg(stream, event_code);
	ctrl_buf[ctrl_end].buf_size = encode_ctrl_msg(&(ctrl_buf[ctrl_end].buf), 
									stream, sizeof(mdt_packet_t) -1);

	if (ctrl_free == 0) {
		ctrl_index++;
		ctrl_end++;
	} else {
		ctrl_end++;
		ctrl_free--;
		ctrl_cnt++;
	}
	if (ctrl_index == CTRL_MAX)
		ctrl_index = 0;
	if (ctrl_end == CTRL_MAX)
		ctrl_end = 0;
	return ctrl_cnt;
}

int clear_ctrl_buf(int clear_index)
{
	int i;
	for (i = 0; i < clear_index; i++) {
		ctrl_index++; 
		ctrl_cnt--;
		ctrl_free++;
		if(ctrl_index == CTRL_MAX)
			ctrl_index = 0;
	}
}

int send_to_ctrl_server(unsigned char *buffer_in, int buffer_len)
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

	host_name = get_ctrl_server_ip_addr();
	host_port = get_ctrl_server_port();

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

	net_packet_dump("daishin packet dump", buffer_in, buffer_len);
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

	DTG_LOGI("Server send success!! : sent length[%d], sending length[%d]", 
			bytecount, buffer_len);

FINISH:
;
	if(hsock != 0) {
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
	if(hsock != 0)
		close(hsock);
	DTG_LOGD("%s: %s -- : err[%d]\n", __FILE__, __func__, err);

	return err;
}

=======
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

#include <format_protocol.h>
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_packet.h"

unsigned int last_dtg_packet_id = 0;

void build_dtg_report_msg
(char* stream, unsigned char event_num)
{
	unsigned short crc16_value;
	time_t c_time = time(NULL);
	unsigned short tmp_word = 0;

	memcpy(stream + 17, &event_num, 1);
	tmp_word = get_dtg_report_period();
	memcpy(stream + 47, &tmp_word, 2);
	tmp_word = get_dtg_create_period();
	memcpy(stream + 51, &tmp_word, 2);
	crc16_value = crc16_get(stream, sizeof(mdt_packet_t) - 3);
	memcpy(stream + 53, &crc16_value, 2);
	memcpy(stream + 58, &c_time, 4);
	memset(stream + 62, 0x20, 4);
}

int g_hsock = 0;

int g_disconnect_to_server()
{
	if(g_hsock > 0)
	{
		close(g_hsock);
		DTG_LOGI("Server close");
		g_hsock = 0;
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

	host_name = get_dtg_server_ip_addr();
	host_port = get_dtg_server_port();

	g_hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(g_hsock == -1)
	{
		err = -1;
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto FINISH1;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		err = -1;
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto FINISH1;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(g_hsock, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		err = -1;
		goto FINISH1;
	}

		return err;

FINISH1:
;
	if(g_hsock > 0) {
		close(g_hsock);
		g_hsock = 0;
	}

	return -1;
	
}

int g_send_to_server(unsigned char *buffer_in, int buffer_len)
{
	int bytecount;
	int err = 0;
	int recv_len = 5;
	int recv_cnt = 0;
	int recved_len = 0;
	struct recv_struct {
		unsigned long packet_id;
		unsigned char ret;
	} recv_buf;

	int sent_len = 0;
	int sending_len = 0;
	int send_retry_cnt = 0;

	memset(&last_dtg_packet_id, 0, 4);
	memcpy(&last_dtg_packet_id, &buffer_in[58], 4);

	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	net_packet_dump("daishin packet dump", buffer_in, buffer_len);
	DTG_LOGI("Server sending.....");
	while(1) {
	bytecount = nettool_send_timedwait(g_hsock, (const char *)buffer_in, buffer_len, 0, 20);
		if(bytecount <= 0)
			send_retry_cnt += 1;
		else {
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

		if(send_retry_cnt > 10) {
			err = NET_SEND_PACKET_ERROR;
			goto FINISH2;
		}
		usleep(1000*200); //300ms
	}

	DTG_LOGI("Daishin Server send success!! : sent length[%d], sending length[%d]", bytecount, buffer_len);

	memset(&recv_buf, 0x00, sizeof(recv_buf));
	while(1) {	
		if((bytecount = recv(g_hsock, &recv_buf, recv_len, 0)) <= 0){
			DTG_LOGD(".");
			recv_cnt++;
			sleep(1);
		} else {
			recv_cnt = 0;
			DTG_LOGD("#");
			recved_len += bytecount;
			if(recved_len == 5){
				DTG_LOGD("Server Recv [%lx][%d]\n", 
					recv_buf.packet_id, recv_buf.ret);
				break;
			}
			else {
				recv_len = (5 - recved_len);
			}
			usleep(1000*200);
		}

		if(recv_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			DTG_LOGE("Server Recv Time Out[%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			goto FINISH2;
		}
		
	}//end of while

	if((recved_len != 5) || (recv_buf.packet_id != last_dtg_packet_id) || (recv_buf.ret != 1)) {
		DTG_LOGE("Server Received Packet Error[%d]", errno);
		DTG_LOGE("=========Server Received Packet=========");
		DTG_LOGE("packet_id [%d] ret [%d]", recv_buf.packet_id, recv_buf.ret);
		DTG_LOGE("last_dtg_packet_id [%d]", last_dtg_packet_id);
		err = NET_RECV_PACKET_ERROR;
	}
FINISH2:
;
	return err;
}

ctrl_strt_t ctrl_buf[CTRL_MAX] = {0, };
int ctrl_index = 0;
int ctrl_end = 0;
int ctrl_cnt = 0;
int ctrl_free = CTRL_MAX;

void build_ctrl_report_msg
(char* stream, unsigned char event_num)
{
	unsigned short crc16_value;
	unsigned short tmp_word = 0;

	memcpy(stream + 17, &event_num, 1);
	tmp_word = get_ctrl_report_period();
	memcpy(stream + 47, &tmp_word, 2);
	tmp_word = get_ctrl_create_period();
	memcpy(stream + 51, &tmp_word, 2);
	crc16_value = crc16_get(stream, sizeof(mdt_packet_t) - 3);
	memcpy(stream + 53, &crc16_value, 2);
}

int encode_ctrl_msg
(char* report_msg, char* tmp_report, int tmp_report_len)
{
	int i;
	int e_count = 0;
	unsigned char ec_value = 0x20;
	unsigned char end_flag = 0x7e;
	unsigned char escape_flag = 0x7d;

	for(i=0; i<tmp_report_len; i++)
	{
		if(tmp_report[i] == end_flag || tmp_report[i] == escape_flag) {
			report_msg[e_count++] = escape_flag;
			report_msg[e_count++] = tmp_report[i] ^ ec_value;
		} else {
			report_msg[e_count++] = tmp_report[i];
		}
	}
	memset(report_msg+e_count, end_flag, 1);
	e_count++;

	return e_count;
}

int save_ctrl_buf
(unsigned char *stream, int stream_size, int event_code)
{
	int offset = 0;
	build_ctrl_report_msg(stream, event_code);
	ctrl_buf[ctrl_end].buf_size = encode_ctrl_msg(&(ctrl_buf[ctrl_end].buf), 
									stream, sizeof(mdt_packet_t) -1);

	if (ctrl_free == 0) {
		ctrl_index++;
		ctrl_end++;
	} else {
		ctrl_end++;
		ctrl_free--;
		ctrl_cnt++;
	}
	if (ctrl_index == CTRL_MAX)
		ctrl_index = 0;
	if (ctrl_end == CTRL_MAX)
		ctrl_end = 0;
	return ctrl_cnt;
}

int clear_ctrl_buf(int clear_index)
{
	int i;
	for (i = 0; i < clear_index; i++) {
		ctrl_index++; 
		ctrl_cnt--;
		ctrl_free++;
		if(ctrl_index == CTRL_MAX)
			ctrl_index = 0;
	}
}

int send_to_ctrl_server(unsigned char *buffer_in, int buffer_len)
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

	host_name = get_ctrl_server_ip_addr();
	host_port = get_ctrl_server_port();

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

	net_packet_dump("daishin packet dump", buffer_in, buffer_len);
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

	DTG_LOGI("Server send success!! : sent length[%d], sending length[%d]", 
			bytecount, buffer_len);

FINISH:
;
	if(hsock != 0) {
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
	if(hsock != 0)
		close(hsock);
	DTG_LOGD("%s: %s -- : err[%d]\n", __FILE__, __func__, err);

	return err;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

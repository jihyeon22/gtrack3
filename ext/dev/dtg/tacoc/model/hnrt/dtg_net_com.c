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
#include <board/board_system.h>

#include <wrapper/dtg_log.h>

#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_packet.h"

//unsigned int last_dtg_packet_id = 0;
unsigned int dtg_sigon_stat = 0;

int disconnect_to_server(int sock_fd)
{
	if(sock_fd > 0)
	{
		close(sock_fd);
		DTG_LOGI("Server close");
	}
	return 0;
}

int connect_to_server(char *host_name, int host_port)
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

	int sock_fd = -1;
	struct sockaddr_in my_addr;
	//int bytecount;
	//int flag;
	//int reuse = 1;
	//int received_max_time_out = 1;

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
	{
		goto ERR_FINISH;
	}

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1)
	{
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);

	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto ERR_FINISH;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	return sock_fd; //success
ERR_FINISH:
	if(sock_fd > 0)
		close(sock_fd);

	return -1;
	
}

/*
void dump_packet(unsigned char *buf, int len, char *msg)
{
	int i;
	int line;
	printf("dump_packet msg========>%s\n", msg);
	for(i = 0, line = 1; i < len; i++, line++)
	{
		printf("%02x ", buf[i]);
		if( (line % 20) == 0)
			printf("\n");
	}
	printf("\n");
}
*/
int send_to_dtg_server(int sock_fd, unsigned char *buffer_in, int buffer_len, char *func, int line, char *msg)
{
	int bytecount;
	int err = 0;
	//int i;

	int sent_len = 0;
	int sending_len = 0;
	int retry_cnt = 0;

	if(sock_fd < 0) {
		DTG_LOGE("%s:%d> %s ----> socket error", func, line, msg);
		return -1;
	}

	//memset(&last_dtg_packet_id, 0, 4);
	//memcpy(&last_dtg_packet_id, &buffer_in[58], 4);

	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	//net_packet_dump("hanuritien packet dump", buffer_in, buffer_len);
	DTG_LOGI("%s:%d> %s", func, line, msg);

	//printf("%s> ==============================> start\n", msg);
	//dump_packet(buffer_in, buffer_len, msg);
	//printf("%s> ==============================> end\n", msg);

	while(1) {
		bytecount = nettool_send_timedwait(sock_fd, (const char *)buffer_in, buffer_len, 0, 20);
			if(bytecount <= 0) {
				DTG_LOGE("nettool_send_timedwait err: %s", strerror(errno));
				retry_cnt += 1;
			}
			else {
				sent_len += bytecount;
				if(buffer_len <= sent_len) {
					break;
				}
			}
		if(retry_cnt > 10) {
			err = NET_SEND_PACKET_ERROR;
			return err;
		}
	}

	
	DTG_LOGI("Hanuriten Server send success!! : sent length[%d], sending length[%d]", sent_len, buffer_len);

	return err;
}

#if defined(BOARD_TL500K)
static int g_server_no_response_cnt = 0;
void add_server_no_ack_count()
{
	g_server_no_response_cnt += 1;
}
int get_server_no_ack_count()
{
	return g_server_no_response_cnt;
}
#endif

resp_pck_t msg_resp[64];
int receive_response(int sock_fd, int dtg_packet)
{
	int count;
	int err = 0;
	int i = 0;
	int retry_cnt = 0;
	int recv_len = 0;

	while(1) {
		//if((count = nettool_recv_timedwait(sock_fd, &msg_resp[i], sizeof(resp_pck_t) - recv_len, 0, 2)) <= 0){
		if((count = nettool_recv_timedwait(sock_fd, &msg_resp[i], sizeof(resp_pck_t) - recv_len, 0, 10)) <= 0){
			DTG_LOGD(".");
			retry_cnt++;
			sleep(1);
		} else {
			retry_cnt = 0;
			DTG_LOGD("#");
			recv_len += count;
			if(recv_len == sizeof(resp_pck_t)){
				DTG_LOGD("Server Recv [%d]\n", msg_resp[i].result);
				i++;
				recv_len = 0;
				break;
			} 
			usleep(1000*200);
		}

		//if(retry_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
		if(retry_cnt > 3) {
			DTG_LOGE("Server Recv Time Out[%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
#if defined(BOARD_TL500K)
			if(dtg_packet == 1)
				g_server_no_response_cnt += 1;
#endif
			return err;
		}
		
	}
	if(msg_resp[i-1].header.msg_len_mark[2] && 0x1)
		err = 1;

#if defined(BOARD_TL500K)
	g_server_no_response_cnt = 0;
#endif
	return err;
}

mdt_strt_t mdt_buf[MDT_MAX] = {0,};
int mdt_index = 0;
int mdt_end = 0;
int mdt_cnt = 0;
int mdt_free = MDT_MAX;
unsigned int mdt_sigon_stat = 0;

void build_mdt_report_msg
(mdt_pck_t* report_msg, msg_mdt_t* stream, unsigned char event_num)
{
	unsigned short crc16_value;

	stream->message_id = 0x64;
	stream->event_code = event_num;

	stream->report_period = get_mdt_report_period();
	stream->create_period = get_mdt_create_period();
#if 0
	crc16_value = crc16_get(stream, sizeof(msg_mdt_t) - 3);
	stream->crc_val = crc16_value;
	stream->endflag = 0x7e;
#endif
	report_msg->header.prtc_id = 0x0003;
	report_msg->header.msg_id = 0x03;
	report_msg->header.svc_id = 0x02;

	int tmp_int = (56 << 1);
	report_msg->header.msg_len_mark[0] = ((0x00ff0000&tmp_int) >> 16);
	report_msg->header.msg_len_mark[1] = ((0x0000ff00&tmp_int) >> 8);
	report_msg->header.msg_len_mark[2] = (0x000000ff&tmp_int);

	memcpy(&report_msg->body, stream, sizeof(msg_mdt_t));

	report_msg->header.prtc_id = htons(report_msg->header.prtc_id);

	report_msg->body.year = htons(report_msg->body.year);
	report_msg->body.speed = htons(report_msg->body.speed);
	report_msg->body.temperature_a = htons(report_msg->body.temperature_a);
	report_msg->body.temperature_b = htons(report_msg->body.temperature_b);
	report_msg->body.temperature_c = htons(report_msg->body.temperature_c);
	report_msg->body.report_period = htons(report_msg->body.report_period);
	report_msg->body.create_period = htons(report_msg->body.create_period);
	report_msg->body.crc_val = htons(report_msg->body.crc_val);
	report_msg->body.gps_x = htonl(report_msg->body.gps_x);
	report_msg->body.gps_y = htonl(report_msg->body.gps_y);
	report_msg->body.on_accumulative_distance = htonl(report_msg->body.on_accumulative_distance);
	crc16_value = crc16_get(&report_msg->body, sizeof(msg_mdt_t) - 3);
	report_msg->body.crc_val = htons(crc16_value);
	report_msg->body.endflag = 0x7e;

}

int save_mdt_buf
(unsigned char *stream, int stream_size, int event_code)
{
	//int offset;
	if (stream_size == 0)
		return -1;
	build_mdt_report_msg(&(mdt_buf[mdt_end].buf), stream, event_code);
	mdt_buf[mdt_end].buf_size = sizeof(mdt_pck_t);

	if (mdt_free == 0) {
		mdt_index++;
		mdt_end++;
	} else {
		mdt_end++;
		mdt_free--;
		mdt_cnt++;
	}
	if (mdt_index == MDT_MAX)
		mdt_index = 0;
	if (mdt_end == MDT_MAX)
		mdt_end = 0;
	return mdt_cnt;
}

int clear_mdt_buf(int clear_index)
{
	int i;
	for (i = 0; i < clear_index; i++) {
		mdt_index++; 
		mdt_cnt--;
		mdt_free++;
		if(mdt_index == MDT_MAX)
			mdt_index = 0;
	}
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
#include <board/board_system.h>

#include <wrapper/dtg_log.h>

#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"
#include "dtg_packet.h"

//unsigned int last_dtg_packet_id = 0;
unsigned int dtg_sigon_stat = 0;

int disconnect_to_server(int sock_fd)
{
	if(sock_fd > 0)
	{
		close(sock_fd);
		DTG_LOGI("Server close");
	}
	return 0;
}

int connect_to_server(char *host_name, int host_port)
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

	int sock_fd = -1;
	struct sockaddr_in my_addr;
	//int bytecount;
	//int flag;
	//int reuse = 1;
	//int received_max_time_out = 1;

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
	{
		goto ERR_FINISH;
	}

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1)
	{
		DTG_LOGE("socket creat err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);        //for linux server
	memset(&(my_addr.sin_zero), 0, 8);

	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto ERR_FINISH;
	}

	DTG_LOGI("Server[%s:%d] connecting....", host_name, host_port);

	if (nettool_connect_timeo(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr), 30) < 0) {
		DTG_LOGE("nettool_connect_timeo err: %s", strerror(errno));
		goto ERR_FINISH;
	}

	return sock_fd; //success
ERR_FINISH:
	if(sock_fd > 0)
		close(sock_fd);

	return -1;
	
}

/*
void dump_packet(unsigned char *buf, int len, char *msg)
{
	int i;
	int line;
	printf("dump_packet msg========>%s\n", msg);
	for(i = 0, line = 1; i < len; i++, line++)
	{
		printf("%02x ", buf[i]);
		if( (line % 20) == 0)
			printf("\n");
	}
	printf("\n");
}
*/
int send_to_dtg_server(int sock_fd, unsigned char *buffer_in, int buffer_len, char *func, int line, char *msg)
{
	int bytecount;
	int err = 0;
	//int i;

	int sent_len = 0;
	int sending_len = 0;
	int retry_cnt = 0;

	if(sock_fd < 0) {
		DTG_LOGE("%s:%d> %s ----> socket error", func, line, msg);
		return -1;
	}

	//memset(&last_dtg_packet_id, 0, 4);
	//memcpy(&last_dtg_packet_id, &buffer_in[58], 4);

	if(buffer_len > 1024) {
		sending_len = 1024;
	}else {
		sending_len = buffer_len;
	}

	//net_packet_dump("hanuritien packet dump", buffer_in, buffer_len);
	DTG_LOGI("%s:%d> %s", func, line, msg);

	//printf("%s> ==============================> start\n", msg);
	//dump_packet(buffer_in, buffer_len, msg);
	//printf("%s> ==============================> end\n", msg);

	while(1) {
		bytecount = nettool_send_timedwait(sock_fd, (const char *)buffer_in, buffer_len, 0, 20);
			if(bytecount <= 0) {
				DTG_LOGE("nettool_send_timedwait err: %s", strerror(errno));
				retry_cnt += 1;
			}
			else {
				sent_len += bytecount;
				if(buffer_len <= sent_len) {
					break;
				}
			}
		if(retry_cnt > 10) {
			err = NET_SEND_PACKET_ERROR;
			return err;
		}
	}

	
	DTG_LOGI("Hanuriten Server send success!! : sent length[%d], sending length[%d]", sent_len, buffer_len);

	return err;
}

#if defined(BOARD_TL500K)
static int g_server_no_response_cnt = 0;
void add_server_no_ack_count()
{
	g_server_no_response_cnt += 1;
}
int get_server_no_ack_count()
{
	return g_server_no_response_cnt;
}
#endif

resp_pck_t msg_resp[64];
int receive_response(int sock_fd, int dtg_packet)
{
	int count;
	int err = 0;
	int i = 0;
	int retry_cnt = 0;
	int recv_len = 0;

	while(1) {
		//if((count = nettool_recv_timedwait(sock_fd, &msg_resp[i], sizeof(resp_pck_t) - recv_len, 0, 2)) <= 0){
		if((count = nettool_recv_timedwait(sock_fd, &msg_resp[i], sizeof(resp_pck_t) - recv_len, 0, 10)) <= 0){
			DTG_LOGD(".");
			retry_cnt++;
			sleep(1);
		} else {
			retry_cnt = 0;
			DTG_LOGD("#");
			recv_len += count;
			if(recv_len == sizeof(resp_pck_t)){
				DTG_LOGD("Server Recv [%d]\n", msg_resp[i].result);
				i++;
				recv_len = 0;
				break;
			} 
			usleep(1000*200);
		}

		//if(retry_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
		if(retry_cnt > 3) {
			DTG_LOGE("Server Recv Time Out[%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
#if defined(BOARD_TL500K)
			if(dtg_packet == 1)
				g_server_no_response_cnt += 1;
#endif
			return err;
		}
		
	}
	if(msg_resp[i-1].header.msg_len_mark[2] && 0x1)
		err = 1;

#if defined(BOARD_TL500K)
	g_server_no_response_cnt = 0;
#endif
	return err;
}

mdt_strt_t mdt_buf[MDT_MAX] = {0,};
int mdt_index = 0;
int mdt_end = 0;
int mdt_cnt = 0;
int mdt_free = MDT_MAX;
unsigned int mdt_sigon_stat = 0;

void build_mdt_report_msg
(mdt_pck_t* report_msg, msg_mdt_t* stream, unsigned char event_num)
{
	unsigned short crc16_value;

	stream->message_id = 0x64;
	stream->event_code = event_num;

	stream->report_period = get_mdt_report_period();
	stream->create_period = get_mdt_create_period();
#if 0
	crc16_value = crc16_get(stream, sizeof(msg_mdt_t) - 3);
	stream->crc_val = crc16_value;
	stream->endflag = 0x7e;
#endif
	report_msg->header.prtc_id = 0x0003;
	report_msg->header.msg_id = 0x03;
	report_msg->header.svc_id = 0x02;

	int tmp_int = (56 << 1);
	report_msg->header.msg_len_mark[0] = ((0x00ff0000&tmp_int) >> 16);
	report_msg->header.msg_len_mark[1] = ((0x0000ff00&tmp_int) >> 8);
	report_msg->header.msg_len_mark[2] = (0x000000ff&tmp_int);

	memcpy(&report_msg->body, stream, sizeof(msg_mdt_t));

	report_msg->header.prtc_id = htons(report_msg->header.prtc_id);

	report_msg->body.year = htons(report_msg->body.year);
	report_msg->body.speed = htons(report_msg->body.speed);
	report_msg->body.temperature_a = htons(report_msg->body.temperature_a);
	report_msg->body.temperature_b = htons(report_msg->body.temperature_b);
	report_msg->body.temperature_c = htons(report_msg->body.temperature_c);
	report_msg->body.report_period = htons(report_msg->body.report_period);
	report_msg->body.create_period = htons(report_msg->body.create_period);
	report_msg->body.crc_val = htons(report_msg->body.crc_val);
	report_msg->body.gps_x = htonl(report_msg->body.gps_x);
	report_msg->body.gps_y = htonl(report_msg->body.gps_y);
	report_msg->body.on_accumulative_distance = htonl(report_msg->body.on_accumulative_distance);
	crc16_value = crc16_get(&report_msg->body, sizeof(msg_mdt_t) - 3);
	report_msg->body.crc_val = htons(crc16_value);
	report_msg->body.endflag = 0x7e;

}

int save_mdt_buf
(unsigned char *stream, int stream_size, int event_code)
{
	//int offset;
	if (stream_size == 0)
		return -1;
	build_mdt_report_msg(&(mdt_buf[mdt_end].buf), stream, event_code);
	mdt_buf[mdt_end].buf_size = sizeof(mdt_pck_t);

	if (mdt_free == 0) {
		mdt_index++;
		mdt_end++;
	} else {
		mdt_end++;
		mdt_free--;
		mdt_cnt++;
	}
	if (mdt_index == MDT_MAX)
		mdt_index = 0;
	if (mdt_end == MDT_MAX)
		mdt_end = 0;
	return mdt_cnt;
}

int clear_mdt_buf(int clear_index)
{
	int i;
	for (i = 0; i < clear_index; i++) {
		mdt_index++; 
		mdt_cnt--;
		mdt_free++;
		if(mdt_index == MDT_MAX)
			mdt_index = 0;
	}
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc

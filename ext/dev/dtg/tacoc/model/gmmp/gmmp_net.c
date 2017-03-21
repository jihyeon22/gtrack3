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

#include <signal.h>
#include <time.h>

#include <pthread.h>

#include "gmmp_manager.h"
#include "gmmp_net.h"

int hsock;

void packet_dump(char *debug_title, int type, void *packet);

int gmmp_session_init()
{
	int ret = 0;
	int err = 0;
	char *ip;
	int port;
	int reuse = 1;
	struct sockaddr_in ServerAddr;
	struct timeval tv_timeo = { 0, 0 };  /* second */
	if(hsock>0)
	{
		close(hsock);
	}
	hsock = socket(PF_INET, SOCK_STREAM, 0);
		if(hsock < 0) {
		printf("socket open error\n");
		return -1;
		goto FINISH;
	}

	tv_timeo.tv_sec = gmmp_get_network_time_out();
	ip = gmmp_get_server_ip();
	port = gmmp_get_server_port();

	printf("network ip : %s\n", ip);
	printf("network port : %d\n", port);
	printf("network timeout : %ld sec\n", tv_timeo.tv_sec);

	setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(setsockopt( hsock, SOL_SOCKET, SO_RCVTIMEO, &tv_timeo, sizeof( tv_timeo )) < 0)
	{
		printf("set rcv_timer error\n");
		err = -2;
		goto FINISH;
	}
	if(setsockopt( hsock, SOL_SOCKET, SO_SNDTIMEO, &tv_timeo, sizeof( tv_timeo )) < 0)
	{
		printf("set snd_timer error\n");
		err = -3;
		goto FINISH;
	}

/*
	// 현재 전송 소켓 버퍼의 크기를 가져온다. - 전송할 때 한번에 보낼 수 있도록 3KB로 수정
	if(getsockopt(hsock, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t *)&rn) < 0) {
		printf("set socket buffer sieze error\n");
		err = -4;
		goto FINISH;
	}else {
		printf("printf current socket buf size : [%d]\n", bsize);
		if(bsize < 3*1024) {
			bsize = 3*1024;
			setsockopt(hsock, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t)rn);

			getsockopt(hsock, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t *)&rn);
			printf("change socket buf size : [%d]\n", bsize);
		}
	}
*/

	ServerAddr.sin_addr.s_addr = inet_addr( ip );
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);

	ret = connect(hsock,(struct sockaddr *) &ServerAddr, sizeof(struct sockaddr_in));
	if(ret < 0) {
		printf("connect error\n");
		err = -5;
		goto FINISH;
	}
	
FINISH:
	return err;
}

int gmmp_session_deinit()
{
	if(hsock > 0){
		if(close(hsock) < 0){
			printf("close error\n");
			return -1;
		}
	}
	return 0;
}
//send & receive packet data into gmmp server
int gmmp_send_to_server(
						unsigned char *send_data, 
						int send_data_len, 
						unsigned char *recv_data, 
						int recv_data_len
						)
{
	int err = 0;

	int byte_count = 0;

	printf("sending packet...\n");
	byte_count = write(hsock, send_data, send_data_len);
	if(byte_count <= 0) {
		printf("packet sending error\n");
		err = -6;
		goto FINISH;
	}
	else if(byte_count != send_data_len) {
		printf("packet sending size diff error\n");
		err = -7;
		goto FINISH;
	}

	printf("wait receiving packet : size[%d]\n", recv_data_len);
	if(recv_data_len == 0)
		goto FINISH;

	byte_count = read(hsock, recv_data, recv_data_len);
	printf("recv byte %d\n",byte_count);
	if(byte_count <= 0) {
		printf("packet receiving error %d\n",byte_count);
		err = -8;
		goto FINISH;
	}
	
	//else if(byte_count != recv_data_len) {
	else if(byte_count < recv_data_len) {
		printf("packet receiving size diff error : size[%d]\n", byte_count);
		err = -9;
		goto FINISH;
	}

FINISH:
	return err;
}

//한번 보낼 때는 total = 1, cur_idx = 0으로 보내면 된다.
void setup_common_header(
						GMMP_COMMON_HEADER *p_gmmp_common, 
						char msg_type, 
						int total, 
						int cur_idx, 
						int transaction_id, 
						char *auth_key
						)
{
	memset(p_gmmp_common, 0x00, sizeof(GMMP_COMMON_HEADER));

	p_gmmp_common->version = 0x20;
	//p_gmmp_common->msg_length = sizeof(GMMP_COMMON_HEADER); 나중에 길이 설정
	p_gmmp_common->msg_type = msg_type;
#ifdef LITTLE_ENDIAN
	p_gmmp_common->time_stamp = htonl(gmmp_get_time_stamp());
	p_gmmp_common->total_count = htons(total);
	p_gmmp_common->current_count = htons(cur_idx);
#else
	p_gmmp_common->time_stamp = gmmp_get_time_stamp();
	p_gmmp_common->total_count = total;
	p_gmmp_common->current_count = cur_idx;
#endif
	strcpy(p_gmmp_common->auth_id, gmmp_get_auth_id());
	if(auth_key != NULL)
		strcpy(p_gmmp_common->auth_key, auth_key);
	//p_gmmp_common->transaction_id = htonl(transaction_id);
}

int send_gw_regi(
				char *domain_id, 
				char *manufacture_id, 
				GMMP_GW_RESPONSE_HDR *p_gw_res)
{
	int ret = 0;
	int err = 0;
	GMMP_GW_REQUEST_HDR gw_req;

	if(domain_id == NULL || manufacture_id == NULL || p_gw_res == NULL) {
		return -1;
	}

	memset(&gw_req, 0x00, sizeof(GMMP_GW_REQUEST_HDR));
	memset(p_gw_res, 0x00, sizeof(GMMP_GW_RESPONSE_HDR));

	setup_common_header(&gw_req.gmmp_common, 
						GMMP_GW_REGISTRATION_REQUEST,
						1, 1, 0, NULL);

	
	strcpy(gw_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_req.manufacture_id, gmmp_get_manufacture_id());
	/*
	strcpy(gw_req.domain_code, domain_id);
	strcpy(gw_req.manufacture_id, manufacture_id);*/

#ifdef LITTLE_ENDIAN
	gw_req.gmmp_common.msg_length = htons(sizeof(GMMP_GW_REQUEST_HDR));
	gw_req.gmmp_common.transaction_id = htonl(gmmp_get_transaction_id());
#else
	gw_req.gmmp_common.msg_length = sizeof(GMMP_GW_REQUEST_HDR);
	gw_req.gmmp_common.transaction_id = gmmp_get_transaction_id();
#endif
	write_ini_tid();

	packet_dump("gmmp_gateway_tx_packet_dump", GW_REG_REQUEST_PACKET_DUMP, &gw_req);

	err = gmmp_session_init();
	if(err < 0) {
		printf("send_gw_regi> gmmp_session_init error[%d]\n", err);
		return -1;
	}
		
	err = gmmp_send_to_server(	(unsigned char *)&gw_req, 
								sizeof(GMMP_GW_REQUEST_HDR),
								(unsigned char *)p_gw_res,
								sizeof(GMMP_GW_RESPONSE_HDR)
							);
	if(err < 0) {
		printf("send_gw_regi> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_gw_regi> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}

	packet_dump("gmmp_gateway_rx_packet_dump", GW_REG_RESPONSE_PACKET_DUMP, p_gw_res);
	return ret;
}

int send_gw_profile(char *gw_id, char *dev_id, char *auth_key, GMMP_PROFILE_RESPONSE_HDR *p_gw_profile_res)
{

	int ret = 0;
	int err = 0;
	GMMP_PROFILE_REQUEST_HDR gw_profile_req;

	memset(&gw_profile_req, 0x00, sizeof(GMMP_PROFILE_REQUEST_HDR));
	memset(p_gw_profile_res, 0x00, sizeof(GMMP_PROFILE_RESPONSE_HDR));

	setup_common_header(&gw_profile_req.gmmp_common, 
						GMMP_PROFILE_REQUEST,
						1, 1, 0, gmmp_get_auth_key());

	strcpy(gw_profile_req.domain_code, gmmp_get_domain_id());
	
	strcpy(gw_profile_req.gw_id, gmmp_get_gw_id());
	//don't need dev_id
	//strcpy(gw_profile_req.device_id, gmmp_get_dev_id());
		
#ifdef LITTLE_ENDIAN
	gw_profile_req.gmmp_common.msg_length = htons(sizeof(GMMP_PROFILE_REQUEST_HDR));
	gw_profile_req.gmmp_common.transaction_id = htonl(gmmp_get_transaction_id());
#else
	gw_profile_req.gmmp_common.msg_length = sizeof(GMMP_PROFILE_REQUEST_HDR);
	gw_profile_req.gmmp_common.transaction_id = gmmp_get_transaction_id();
#endif
	write_ini_tid();

	packet_dump("gmmp_gw_profile_tx_packet_dump", GW_PROFILE_REQUEST_PACKET_DUMP, &gw_profile_req);

	err = gmmp_session_init();
	if(err < 0) {
		printf("send_gw_profile> gmmp_session_init error[%d]\n", err);
		return -1;
	}
	
	err = gmmp_send_to_server(	(unsigned char *)&gw_profile_req, 
								sizeof(GMMP_PROFILE_REQUEST_HDR),
								(unsigned char *)p_gw_profile_res,
								sizeof(GMMP_PROFILE_RESPONSE_HDR)
							);
	if(err < 0) {
		printf("send_gw_profile> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else{
		write_ini_tid();
	}
	
	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_gw_profile> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}

	packet_dump("gmmp_gw_profile_rx_packet_dump", GW_PROFILE_RESPONSE_PACKET_DUMP, p_gw_profile_res);

	return ret;
}

int send_delivery_packet(char report_type,char media_type,unsigned char *data,int data_len)
{
	int ret = 0;
	int err = 0;
	int packet_length;
	int total_count,cur_count;
	unsigned int tid;
	GMMP_DELIVERY_REQUEST_HDR gw_delivery_req;
	GMMP_DELIVERY_RESPONSE_HDR gw_delivery_res;
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_delivery_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}
	
	tid=gmmp_get_transaction_id();
	write_ini_tid();
	
	total_count = data_len/2048;
	if(data_len%2048){
		total_count++;
	}
	
	for(cur_count=1;cur_count<=total_count;cur_count++){
		memset(&gw_delivery_req, 0x00, sizeof(GMMP_DELIVERY_REQUEST_HDR));
		memset(&gw_delivery_res, 0x00, sizeof(GMMP_DELIVERY_RESPONSE_HDR));
		if(cur_count<total_count){
			memcpy(gw_delivery_req.message_body,data+(2048*(cur_count-1)),2048);
			packet_length = sizeof(GMMP_DELIVERY_REQUEST_HDR);
		}
		else{
			memcpy(gw_delivery_req.message_body,data+(2048*(cur_count-1)),data_len - (2048*(cur_count-1)) );
			packet_length = sizeof(GMMP_DELIVERY_REQUEST_HDR) - ( 2048 - (data_len - (2048*(cur_count-1))) );
		}
		setup_common_header(&gw_delivery_req.gmmp_common,
							GMMP_PACKET_DELIVERY_REQUEST,
							total_count, cur_count, 0, gmmp_get_auth_key());
		strcpy(gw_delivery_req.domain_code, gmmp_get_domain_id());
		strcpy(gw_delivery_req.gw_id, gmmp_get_gw_id());
	
		gw_delivery_req.report_type=report_type;
		gw_delivery_req.media_type=media_type;
#ifdef LITTLE_ENDIAN
		gw_delivery_req.gmmp_common.msg_length = htons(packet_length);
		gw_delivery_req.gmmp_common.transaction_id = htonl(tid);
#else
		gw_delivery_req.gmmp_common.msg_length = packet_length;
		gw_delivery_req.gmmp_common.transaction_id = tid;
#endif
		packet_dump("gmmp_gw_delivery_tx_packet_dump", GW_DELIVERY_REQUEST_PACKET_DUMP, &gw_delivery_req);

		err = gmmp_send_to_server(	(unsigned char *)&gw_delivery_req,
								packet_length,
								(unsigned char *)&gw_delivery_res,
								sizeof(GMMP_DELIVERY_RESPONSE_HDR)-4 //offset time value is optional.
								);
		if(err < 0) {
			printf("send_delivery_packet> gmmp_send_to_server error[%d]\n", err);
			ret = -1;
		}
		packet_dump("gmmp_gw_delivery_rx_packet_dump", GW_DELIVERY_RESPONSE_PACKET_DUMP, &gw_delivery_res);
	}
	
	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_delivery_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_control_response(int transaction_id,unsigned char control_type)
{
	int ret = 0;
	int err = 0;
	GMMP_CONTROL_RESPONSE_HDR gw_control_res;
	memset(&gw_control_res, 0x00, sizeof(GMMP_CONTROL_RESPONSE_HDR));
		
	setup_common_header(&(gw_control_res.gmmp_common),
						GMMP_CONTROL_RESPONSE,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_control_res.domain_code, gmmp_get_domain_id());
	strcpy(gw_control_res.gw_id, gmmp_get_gw_id());
	
#ifdef LITTLE_ENDIAN
	gw_control_res.gmmp_common.msg_length = htons(sizeof(GMMP_CONTROL_RESPONSE_HDR));
	gw_control_res.gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_control_res.gmmp_common.msg_length = sizeof(GMMP_CONTROL_RESPONSE_HDR);
	gw_control_res.gmmp_common.transaction_id = transaction_id;
#endif

	gw_control_res.control_type = control_type;
	packet_dump("gmmp_gw_contrl_response_packet_dump", GW_CONTROL_RESPONSE_PACKET_DUMP, &gw_control_res);

	err = gmmp_session_init();
	if(err < 0) {
		printf("send_control_response> gmmp_session_init error[%d]\n", err);
		return -1;
	}
	
	err = gmmp_send_to_server(	(unsigned char *)&gw_control_res,
							sizeof(GMMP_CONTROL_RESPONSE_HDR),
							NULL,
							0
							);
	if(err < 0) {
		printf("send_control_response_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	
	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_control_response> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}	
	
	return ret;
}

int send_ftp_info_request(unsigned char control_type,int transaction_id,GMMP_FTP_INFO_RESPONSE_HDR *gw_ftp_res)
{
	int ret = 0;
	int err = 0;
	GMMP_FTP_INFO_REQUEST_HDR gw_ftp_req;
		
	setup_common_header(&(gw_ftp_req.gmmp_common),
						GMMP_FTP_INFO_REQUEST,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_ftp_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_ftp_req.gw_id, gmmp_get_gw_id());
	
//request에서 받은 transaction_id를 그대로 사용하기 때문에 htonl을 거꾸로 써줘야한다. 추후 확인 요함.
#ifdef LITTLE_ENDIAN
	gw_ftp_req.gmmp_common.msg_length = htons(sizeof(GMMP_FTP_INFO_REQUEST_HDR));
	gw_ftp_req.gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_ftp_req.gmmp_common.msg_length = sizeof(GMMP_FTP_INFO_REQUEST_HDR);
	gw_ftp_req.gmmp_common.transaction_id = transaction_id;
#endif
	gw_ftp_req.control_type=control_type;
	packet_dump("gmmp_gw_ftp_request_packet_dump", GW_FTP_REQUEST_PACKET_DUMP, &gw_ftp_req);
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_ftp_request_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}

	err = gmmp_send_to_server(	(unsigned char *)&gw_ftp_req,
							sizeof(GMMP_FTP_INFO_REQUEST_HDR),
							(unsigned char *)gw_ftp_res,
							sizeof(GMMP_FTP_INFO_RESPONSE_HDR)
							);
	if(err < 0) {
		printf("send_ftp_request_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else{
		packet_dump("gmmp_gw_ftp_response_packet_dump", GW_FTP_RESPONSE_PACKET_DUMP, gw_ftp_res);
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_ftp_request_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_long_sentence_request(char control_type,int transaction_id,GMMP_LONG_SENTENCE_RESPONSE_HDR *gw_long_res)
{
	int ret = 0;
	int err = 0;
	GMMP_LONG_SENTENCE_REQUEST_HDR gw_long_req;
	
	setup_common_header(&(gw_long_req.gmmp_common),
						GMMP_LONG_SENTENCE_REQUEST,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_long_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_long_req.gw_id, gmmp_get_gw_id());
	
//request에서 받은 transaction_id를 그대로 사용하기 때문에 htonl을 거꾸로 써줘야한다. 추후 확인 요함.
#ifdef LITTLE_ENDIAN
	gw_long_req.gmmp_common.msg_length = htons(sizeof(GMMP_LONG_SENTENCE_REQUEST_HDR));
	gw_long_req.gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_long_req.gmmp_common.msg_length = sizeof(GMMP_LONG_SENTENCE_REQUEST_HDR);
	gw_long_req.gmmp_common.transaction_id = transaction_id;
#endif

	gw_long_req.control_type = control_type;

	packet_dump("gmmp_gw_long_request_packet_dump", GW_LONG_SENTENCE_REQUEST_PACKET_DUMP, &gw_long_req);
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_long_request_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}	

	err = gmmp_send_to_server(	(unsigned char *)&gw_long_req,
							sizeof(GMMP_LONG_SENTENCE_REQUEST_HDR),
							(unsigned char *)gw_long_res,
							sizeof(GMMP_LONG_SENTENCE_RESPONSE_HDR)
							);
	
	if(err < 0) {
		printf("send_long_request_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_long_request_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_control_notification(char control_type,int transaction_id,int message_size,int result,
GMMP_CONTROL_NOTIFICATION_HDR *gw_control_noti,
GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR *gw_control_noti_res)
{
	int ret = 0;
	int err = 0;
	int length;
	
	setup_common_header(&(gw_control_noti->gmmp_common),
						GMMP_CONTROL_NOTIFICATION,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_control_noti->domain_code, gmmp_get_domain_id());
	strcpy(gw_control_noti->gw_id, gmmp_get_gw_id());
	
	length = sizeof(GMMP_CONTROL_NOTIFICATION_HDR) - (2048 - message_size);
#ifdef LITTLE_ENDIAN
	gw_control_noti->gmmp_common.msg_length = htons(length);
	gw_control_noti->gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_control_noti->gmmp_common.msg_length = length;
	gw_control_noti->gmmp_common.transaction_id = transaction_id;
#endif

	gw_control_noti->control_type = control_type;

	packet_dump("gmmp_gw_control_notification_packet_dump", GW_CONTROL_NOTIFICATION_PACKET_DUMP, gw_control_noti);
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_control_notification_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}

	err = gmmp_send_to_server(	(unsigned char *)gw_control_noti,
							length,
							(unsigned char *)gw_control_noti_res,
							sizeof(GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR)
							);
	
	if(err < 0) {
		printf("send_control_notification_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else{
		packet_dump("gmmp_gw_control_notification_response_packet_dump", GW_CONTROL_NOTIFICATION_RESPONSE_PACKET_DUMP, gw_control_noti_res);
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_control_notification_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_remote_request(int transaction_id,GMMP_REMOTE_INFO_REQUEST_HDR *gw_remote_res)
{
	int ret = 0;
	int err = 0;
	GMMP_REMOTE_INFO_REQUEST_HDR gw_remote_req;
	
	setup_common_header(&(gw_remote_req.gmmp_common),
						GMMP_REMOTE_INFO_REQUEST,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_remote_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_remote_req.gw_id, gmmp_get_gw_id());
	
//request에서 받은 transaction_id를 그대로 사용하기 때문에 htonl을 거꾸로 써줘야한다. 추후 확인 요함.
#ifdef LITTLE_ENDIAN
	gw_remote_req.gmmp_common.msg_length = htons(sizeof(GMMP_LONG_SENTENCE_REQUEST_HDR));
	gw_remote_req.gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_remote_req.gmmp_common.msg_length = sizeof(GMMP_LONG_SENTENCE_REQUEST_HDR);
	gw_remote_req.gmmp_common.transaction_id = transaction_id;
#endif

	gw_remote_req.control_type = 0x11;

	packet_dump("gmmp_gw_remote_request_packet_dump", GW_REMOTE_REQUEST_PACKET_DUMP, &gw_remote_req);
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_long_request_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}	

	err = gmmp_send_to_server(	(unsigned char *)&gw_remote_req,
							sizeof(GMMP_REMOTE_INFO_REQUEST_HDR),
							(unsigned char *)gw_remote_res,
							sizeof(GMMP_REMOTE_INFO_RESPONSE_HDR)
							);
	
	if(err < 0) {
		printf("send_long_request_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else {
		packet_dump("gmmp_gw_remote_request_packet_dump", GW_REMOTE_RESPONSE_PACKET_DUMP, &gw_remote_req);
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_long_request_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_heartbeat(GMMP_HEARTBEAT_RESPONSE_HDR *gw_heartbeat_res)
{
	int ret = 0;
	int err = 0;
	GMMP_HEARTBEAT_REQUEST_HDR gw_heartbeat_req;
	strcpy(gw_heartbeat_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_heartbeat_req.gw_id, gmmp_get_gw_id());

	setup_common_header(&(gw_heartbeat_req.gmmp_common),
					GMMP_HEARTBEAT_REQUEST,
					1, 1, 0, gmmp_get_auth_key());
#ifdef LITTLE_ENDIAN
	gw_heartbeat_req.gmmp_common.msg_length = htons(sizeof(GMMP_HEARTBEAT_REQUEST_HDR));
	gw_heartbeat_req.gmmp_common.transaction_id = htonl(gmmp_get_transaction_id());
#else
	gw_heartbeat_req.gmmp_common.msg_length = sizeof(GMMP_HEARTBEAT_REQUEST_HDR);
	gw_heartbeat_req.gmmp_common.transaction_id = gmmp_get_transaction_id();
#endif
	write_ini_tid();

	packet_dump("gmmp_gw_heartbeat_request_packet_dump", GW_HEARTBEAT_REQUEST_PACKET_DUMP, &gw_heartbeat_req);

	err = gmmp_session_init();
	if(err < 0) {
		printf("send_heartbeat> gmmp_session_init error[%d]\n", err);
		return -1;
	}

	err = gmmp_send_to_server(	(unsigned char *)&gw_heartbeat_req,
							sizeof(GMMP_HEARTBEAT_REQUEST_HDR),
							(unsigned char *)gw_heartbeat_res,
							sizeof(GMMP_HEARTBEAT_RESPONSE_HDR)
							);
	
	if(err < 0) {
		printf("send_control_notification_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else{
		packet_dump("gmmp_gw_heartbeat_response_packet_dump", GW_HEARTBEAT_RESPONSE_PACKET_DUMP, gw_heartbeat_res);
	}
	
	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_delivery_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_gw_deregi(GMMP_DEREGISTRATION_RESPONSE_HDR *gw_deregi_res){
	int ret = 0;
	int err = 0;
	GMMP_DEREGISTRATION_REQUEST_HDR gw_deregi_req;

	strcpy(gw_deregi_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_deregi_req.gw_id, gmmp_get_gw_id());

	setup_common_header(&(gw_deregi_req.gmmp_common),
					GMMP_GW_DE_REGISTRATION_REQUEST,
					1, 1, 0, gmmp_get_auth_key());
#ifdef LITTLE_ENDIAN
	gw_deregi_req.gmmp_common.msg_length = htons(sizeof(GMMP_HEARTBEAT_REQUEST_HDR));
	gw_deregi_req.gmmp_common.transaction_id = htonl(gmmp_get_transaction_id());
#else
	gw_deregi_req.gmmp_common.msg_length = sizeof(GMMP_HEARTBEAT_REQUEST_HDR);
	gw_deregi_req.gmmp_common.transaction_id = gmmp_get_transaction_id();
#endif
	write_ini_tid();

	packet_dump("gmmp_gw_deregi_request_packet_dump", GW_DEREG_REQUEST_PACKET_DUMP, &gw_deregi_req);

	err = gmmp_session_init();
	if(err < 0) {
		printf("send_deregi> gmmp_session_init error[%d]\n", err);
		return -1;
	}

	err = gmmp_send_to_server(	(unsigned char *)&gw_deregi_req,
							sizeof(GMMP_DEREGISTRATION_REQUEST_HDR),
							(unsigned char *)gw_deregi_res,
							sizeof(GMMP_DEREGISTRATION_RESPONSE_HDR)
							);
	
	if(err < 0) {
		printf("send_deregi> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}
	else{
		packet_dump("gmmp_gw_deregi_response_packet_dump", GW_DEREG_RESPONSE_PACKET_DUMP, gw_deregi_res);
	}
	
	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_deregi> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}

int send_remote_info_request(int transaction_id,GMMP_REMOTE_INFO_RESPONSE_HDR*gw_remote_res)
{
	int ret = 0;
	int err = 0;
	GMMP_REMOTE_INFO_REQUEST_HDR gw_remote_req;
		
	setup_common_header(&(gw_remote_req.gmmp_common),
						GMMP_REMOTE_INFO_REQUEST,
						1, 1, 0, gmmp_get_auth_key());
	
	strcpy(gw_remote_req.domain_code, gmmp_get_domain_id());
	strcpy(gw_remote_req.gw_id, gmmp_get_gw_id());
	
//request에서 받은 transaction_id를 그대로 사용하기 때문에 htonl을 거꾸로 써줘야한다. 추후 확인 요함.
#ifdef LITTLE_ENDIAN
	gw_remote_req.gmmp_common.msg_length = htons(sizeof(GMMP_REMOTE_INFO_REQUEST_HDR));
	gw_remote_req.gmmp_common.transaction_id = htonl(transaction_id);
#else
	gw_remote_req.gmmp_common.msg_length = sizeof(GMMP_REMOTE_INFO_REQUEST_HDR);
	gw_remote_req.gmmp_common.transaction_id = transaction_id;
#endif

	packet_dump("gmmp_gw_remote_request_packet_dump", GW_REMOTE_REQUEST_PACKET_DUMP, &gw_remote_req);
	
	err = gmmp_session_init();
	if(err < 0) {
		printf("send_remote_request_packet> gmmp_session_init error[%d]\n", err);
		return -1;
	}

	err = gmmp_send_to_server(	(unsigned char *)&gw_remote_req,
							sizeof(GMMP_FTP_INFO_REQUEST_HDR),
							(unsigned char *)gw_remote_res,
							sizeof(GMMP_FTP_INFO_RESPONSE_HDR)
							);
	if(err < 0) {
		printf("send_remote_request_packet> gmmp_send_to_server error[%d]\n", err);
		ret = -1;
	}

	err = gmmp_session_deinit();
	if(err < 0) {
		printf("send_remote_request_packet> gmmp_session_deinit error[%d]\n", err);
		return -1;
	}
	
	return ret;
}


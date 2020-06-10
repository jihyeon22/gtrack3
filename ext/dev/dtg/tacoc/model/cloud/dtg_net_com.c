#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <util/crc16.h>
#include <board/board_system.h>
#include <api/update.h>
#include <wrapper/dtg_atcmd.h>

#include <wrapper/dtg_log.h>

#include "dtg_type.h"
#include "dtg_packet.h"
#include "dtg_debug.h"
#include "dtg_utill.h"
#include "dtg_data_manage.h"

int setup_first_packet(PACKET_DATA *p_packet, unsigned int type, int org_len, int compress_len, unsigned char *data, int data_len)
{
	unsigned char *p_crc_data;

	int packet_len = 0;
	p_packet->hdr.package_version_major = get_package_major_version();
	p_packet->hdr.package_version_minor = get_package_minor_version();
	packet_len += sizeof(p_packet->hdr.package_version_major) + sizeof(p_packet->hdr.package_version_minor); //major(1), minor(1)

	p_packet->hdr.msg_type = type;
	packet_len += sizeof(p_packet->hdr.msg_type);

	p_packet->hdr.device_type = get_package_type();		// Package TYPE
	packet_len += sizeof(p_packet->hdr.device_type);		// device type
	
	p_packet->hdr.packet_num = 0;
	packet_len += sizeof(p_packet->hdr.packet_num);
	p_packet->hdr.origin_length = org_len;
	packet_len += sizeof(p_packet->hdr.origin_length);
	p_packet->hdr.compress_length = compress_len;
	packet_len += sizeof(p_packet->hdr.compress_length);

	strcpy(p_packet->hdr.phone_num, atcmd_get_phonenum());
	packet_len += sizeof(p_packet->hdr.phone_num);

	//PACKET_MSG_TRACE("p_packet->sent_packet_size : [%d]\n", packet_len);

	if(data != NULL || data_len != 0) {
		memcpy(p_packet->p_data, data, data_len);
		packet_len += data_len;
	}

	//PACKET_MSG_TRACE("p_packet->sent_packet_size : [%d]\n", packet_len);
	packet_len += (sizeof(p_packet->hdr.sent_packet_size) + sizeof(p_packet->hdr.crc16)); //sent packet size(4) + crc16(2)
	p_packet->hdr.sent_packet_size = packet_len;

	p_crc_data = (unsigned char *)p_packet;
	crc16_get(NULL, 0);
	p_packet->hdr.crc16 = crc16_get(&p_crc_data[sizeof(p_packet->hdr.crc16)], packet_len-sizeof(p_packet->hdr.crc16));


	//PACKET_MSG_TRACE("p_crc_data = [0x%x][0x%x][0x%x][0x%x]\n", p_crc_data[4], p_crc_data[5], p_crc_data[6], p_crc_data[7]);
	//PACKET_MSG_TRACE("p_crc_data = [0x%x][0x%x][0x%x][0x%x]\n", p_crc_data[(packet_len)-4], p_crc_data[(packet_len)-3], p_crc_data[(packet_len)-2], p_crc_data[(packet_len)-1]);
	//PACKET_MSG_TRACE("packet_len = [%d]\n", packet_len);
	//PACKET_MSG_TRACE("** crc16_value [0x%x]\n", p_packet->hdr.crc16);
	//PACKET_MSG_TRACE("data_len[%d]\n", data_len);

	PACKET_MSG_TRACE("\n\nfirst hdr size : [%d]\n", sizeof(PACKET_DATA_HDR));
	PACKET_MSG_TRACE("first>p_packet->package_version : [%d.%02d]\n", p_packet->hdr.package_version_major, p_packet->hdr.package_version_minor);
	PACKET_MSG_TRACE("first>p_packet->msg_type : [0x%x]\n", p_packet->hdr.msg_type);
	PACKET_MSG_TRACE("first>p_packet->device_type : [0x%x]\n", p_packet->hdr.device_type);
	PACKET_MSG_TRACE("first>p_packet->packet_num : [%d]\n", p_packet->hdr.packet_num);
	PACKET_MSG_TRACE("first>p_packet->origin_length : [%d]\n", p_packet->hdr.origin_length);
	PACKET_MSG_TRACE("first>p_packet->compress_length : [%d]\n", p_packet->hdr.compress_length);
	PACKET_MSG_TRACE("first>p_packet->sent_packet_size : [%d]\n", p_packet->hdr.sent_packet_size);
	PACKET_MSG_TRACE("first> p_packet->crc16 = [0x%x]\n\n", p_packet->hdr.crc16);

	return 0;

}


int setup_next_packet(PACKET_NEXT *p_packet, unsigned int type, int packet_num, unsigned char *data, int data_len)
{
	unsigned char *p_crc_data;
	int packet_len = 0;

	p_packet->hdr.msg_type = type;
	packet_len += sizeof(p_packet->hdr.msg_type);
	p_packet->hdr.packet_num = packet_num;
	packet_len += sizeof(p_packet->hdr.packet_num);

	memset(p_packet->hdr.dummy, 0, sizeof(p_packet->hdr.dummy));
	packet_len += sizeof(p_packet->hdr.dummy);

	if(data != NULL || data_len != 0 ) {
		memcpy(p_packet->p_data, data, data_len);
		packet_len += data_len;
	}

	packet_len += (sizeof(p_packet->hdr.sent_packet_size) + sizeof(p_packet->hdr.crc16)); //sent packet size(4) + crc16(2)
	p_packet->hdr.sent_packet_size = packet_len;

	p_crc_data = (unsigned char *)p_packet;
	
	crc16_get(NULL, 0);
	p_packet->hdr.crc16 = crc16_get(&p_crc_data[sizeof(p_packet->hdr.crc16)], packet_len-sizeof(p_packet->hdr.crc16));

	PACKET_MSG_TRACE("\n\nnext hdr size : [%d]\n", sizeof(PACKET_NEXT_HDR));
	PACKET_MSG_TRACE("next>p_packet->msg_type : [0x%x]\n", p_packet->hdr.msg_type);
	PACKET_MSG_TRACE("next>p_packet->packet_num : [%d]\n", p_packet->hdr.packet_num);
	PACKET_MSG_TRACE("next>p_packet->sent_packet_size : [%d]\n", p_packet->hdr.sent_packet_size);
	PACKET_MSG_TRACE("next>setup_next_packet> p_packet->crc16 = [0x%x]\n\n", p_packet->hdr.crc16);
	return 0;
}

int packet_transfer(int hsock, int type, unsigned char  *buf, int buflen, void *pData)
{
	int rev_cnt = 0;
	int resp_code = RETURN_OK;
	int err = NET_SUCCESS_OK;
	int bytecount = 0;
	int send_retry_cnt = 0;
	int max_retry_count = 0;//5;
	int retry_count = 0;
	UPDATE_INFO_RESPONSE *pPacket_update_info;

#if (1)
	int sflag;
	sflag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, sflag | O_NONBLOCK );
#endif

	if(type & MSG_TYPE_UPDATE_INFO_REQUEST)
		pPacket_update_info = (UPDATE_INFO_RESPONSE *)pData;


	net_packet_dump("cloud_reg packet dump", buf, buflen);
SEND_AGAIN_DATA:
	if( (bytecount=send(hsock, buf, buflen,0)) <= 0){
		send_retry_cnt += 1;
		if(send_retry_cnt >= SEND_MAX_RETRY_COUNT) {
			DEBUG_ERROR("Error sending data %d\n", errno);
			return NET_SEND_PACKET_ERROR;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != buflen) {
		send_retry_cnt += 1;
		if(send_retry_cnt >= SEND_MAX_RETRY_COUNT) {
			DEBUG_ERROR("Error send bytes is not same sent bytes : [%d]\n", errno);
			return NET_SEND_PACKET_ERROR; 
		}
		goto SEND_AGAIN_DATA;
	}

	send_retry_cnt = 0;
	PACKET_MSG_TRACE("Client> Sent bytes %d OK\n", bytecount);

	resp_code = RETURN_OK;
	bytecount = 0;

#if (1)
	int flag;
	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );
#endif

	while(1) {	
		PACKET_MSG_TRACE(".");
		if((bytecount = recv(hsock, &resp_code, 1, 0)) <= 0){
			if(rev_cnt > 5)
				DEBUG_ERROR("packet_transfer> Error receiving data %d\n", errno);
		}
		PACKET_MSG_TRACE("#");
		if(bytecount > 0) {
			DEBUG_INFO("1-0.bytecount[%d], resp_code[%d]\n", bytecount, resp_code);
			if(resp_code != RETURN_OK) {
				if(resp_code == RETURN_UPDATE_MUST_BE)
					err = NET_PACKAGE_UPDATE_NEED;
				else
					err = NET_RECV_PACKET_ERROR;
			}else {
				if(type & MSG_TYPE_UPDATE_INFO_REQUEST) {
retry_update_packet: //jwrho
					if((bytecount = recv(hsock, pPacket_update_info, sizeof(UPDATE_INFO_RESPONSE), 0)) <= 0){
						if(rev_cnt > 5)
							DEBUG_ERROR("packet_transfer> Error receiving data %d\n", errno);
					}
					DEBUG_INFO("1-1.bytecount[%d]\n", bytecount);
					if(bytecount == sizeof(UPDATE_INFO_RESPONSE)) {
						err = NET_SUCCESS_UPDATE_INFO;
					}else {
						err = NET_RECV_PACKET_ERROR;
						//jwrho ++
						rev_cnt++;
						if(rev_cnt > RECEIVED_MAX_TIME_OUT) {
							DEBUG_ERROR("Receive Time out : [%d]\n", errno);
							err = NET_RECV_PACKET_TIME_OUT;
							break;
						}
						sleep(1);
						goto retry_update_packet;
						//jwrho --
					}
				}
				else {
					err = NET_SUCCESS_OK;
				}
			}
			break; //packet received success
		}

		rev_cnt++;
		if(rev_cnt > RECEIVED_MAX_TIME_OUT) {
			DEBUG_ERROR("Receive Time out : [%d]\n", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			break;
		}
		sleep(1);
	}//end of while
#if (1)
	fcntl( hsock, F_SETFL, flag );
#endif

	PACKET_MSG_TRACE("packet_transfer> packet_transfer> return code[%d]..\n", err);
	return err;
	}

int request_data_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData)
{
	DEBUG_FUNC_TRACE("%s ++\n", __func__);

	PACKET_DATA packet_first;
	PACKET_NEXT packet_next;

	int packet_legnth = 0;
	int host_port = 0;
	char *host_name;
	int rev_cnt = 0;
	int i;
	int packet_sent_size = 0;

	struct sockaddr_in my_addr;
	int bytecount;
	int hsock = 0;
	int * p_int;
	int err = NET_SUCCESS_OK;
	char resp_code = 0;
	int connect_cnt = 0;
	int flag;
	int last_packet_timeout = RECEIVED_MAX_TIME_OUT;

	host_name = get_reg_server_ip_addr();
	host_port = get_reg_server_port();

	DEBUG_INFO("host_addr = <%s>\n", host_name);
	DEBUG_INFO("host_port = <%d>\n", host_port);

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		err = NET_SOCKET_OPEN_FAIL;
		DEBUG_ERROR("Error initializing socket %d\n",errno);
		goto FINISH;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);	//for linux server
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = nettool_get_host_name(host_name);
	if(my_addr.sin_addr.s_addr == 0) {
		err = NET_SOCKET_OPEN_FAIL;
		DTG_LOGE("!!!!!host name<%s> Convert Error\n", host_name);
		goto FINISH;
	}

	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

	while(1) {
		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5) {
				DEBUG_ERROR("Error connecting socket %d\n", errno);
			}
		} else {
			DEBUG_INFO("server connect success\n");
			break;
		}

		if(connect_cnt > RECEIVED_MAX_TIME_OUT) {
			err = NET_SERVER_CONNECT_ERROR;
			DEBUG_ERROR("Error connecting socket %d time out\n", errno);
			goto FINISH;
		}
		connect_cnt += 1;
		sleep(1);
	}
	
	PACKET_MSG_TRACE("buffer_len[%d], ONE_PACKET_SIZE[%d]\n", buffer_len, ONE_PACKET_SIZE);
	if(buffer_len <= ONE_PACKET_SIZE) {
		//case 1 : ��ü size�� ONE_PACKET_SIZE���� ���� ����
		PACKET_MSG_TRACE("case 1 send\n");
		setup_first_packet(	&packet_first, 
									type | MSG_FIRST_PACKET_DATA | MSG_FINISH_DATA,
									orgsize,
									buffer_len,
									buffer_in,
									buffer_len
								);
		//err = packet_transfer(hsock, (unsigned char *)&packet_first, packet_first.hdr.sent_packet_size);
		err = packet_transfer(hsock, type, (unsigned char *)&packet_first, packet_first.hdr.sent_packet_size, pData);
		if(err < 0) {
			DEBUG_ERROR("1.packet_transfer > return [%d]\n", err);
			goto FINISH;
		}
	}
	else {
		//case 2 : ��ü size�� ONE_PACKET_SIZE���� ũ�� ������ packet�� ����� ������ ����
		int packet_num = 0;
		int packet_legnth = ONE_PACKET_SIZE;
		int packet_last = 0;
		int msg_type = type | MSG_CONTINUE_DATA;

		PACKET_MSG_TRACE("case 2 send\n");
		
		
		packet_sent_size  = 0;
		while(1) {
			if(packet_num == 0) {
				setup_first_packet(	&packet_first, 
									type | MSG_FIRST_PACKET_DATA | MSG_CONTINUE_DATA,
									orgsize,
									buffer_len,
									buffer_in,
									ONE_PACKET_SIZE
									);
				err = packet_transfer(hsock, type, (unsigned char *)&packet_first, packet_first.hdr.sent_packet_size, pData);
				if(err < 0) {
					DEBUG_ERROR("2.packet_transfer > return [%d]\n", err);
					goto FINISH;
				}
			}
			else {

				if(packet_last)
					msg_type = type | MSG_FINISH_DATA;
				else
					msg_type = type | MSG_CONTINUE_DATA;
					

				setup_next_packet(&packet_next, msg_type, packet_num, &buffer_in[packet_sent_size], packet_legnth);

				err = packet_transfer(hsock, type, (unsigned char *)&packet_next, packet_next.hdr.sent_packet_size, pData);
				if(err < 0) {
					DEBUG_ERROR("3.packet_transfer > return [%d]\n", err);
					goto FINISH;
				}
			}

			if(packet_last) {
				break;
			}


			packet_num += 1;
			packet_sent_size += ONE_PACKET_SIZE;

			if(buffer_len - packet_sent_size <= ONE_PACKET_SIZE) {
				packet_legnth = buffer_len - packet_sent_size;
				packet_last = 1;
			}
		} //end of while
	}//end of else

	

#if (1)
{
	//TODO : last packet�� ���ٸ��� ������ ����
	if(err >= 0) {
		PACKET_MSG_TRACE("WAIT_LAST_PACKET_RECEIVED [%d]\n", err);
		resp_code = 0;
		bytecount = 0;
		rev_cnt = 0;

		//none-blocking mode�� ��ȯ
		flag = fcntl( hsock, F_GETFL, 0 );
		fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

		if(orgsize > 0)
			last_packet_timeout = (orgsize/100)/100 + RECEIVED_MAX_TIME_OUT;

		DEBUG_INFO("send_to_server> last_packet_timeout : [%d]\n", last_packet_timeout);
		while(1) {	
			//recv�� length�� 0���� �ָ� non_blocking �Լ��� �ȴ�.
			DEBUG_INFO(".");

			if((bytecount = recv(hsock, &resp_code, 1, 0))== -1){
				if(rev_cnt > 5)
					DEBUG_ERROR("send_to_server> Net Error receiving data %d\n", errno);
			}
			DEBUG_INFO("#");
			if(bytecount > 0) {
				DEBUG_INFO("3.bytecount[%d], resp_code[%d]\n", bytecount, resp_code);
				if(resp_code != RETURN_OK) {
					if(resp_code == RETURN_UPDATE_MUST_BE)
						err = NET_PACKAGE_UPDATE_NEED;
					else
					err = NET_SEND_PACKET_ERROR;
				}
				else {
					err = NET_SUCCESS_OK;
				}

				break; //received resp_code success
			}

			rev_cnt++;

			//if(rev_cnt > RECEIVED_MAX_TIME_OUT) {
			if(rev_cnt > last_packet_timeout) {
				DEBUG_ERROR("Receive Time out : [%d]sec, err[%d]\n", last_packet_timeout, errno);
				err = NET_RECV_PACKET_TIME_OUT;
				goto FINISH;
			}
			//usleep(300);
			sleep(1);
		}//end of while
	} else {
		DEBUG_ERROR("ERROR> NO WAIT_LAST_PACKET_RECEIVED [%d], err\n");
	}
}
#endif

FINISH:
;
	if(hsock != 0)
		close(hsock);
	DEBUG_FUNC_TRACE("%s -- : err[%d]\n", __func__, err);
	return err;
}

int send_to_reg_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData)
{
	int err;
	UPDATE_INFO_RESPONSE Packet_update_info;
	unsigned char *ptmp;

	err = request_data_to_server(type, buffer_in, buffer_len, orgsize, pData);
	if(err == NET_PACKAGE_UPDATE_NEED) {
		DEBUG_INFO("update info> update need command received...\n");
		sleep(3);

		DEBUG_INFO("update info> update info data size : [%d]\n", sizeof(UPDATE_INFO_RESPONSE));
		memset(&Packet_update_info, 0x00, sizeof(UPDATE_INFO_RESPONSE));
		err = request_data_to_server(MSG_TYPE_UPDATE_INFO_REQUEST, NULL, 0, 0, &Packet_update_info);
		if(err == NET_SUCCESS_UPDATE_INFO) {

			DEBUG_INFO("update info> update_server_ip[%s]\n", Packet_update_info.update_server_ip);
			DEBUG_INFO("update info> update_server_port[%d]\n", Packet_update_info.update_server_port);
			DEBUG_INFO("update info> id[%s]\n", Packet_update_info.id);
			DEBUG_INFO("update info> passwrd[%s]\n", Packet_update_info.passwrd);
			DEBUG_INFO("update info> file_path[%s]\n", Packet_update_info.file_path);
			ptmp = (unsigned char *)&Packet_update_info;
	
			int ret;
			/* Step 1. FTP Download */
			DEBUG_INFO("FTP Download Run..");
			ret = update_ftp_download(Packet_update_info.update_server_ip,
									  Packet_update_info.update_server_port,
									  Packet_update_info.id,
									  Packet_update_info.passwrd,
									  Packet_update_info.file_path);
			if (ret < 0) {
				DEBUG_ERROR("%s", update_strerror(update_errno));
				return -1;
			}

			/* Step 2. uncompress for downloaded file */
			DEBUG_INFO("UPDATE Run..");
			ret = update_cmd(1,"NONE","NONE");
			if (ret < 0) {
				DEBUG_ERROR("%s", update_strerror(update_errno));
				return -1;
			}

			/* Step 3. Reboot */
			do {
				ret = reboot(60);
			} while (ret < 0);

		}
	}

	return err;
}

int send_to_server(unsigned char  *buffer_in, int buffer_len)
{
	DEBUG_FUNC_TRACE("%s ++\n", __func__);

	PACKET_DATA packet_first;
	PACKET_NEXT packet_next;

	int packet_legnth = 0;


	int host_port = 0;
	char* host_name= NULL;
	int rev_cnt = 0;
	int i;
	int packet_sent_size = 0;

	struct sockaddr_in my_addr;
	int bytecount;
	int hsock = 0;
	int * p_int;
	int err = NET_SUCCESS_OK;
	char resp_code = 0;
	int connect_cnt = 0;
	int flag;
	int sent_len = 0;
	int sending_len = 0;
	int send_retry_cnt = 0;

	host_name = get_server_ip_addr();
	host_port = get_server_port();

	DEBUG_INFO("host_addr = <%s>\n", host_name);
	DEBUG_INFO("host_port = <%d>\n", host_port);

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		err = NET_SOCKET_OPEN_FAIL;
		DEBUG_ERROR("Error initializing socket %d\n",errno);
		goto FINISH;
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
		goto FINISH;
	}

	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

	while(1) {
		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5)
				DEBUG_ERROR("Error connecting socket %d\n", errno);
		} else {
			DEBUG_INFO("server connect success\n");
			break;
		}

		if(connect_cnt > RECEIVED_MAX_TIME_OUT) {
			err = NET_SERVER_CONNECT_ERROR;
			DEBUG_ERROR("Error connecting socket %d time out\n", errno);
			goto FINISH;
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
	
	net_packet_dump("cloud packet dump", buffer_in, buffer_len);
	while(1) {
		bytecount = 0;
		if( (bytecount=send(hsock, (const char *)&buffer_in[sent_len], sending_len,0)) <= 0 )
			send_retry_cnt += 1;
		else {
			DEBUG_INFO("Cloud Server sent length[%d]\n", bytecount);
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
			DEBUG_ERROR("Cloud Server Send Packet Time out\n");
			err = NET_SEND_PACKET_ERROR;
			goto FINISH;
		}
		usleep(1000*200); //300ms
	}
	DEBUG_INFO("Cloude sending send_retry_cnt[%d]\n", send_retry_cnt);

FINISH:
;
	if(hsock != 0)
		close(hsock);

	return err;
}

int send_to_summary_server(unsigned char  *buffer_in, int buffer_len)
{
	DEBUG_FUNC_TRACE("%s ++\n", __func__);


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
		DEBUG_ERROR("Summary Error initializing socket %d\n",errno);
		goto FINISH;
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
		goto FINISH;
	}

	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

	while(1) {
		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5)
				DEBUG_ERROR("Summary Error connecting socket %d\n", errno);
		} else {
			DEBUG_INFO("Summary server connect success\n");
			break;
		}

		if(connect_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			err = NET_SERVER_CONNECT_ERROR;
			DEBUG_ERROR("Summary Error connecting socket %d time out\n", errno);
			goto FINISH;
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

	DEBUG_INFO("Summary sending length[%d]\n", buffer_len);
	//sending_len = buffer_len;
	while(1) {
		bytecount = 0;
		if( (bytecount=send(hsock, (const char *)&buffer_in[sent_len], sending_len,0)) <= 0 )
			send_retry_cnt += 1;
		else {
			DEBUG_INFO("Summary sent length[%d]\n", bytecount);
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
			DEBUG_ERROR("Summary Send Packet Time out\n");
			err = NET_SEND_PACKET_ERROR;
			goto FINISH;
		}
		usleep(1000*200); //300ms
	}
	DEBUG_INFO("Summary sending send_retry_cnt[%d]\n", send_retry_cnt);

	recv_len = 4;
	recved_len = 0;
	memset(recv_buf, 0x00, sizeof(recv_buf));
	while(1) {	
		//recv�� length�� 0���� �ָ� non_blocking �Լ��� �ȴ�.
		if((bytecount = recv(hsock, &recv_buf[recved_len], recv_len, 0)) <= 0){
			PACKET_MSG_TRACE(".\n");

			rev_cnt++;
			sleep(1);
		} else {
			rev_cnt = 0;
			PACKET_MSG_TRACE("#\n");

			recved_len += bytecount;
			if(recved_len == 4) {
				DEBUG_INFO("Summary Recv [%c][%c][%c][%c]\n", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);
				break;
			}
			else {
				recv_len = (4 - recved_len);
			}
			usleep(1000*200);
		}

		if(rev_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			DEBUG_ERROR("Summary Recv Time Out[%d]\n", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			goto FINISH;
		}
		
	}//end of while

	if( !((recved_len == 4) && (!memcmp(recv_buf, "TRUE", 4))) ) {
		DEBUG_ERROR("Summary Received Packet Error[%d]\n", errno);
		err = NET_RECV_PACKET_ERROR;
	}

FINISH:
;
	if(hsock != 0)
		close(hsock);
	DEBUG_FUNC_TRACE("%s -- : err[%d]\n", __func__, err);

	return err;
}

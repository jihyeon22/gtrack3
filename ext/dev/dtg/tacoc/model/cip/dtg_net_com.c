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
#include "dtg_net_com.h"

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

	if(data != NULL || data_len != 0) {
		memcpy(p_packet->p_data, data, data_len);
		packet_len += data_len;
	}

	packet_len += (sizeof(p_packet->hdr.sent_packet_size) + sizeof(p_packet->hdr.crc16)); //sent packet size(4) + crc16(2)
	p_packet->hdr.sent_packet_size = packet_len;

	p_crc_data = (unsigned char *)p_packet;
	crc16_get(NULL, 0);
	p_packet->hdr.crc16 = crc16_get(&p_crc_data[sizeof(p_packet->hdr.crc16)], packet_len-sizeof(p_packet->hdr.crc16));

	DTG_LOGD("first hdr size : [%d]", sizeof(PACKET_DATA_HDR));
	DTG_LOGD("first>p_packet->package_version : [%d.%02d]", p_packet->hdr.package_version_major, p_packet->hdr.package_version_minor);
	DTG_LOGD("first>p_packet->msg_type : [0x%x]", p_packet->hdr.msg_type);
	DTG_LOGD("first>p_packet->device_type : [0x%x]", p_packet->hdr.device_type);
	DTG_LOGD("first>p_packet->packet_num : [%d]", p_packet->hdr.packet_num);
	DTG_LOGD("first>p_packet->origin_length : [%d]", p_packet->hdr.origin_length);
	DTG_LOGD("first>p_packet->compress_length : [%d]", p_packet->hdr.compress_length);
	DTG_LOGD("first>p_packet->sent_packet_size : [%d]", p_packet->hdr.sent_packet_size);
	DTG_LOGD("first>p_packet->crc16 = [0x%x]\n", p_packet->hdr.crc16);

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

	DTG_LOGD("next hdr size : [%d]", sizeof(PACKET_NEXT_HDR));
	DTG_LOGD("next>p_packet->msg_type : [0x%x]", p_packet->hdr.msg_type);
	DTG_LOGD("next>p_packet->packet_num : [%d]", p_packet->hdr.packet_num);
	DTG_LOGD("next>p_packet->sent_packet_size : [%d]", p_packet->hdr.sent_packet_size);
	DTG_LOGD("next>setup_next_packet> p_packet->crc16 = [0x%x]\n", p_packet->hdr.crc16);
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

#if 0
	/* Power Source Check */
	if (power_get_power_source() == POWER_SRC_BATTERY && 
		!(type & (MSG_TYPE_REGISTRATION | MSG_TYPE_DE_REGISTRATION))) {
		DTG_LOGE("Packet Transfer Send Loop: DC Line cutted!!");
		return NET_EVENT_POWER_SRC_BATTERY;
	}
#endif
	net_packet_dump("cip packet dump", buf, buflen);
SEND_AGAIN_DATA:
	if( (bytecount=send(hsock, buf, buflen,0)) <= 0){
		send_retry_cnt += 1;
		if(send_retry_cnt >= SEND_MAX_RETRY_COUNT) {
			DTG_LOGE("Error sending data %d", errno);
			return NET_SEND_PACKET_ERROR;
		}
		goto SEND_AGAIN_DATA;
	}
	if(bytecount != buflen) {
		send_retry_cnt += 1;
		if(send_retry_cnt >= SEND_MAX_RETRY_COUNT) {
			DTG_LOGE("Error send bytes is not same sent bytes : [%d]", errno);
			return NET_SEND_PACKET_ERROR; 
		}
		goto SEND_AGAIN_DATA;
	}

	send_retry_cnt = 0;
	DTG_LOGI("Client> Sent bytes %d OK", bytecount);

	resp_code = RETURN_OK;
	bytecount = 0;

#if (1)
	int flag;
	flag = fcntl( hsock, F_GETFL, 0 );
	fcntl( hsock, F_SETFL, flag | O_NONBLOCK );
#endif

	while(1) {
#if 0
		/* Power Source Check */
		if (power_get_power_source() == POWER_SRC_BATTERY && 
			!(type & (MSG_TYPE_REGISTRATION | MSG_TYPE_DE_REGISTRATION))) {
			DTG_LOGE("Packet Transfer Receive Loop: DC Line cutted!!");
			return NET_EVENT_POWER_SRC_BATTERY;
		}
#endif

		DTG_LOGD(".");
		if((bytecount = recv(hsock, &resp_code, 1, 0)) <= 0){
			if(rev_cnt > 5)
				DTG_LOGE("packet_transfer> Error receiving data %d", errno);
		}
		DTG_LOGD("#");
		if(bytecount > 0) {
			DTG_LOGD("1-0.bytecount[%d], resp_code[%d]", bytecount, resp_code);
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
							DTG_LOGE("packet_transfer> Error receiving data %d", errno);
					}
					DTG_LOGD("1-1.bytecount[%d]", bytecount);
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
			DTG_LOGE("Receive Time out : [%d]", errno);
			err = NET_RECV_PACKET_TIME_OUT;
			break;
		}
		sleep(1);
	}//end of while
#if (1)
	fcntl( hsock, F_SETFL, flag );
#endif

	DTG_LOGD("packet_transfer> packet_transfer> return code[%d]..", err);
	return err;
	}

int request_data_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData, int IPOfServer)
{
	DTG_LOGD("%s ++\n", __func__);

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
	int last_packet_timeout = RECEIVED_MAX_TIME_OUT;

	if (IPOfServer == MDS_IP){
		host_name = get_reg_server_ip_addr();
		host_port = get_reg_server_port();
	}
	else{
		host_name = get_server_ip_addr();
		host_port = get_server_port();
	}
	
	DTG_LOGD("host_addr = <%s>", host_name);
	DTG_LOGD("host_port = <%d>", host_port);

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		err = NET_SOCKET_OPEN_FAIL;
		DTG_LOGE("Error initializing socket %d",errno);
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
#if 0
		/* Power Source Check */
		if (power_get_power_source() == POWER_SRC_BATTERY && 
			!(type & (MSG_TYPE_REGISTRATION | MSG_TYPE_DE_REGISTRATION))) {
			DTG_LOGE("Connection Loop: DC Line cutted!!");
			err = NET_EVENT_POWER_SRC_BATTERY;
			goto FINISH;
		}
#endif

		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5) {
				DTG_LOGE("Error connecting socket %d", errno);
			}
		} else {
			DTG_LOGI("server connect success");
			break;
		}

		if(connect_cnt > RECEIVED_MAX_TIME_OUT) {
			err = NET_SERVER_CONNECT_ERROR;
			DTG_LOGE("Error connecting socket %d time out", errno);
			goto FINISH;
		}
		connect_cnt += 1;
		sleep(1);
	}
	
	DTG_LOGD("buffer_len[%d], ONE_PACKET_SIZE[%d]", buffer_len, ONE_PACKET_SIZE);
	if(buffer_len <= ONE_PACKET_SIZE) {
		//case 1 : ��ü size�� ONE_PACKET_SIZE���� ���� ����
		DTG_LOGD("case 1 send");
		setup_first_packet(	&packet_first, 
									type | MSG_FIRST_PACKET_DATA | MSG_FINISH_DATA,
									orgsize,
									buffer_len,
									buffer_in,
									buffer_len
								);
		err = packet_transfer(hsock, type, (unsigned char *)&packet_first, packet_first.hdr.sent_packet_size, pData);
		if(err < 0) {
			DTG_LOGE("1.packet_transfer > return [%d]", err);
			goto FINISH;
		}
	}
	else {
		//case 2 : ��ü size�� ONE_PACKET_SIZE���� ũ�� ������ packet�� ����� ������ ����
		int packet_num = 0;
		int packet_legnth = ONE_PACKET_SIZE;
		int packet_last = 0;
		int msg_type = type | MSG_CONTINUE_DATA;

		DTG_LOGD("case 2 send");
		
		
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
					DTG_LOGE("2.packet_transfer > return [%d]", err);
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
					DTG_LOGE("3.packet_transfer > return [%d]", err);
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
		DTG_LOGI("WAIT_LAST_PACKET_RECEIVED [%d]", err);
		resp_code = 0;
		bytecount = 0;
		rev_cnt = 0;

		//none-blocking mode�� ��ȯ
		flag = fcntl( hsock, F_GETFL, 0 );
		fcntl( hsock, F_SETFL, flag | O_NONBLOCK );

		// CIP SERVER Response Timeout Modify : 20130528 YK
		if(orgsize > 0)
			last_packet_timeout = (int)(orgsize/3000) + RECEIVED_MAX_TIME_OUT;

		DTG_LOGD("send_to_server> last_packet_timeout : [%d]", last_packet_timeout);
		while(1) {	
#if 0
			/* Power Source Check */
			if (power_get_power_source() == POWER_SRC_BATTERY && 
				!(type & (MSG_TYPE_REGISTRATION | MSG_TYPE_DE_REGISTRATION))) {
				DTG_LOGE("Server Response Loop: DC Line cutted!!");
				err = NET_EVENT_POWER_SRC_BATTERY;
				break;
			}
#endif

			//recv�� length�� 0���� �ָ� non_blocking �Լ��� �ȴ�.
			DTG_LOGD(".");

			if((bytecount = recv(hsock, &resp_code, 1, 0))== -1){
				if(rev_cnt > 5)
					DTG_LOGE("send_to_server> Net Error receiving data %d", errno);
			}
			DTG_LOGD("#");
			if(bytecount > 0) {
				DTG_LOGD("3.bytecount[%d], resp_code[%d]", bytecount, resp_code);
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
				DTG_LOGE("Receive Time out : [%d]sec, err[%d]", last_packet_timeout, errno);
				err = NET_RECV_PACKET_TIME_OUT;
				goto FINISH;
			}
			//usleep(300);
			sleep(1);
		}//end of while
	} else {
		DTG_LOGE("ERROR> NO WAIT_LAST_PACKET_RECEIVED [%d], err");
	}
}
#endif
FINISH:
;
	if(hsock > 0)
		close(hsock);
	DTG_LOGD("%s -- : err[%d]\n", __func__, err);
	return err;
}

int send_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData, int IPOfServer)
{
	int err;
	UPDATE_INFO_RESPONSE Packet_update_info;
	unsigned char *ptmp;

	err = request_data_to_server(type, buffer_in, buffer_len, orgsize, pData, IPOfServer);
	if(err == NET_PACKAGE_UPDATE_NEED) {
		DTG_LOGI("update info> update need command received...");
		sleep(3);

		DTG_LOGD("update info> update info data size : [%d]", sizeof(UPDATE_INFO_RESPONSE));
		memset(&Packet_update_info, 0x00, sizeof(UPDATE_INFO_RESPONSE));
		err = request_data_to_server(MSG_TYPE_UPDATE_INFO_REQUEST, NULL, 0, 0, &Packet_update_info, IPOfServer);
		if(err == NET_SUCCESS_UPDATE_INFO) {

			DTG_LOGT("\nupdate info> update_server_ip[%s]", Packet_update_info.update_server_ip);
			DTG_LOGT("update info> update_server_port[%d]", Packet_update_info.update_server_port);
			DTG_LOGT("update info> id[%s]", Packet_update_info.id);
			DTG_LOGT("update info> passwrd[%s]", Packet_update_info.passwrd);
			DTG_LOGT("update info> file_path[%s]\n", Packet_update_info.file_path);
			ptmp = (unsigned char *)&Packet_update_info;
	
			int ret;
			/* Step 1. FTP Download */
			DTG_LOGI("FTP Download Run..");
			ret = update_ftp_download(Packet_update_info.update_server_ip,
									  Packet_update_info.update_server_port,
									  Packet_update_info.id,
									  Packet_update_info.passwrd,
									  Packet_update_info.file_path);
			if (ret < 0) {
				DTG_LOGE("%s", update_strerror(update_errno));
				return -1;
			}
			DTG_LOGI("FTP Download Success..");

			/* Step 2. uncompress for downloaded file */
			DTG_LOGI("UPDATE Run..");
			ret = update_cmd(1,"NONE","NONE");
			if (ret < 0) {
				DTG_LOGE("%s", update_strerror(update_errno));
				return -1;
			}
			DTG_LOGI("UPDATE Success..");

			/* Step 3. Reboot */
			do {
				ret = reboot(60);
			} while (ret < 0);
		}
	}

	return err;
}

int send_to_reg_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData)
{
	send_to_server(type, buffer_in, buffer_len, orgsize, pData, MDS_IP);
}

int send_to_summary_server(unsigned char  *buffer_in, int buffer_len)
{
	DTG_LOGD("%s ++\n", __func__);


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

	DTG_LOGD("Summary host_addr = <%s>", host_name);
	DTG_LOGD("Summary host_port = <%d>", host_port);

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		err = NET_SOCKET_OPEN_FAIL;
		DTG_LOGE("Summary Error initializing socket %d",errno);
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
				DTG_LOGE("Summary Error connecting socket %d", errno);
		} else {
			DTG_LOGD("Summary server connect success");
			break;
		}

		if(connect_cnt > (RECEIVED_MAX_TIME_OUT / 2)) {
			err = NET_SERVER_CONNECT_ERROR;
			DTG_LOGE("Summary Error connecting socket %d time out", errno);
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

	DTG_LOGD("Summary sending length[%d]", buffer_len);
	//sending_len = buffer_len;
	while(1) {
		bytecount = 0;
		if( (bytecount=send(hsock, (const char *)&buffer_in[sent_len], sending_len,0)) <= 0 )
			send_retry_cnt += 1;
		else {
			DTG_LOGD("Summary sent length[%d]", bytecount);
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
			goto FINISH;
		}
		usleep(1000*200); //300ms
	}
	DTG_LOGD("Summary sending send_retry_cnt[%d]", send_retry_cnt);

	recv_len = 4;
	recved_len = 0;
	memset(recv_buf, 0x00, sizeof(recv_buf));
	while(1) {	
		//recv�� length�� 0���� �ָ� non_blocking �Լ��� �ȴ�.
		if((bytecount = recv(hsock, &recv_buf[recved_len], recv_len, 0)) <= 0){
			DTG_LOGD(".\n");

			rev_cnt++;
			sleep(1);
		} else {
			rev_cnt = 0;
			DTG_LOGD("#\n");

			recved_len += bytecount;
			if(recved_len == 4) {
				DTG_LOGD("Summary Recv [%c][%c][%c][%c]", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);
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
			goto FINISH;
		}
		
	}//end of while

	if( !((recved_len == 4) && (!memcmp(recv_buf, "TRUE", 4))) ) {
		DTG_LOGE("Summary Received Packet Error[%d]", errno);
		err = NET_RECV_PACKET_ERROR;
	}

FINISH:
;
	if(hsock > 0)
		close(hsock);
	DTG_LOGD("%s -- : err[%d]\n", __func__, err);

	return err;
}

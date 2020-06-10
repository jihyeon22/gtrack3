
/*
 * dtg_regist_process.c
 *
 *  Created on: 2013. 4. 8.
 *      Author: ongten
 */


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
#include <errno.h>

#include <util/crc16.h>
#include <wrapper/dtg_atcmd.h>
#include <api/update.h>

#include <wrapper/dtg_log.h>
#include "dtg_type.h"
#include "dtg_packet.h"
#include "dtg_debug.h"
#include "dtg_regist_process.h"
#include "gmmp_manager.h"


int setup_first_packet(PACKET_DATA *p_packet, unsigned int type, int org_len, int compress_len, unsigned char *data, int data_len)
{
	unsigned char *p_crc_data;

	int packet_len = 0;
	p_packet->hdr.package_version_major = 1;
	p_packet->hdr.package_version_minor = 0;
	packet_len += sizeof(p_packet->hdr.package_version_major) + sizeof(p_packet->hdr.package_version_minor); //major(1), minor(1)

	p_packet->hdr.msg_type = type;
	packet_len += sizeof(p_packet->hdr.msg_type);

	p_packet->hdr.device_type = 60000;
	packet_len += sizeof(p_packet->hdr.device_type); //device type

	p_packet->hdr.packet_num = 0;
	packet_len += sizeof(p_packet->hdr.packet_num);
	p_packet->hdr.origin_length = org_len;
	packet_len += sizeof(p_packet->hdr.origin_length);
	p_packet->hdr.compress_length = compress_len;
	packet_len += sizeof(p_packet->hdr.compress_length);

	strcpy(p_packet->hdr.phone_num, gmmp_get_auth_id());
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
	PACKET_MSG_TRACE("Client> Sent bytes %d OK", bytecount);

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
				DTG_LOGE("packet_transfer> Error receiving data %d", errno);
		}
		PACKET_MSG_TRACE("#");
		if(bytecount > 0) {
			DTG_LOGD("1-0.bytecount[%d], resp_code[%d]", bytecount, resp_code);
			if(resp_code != RETURN_OK) {
				err = NET_RECV_PACKET_ERROR;
			}else {
				err = NET_SUCCESS_OK;
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

	PACKET_MSG_TRACE("packet_transfer> packet_transfer> return code[%d]..\n", err);
	return err;
}

int request_data_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData)
{
	DTG_LOGD("%s: %s ++", __FILE__, __func__);

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

	host_name = "m2m.mdstec.com";
	host_port = 30007;

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
		if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0 ){
			if( connect_cnt > 5) {
				DTG_LOGE("Error connecting socket %d", errno);
			}
		} else {
			DTG_LOGD("server connect success");
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

	PACKET_MSG_TRACE("buffer_len[%d], ONE_PACKET_SIZE[%d]\n", buffer_len, ONE_PACKET_SIZE);

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
		DTG_LOGE("1.packet_transfer > return [%d]", err);
		goto FINISH;
	}

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

		DTG_LOGD("send_to_server> last_packet_timeout : [%d]", last_packet_timeout);
		while(1) {
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
	if(hsock != 0)
		close(hsock);
	DTG_LOGD("%s: %s -- : err[%d]", __FILE__, __func__, err);
	return err;
}

int send_reg_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData)
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
			//net_packet_dump("packet dump", ptmp, sizeof(UPDATE_INFO_RESPONSE));
	
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

#define REGISTRATION	1
#define DEREGISTRATION	0
#define INVALID_REG		-1
static int regist_flag = 0;

REGIST_DATA regist_data = {
	.xml_starter = "<xml>\n",
	.xml_header_starter = "<H>\n",
	.xml_header_ender = "</H>\n",
	.xml_body_starter = "<B>\n",
	.xml_imei_starter = "<I>",
	.imei = {0,},
	.xml_imei_ender = "</I>\n",
	.xml_dtgenv_starter = "<D>",
	.dtg_env ={0,},
	.xml_dtgenv_ender = "</D>\n",
	.xml_dtgstat_starter = "<DS>",
	.dtg_stat = {'0', 'x', '0', '0', '0', '0'},
	.xml_dtgstat_ender = "</DS>\n",
	.xml_dtgrl_starter = "<RL>",
	.dtg_rl = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'},
	.xml_dtgrl_ender = "</RL>\n",
	.xml_dtgvrn_starter = "<VR>",
	.dtg_vrn = {'#', '#', '#', '#', '0', '0', '#', '#', '0', '0', '0', '0'},
	.xml_dtgvrn_ender = "</VR>\n",
	.xml_body_ender = "</B>\n",
	.xml_ender = "</xml>\n",
};

void send_device_registration()
{
	int ret, fd;
	UPDATE_INFO_RESPONSE Packet_update_info;
	char dtg_status[128] = {0};
	char *ptr;

	if (regist_flag == INVALID_REG) {
		return ;					// 
	}
	
	strncpy(regist_data.imei, atcmd_get_imei(),15);
	if (getenv(DTG_ENV_VAL))
		strncpy(regist_data.dtg_env, getenv(DTG_ENV_VAL),2);
	else
		strncpy(regist_data.dtg_env, "-9",2);

	do {
		fd = open("/var/dtg_status", O_RDONLY , 0644);
		sleep(1);
	} while (fd < 0 && errno != ENOENT);
	if (fd > 0) {
		read(fd, dtg_status, 128);
		ptr = strtok(dtg_status, "/");
		strncpy(regist_data.dtg_stat, ptr, 6);
		ptr = strtok(NULL, "/");
		strncpy(regist_data.dtg_rl, ptr, 10);
		ptr = strtok(NULL, "/");
		if (ptr != NULL)
			strncpy(regist_data.dtg_vrn, ptr, 12);
	}

	ret = send_reg_to_server(MSG_TYPE_REGISTRATION, &regist_data, 
			sizeof(REGIST_DATA), 0, &Packet_update_info);
	if(ret == NET_SUCCESS_OK) {
		regist_flag = REGISTRATION;		
		DTG_LOGI("DEVICE REGISTRATION OK[%d]", ret);
	}else {
		DTG_LOGE("DEVICE REGISTRATION FAILURE[%d]", ret);
	}
}

void send_device_de_registration()
{
	s32 ret;

	if (regist_flag != REGISTRATION) {
		regist_flag = INVALID_REG;
		return ;
	}

	ret = send_reg_to_server(MSG_TYPE_DE_REGISTRATION, NULL, 0, 0, NULL);
	if (ret == NET_SUCCESS_OK) {
		DTG_LOGI("DEVICE DEREGISTRATION OK[%d]", ret);
	} else {
		DTG_LOGE("DEVICE DEREGISTRATION FAILURE[%d]", ret);
	}
}

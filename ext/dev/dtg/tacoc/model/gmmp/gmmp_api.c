#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <assert.h>

#include "gmmp_base64.h"
#include "gmmp_manager.h"
#include "gmmp_utill.h"
#include "gmmp_net.h"
#include "gmmp_dump.h"
#include "gmmp_api.h"

#ifndef TACOC_STANDALONE
#include <api/update.h>
#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>
#include "dtg_packet.h"
#endif
/*
"211.115.15.205",	//ini ����
					21001,				//ini ����
					"gmmpsvc",			//ini ����
					"MF051601",			//ini ����
					"/system/gmmp/m2m_gateway_profile.dat");
*/

#ifndef TACOC_STANDALONE

#endif

pthread_mutex_t mutex;

int GMMP_Module_Init()
{

	int ret = 0;
	int err = 0;

	DEVICE_PROFILE_INFO tmp;
	GMMP_GW_RESPONSE_HDR gw_res;
	GMMP_PROFILE_RESPONSE_HDR gw_profile_res;
	
	pthread_mutex_lock(&mutex);
	if(is_file_exist(gmmp_get_profile_path()) >= 0) {
		printf("%s exist!!!\n", gmmp_get_profile_path());
		if( (err = read_profile_contents(gmmp_get_profile_path())) < 0) {
			remove(gmmp_get_profile_path());
			printf("GMMP_Module_Init> read_profile_contents error : [%d]\n", err);
			ret = -1;
			goto FINISH;
		}
	}
	else {
		//Gateway ���� ��û
		memset(&gw_res, 0x00, sizeof(GMMP_GW_RESPONSE_HDR));
		err = send_gw_regi(gmmp_get_domain_id(),gmmp_get_manufacture_id(),&gw_res);
		if(err < 0) {
			printf("GMMP_Module_Init> send_gw_regi error[%d]\n", err);
			ret = -2;
			goto FINISH;
		}
		else if(gw_res.result_code != 0x00) {
			if(gw_res.result_code == 0x06){
				printf("GMMP_Module_init> This auth_id(%s) is not registered.\n",gmmp_get_auth_id());
			}
			printf("GMMP_Module_Init> gateway regi result code error[%d]\n", gw_res.result_code);
			ret = -3;
			goto FINISH;
		} 
		else {
			printf("GMMP_Module_Init>gateway regi success!!\n");
			gmmp_set_auth_key(gw_res.gmmp_common.auth_key);
			gmmp_set_gw_id(gw_res.gw_id);
		}

		//profile ��û
		memset(&gw_profile_res, 0x00, sizeof(GMMP_PROFILE_RESPONSE_HDR));
		err = send_gw_profile(gw_res.gw_id, NULL, gw_res.gmmp_common.auth_key, &gw_profile_res);
		if(err < 0) {
			printf("GMMP_Module_Init> send_gw_profile error[%d]\n", err);
			ret = -4;
			goto FINISH;
		}
		else if(gw_profile_res.result_code != 0x00) {
			printf("GMMP_Module_Init> gateway regi result code error[%d]\n", gw_profile_res.result_code);
			ret = -5;
			goto FINISH;
		} 
		else 
		{
			memset(&tmp, 0x00, sizeof(DEVICE_PROFILE_INFO));
			strcpy(tmp.AuthKey, gw_profile_res.gmmp_common.auth_key);
			strcpy(tmp.GwId, gw_profile_res.gw_id);
			if(gw_profile_res.device_id != NULL)
				strcpy(tmp.DeviceId, gw_profile_res.device_id);

			tmp.HeartbeatPeriod = htonl(gw_profile_res.heartbeat_period);
			tmp.ReportPeriod = htonl(gw_profile_res.reporting_period);
			tmp.ReportOffset = htonl(gw_profile_res.report_offset);
			tmp.ResponseTimerout = htonl(gw_profile_res.response_timeout);
			strcpy(tmp.Model, gw_profile_res.model);
			strcpy(tmp.FirmwareVer, gw_profile_res.firmware_version);
			strcpy(tmp.SoftwareVer, gw_profile_res.software_version);
			strcpy(tmp.HardwareVer, gw_profile_res.hardware_version);

			//profile ����
			err = write_profile_contents(tmp, gmmp_get_profile_path());//���ο��� profile ���� ���ŵȴ�.
			if(err < 0) {
				ret = -6;
				printf("GMMP_Module_Init> write_profile_contents err[%d]\n", err);
				goto FINISH;
			}
			printf("GMMP_Module_Init>gateway get profile success!!\n");
		}

	}

	printf("++++ gmmp device current profile infomation ++++\n");
	printf("ip : [%s]\n", gmmp_get_server_ip());
	printf("port = [%d]\n", gmmp_get_server_port());
	printf("domain id = [%s]\n", gmmp_get_domain_id());
	printf("manufacture id = [%s]\n", gmmp_get_manufacture_id());
	printf("network time out = [%d]\n", gmmp_get_network_time_out());
	printf("profile path = [%s]\n", gmmp_get_profile_path());
	printf("auth id = [%s]\n", gmmp_get_auth_id());
	printf("auth key = [%s]\n", gmmp_get_auth_key());
	printf("gw id = [%s]\n", gmmp_get_gw_id());
	printf("---- gmmp device current profile infomation ----\n\n\n");


FINISH:
	pthread_mutex_unlock(&mutex);
	return ret;
}

int GMMP_Profile_Request()
{
	DEVICE_PROFILE_INFO tmp;
	GMMP_PROFILE_RESPONSE_HDR gw_profile_res;
	int ret = 0;
	int err = 0;

	pthread_mutex_lock(&mutex);
	memset(&gw_profile_res, 0x00, sizeof(GMMP_PROFILE_RESPONSE_HDR));
	err = send_gw_profile(gmmp_get_gw_id(), NULL, gmmp_get_auth_key(), &gw_profile_res);

	if(err < 0) {
		printf("GMMP_Module_Init> send_gw_profile error[%d]\n", err);
		ret = -1;
		goto FINISH;
	}
	else if(gw_profile_res.result_code != 0x00) {
		printf("GMMP_Module_Init> gateway regi result code error[%d]\n", gw_profile_res.result_code);
		ret = -2;
		goto FINISH;
	} 
	else 
	{
		memset(&tmp, 0x00, sizeof(DEVICE_PROFILE_INFO));
		strcpy(tmp.AuthKey, gw_profile_res.gmmp_common.auth_key);
		strcpy(tmp.GwId, gw_profile_res.gw_id);
		if(gw_profile_res.device_id != NULL)
			strcpy(tmp.DeviceId, gw_profile_res.device_id);

		tmp.HeartbeatPeriod = htonl(gw_profile_res.heartbeat_period);
		tmp.ReportPeriod = htonl(gw_profile_res.reporting_period);
		tmp.ReportOffset = htonl(gw_profile_res.report_offset);
		tmp.ResponseTimerout = htonl(gw_profile_res.response_timeout);
		strcpy(tmp.Model, gw_profile_res.model);
		strcpy(tmp.FirmwareVer, gw_profile_res.firmware_version);
		strcpy(tmp.SoftwareVer, gw_profile_res.software_version);
		strcpy(tmp.HardwareVer, gw_profile_res.hardware_version);

		//profile ����
		err = write_profile_contents(tmp, gmmp_get_profile_path()); //���ο��� profile ���� ���ŵȴ�.
		if(err < 0) {
			ret = -3;
			printf("GMMP_Profile_Request> write_profile_contents err[%d]\n", err);
			goto FINISH;
		}
		printf("GMMP_Profile_Request>gateway get profile success!!\n");
	}
FINISH:
	pthread_mutex_unlock(&mutex);
	return ret;
}

int GMMP_Delivery_Packet(char report_type, char media_type, unsigned char *data, int data_len)
{
	int ret = 0;
	int err = 0;

	pthread_mutex_lock(&mutex);
	err = send_delivery_packet(report_type,media_type,data,data_len);
	if(err < 0) {
		ret = -1;
		printf("GMMP_Delivery_Packet> send delivery packet err[%d]\n", err);
		goto FINISH;
	}
FINISH:
	pthread_mutex_unlock(&mutex);
	return ret;
}

int GMMP_Control_Process(char *packet,unsigned int len,int itype)
{
	int ret = 0;
	int err = 0;
	unsigned int packet_len;
	unsigned char control_type;
	unsigned int transaction_id;
	int message_size = 0;
	
	GMMP_CONTROL_REQUEST_HDR *p_gw_control_req;
	GMMP_CONTROL_REQUEST_SMS_HDR *p_gw_control_sms_req;
	GMMP_CONTROL_NOTIFICATION_HDR gw_control_noti;
	GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR gw_control_noti_res;

	pthread_mutex_lock(&mutex);
	if(itype==SMS){
		p_gw_control_sms_req=(GMMP_CONTROL_REQUEST_SMS_HDR *)base64_decode(packet,len,&packet_len);
		if(p_gw_control_sms_req==NULL || packet_len<sizeof(GMMP_CONTROL_REQUEST_SMS_HDR) )
		{
			ret = -1;
			printf("GMMP_Control_Process> sms data err\n");
			goto FINISH;
		}
		packet_dump("gmmp_gw_contrl_sms_request_packet_dump", GW_CONTROL_REQUEST_SMS_PACKET_DUMP, p_gw_control_sms_req);
		control_type = p_gw_control_sms_req->control_type;
#ifdef LITTLE_ENDIAN
		transaction_id = htonl(p_gw_control_sms_req->omp_transaction_id);
#else
		transaction_id = p_gw_control_sms_req->omp_transaction_id;
#endif
	}
	else{
		p_gw_control_req=(GMMP_CONTROL_REQUEST_HDR *)packet;
		if(p_gw_control_sms_req==NULL || len<sizeof(GMMP_CONTROL_REQUEST_HDR))
		{
			ret = -1;
			printf("GMMP_Control_Process> network packet data err\n");
			goto FINISH;
		}
		packet_dump("gmmp_gw_contrl_request_packet_dump", GW_CONTROL_REQUEST_PACKET_DUMP, &p_gw_control_req);
		control_type = p_gw_control_req->control_type;
#ifdef LITTLE_ENDIAN
		transaction_id = htonl(p_gw_control_req->gmmp_common.transaction_id);
#else
		transaction_id = p_gw_control_req->gmmp_common.transaction_id;
#endif
	}

	err = send_control_response(transaction_id,control_type);
	if(err < 0)
	{
		ret = -1;
		printf("GMMP_Control_Process> send control response err[%d]\n", err);
		goto FINISH;
	}

	memset(&gw_control_noti,0,sizeof(GMMP_CONTROL_NOTIFICATION_HDR));
	
	switch(control_type){

		case CTRL_RESET:
		{
#ifndef TACOC_STANDALONE
			mdmc_api_reset_wrapper();
#endif
			break;
		}
		case CTRL_TURN_OFF:
			gmmp_set_delivery_state(0);
			break;
		case CTRL_REPORT_ON:
			gmmp_set_delivery_state(1);
			set_delivery_timer();
			break;
		case CTRL_REPORT_OFF:
			gmmp_set_delivery_state(0);
			break;
		case CTRL_TIME_SYNC:
		{
			//�ð��� ����
			int time_stamp;
			struct tm *ptm;
			time_t time_t_temp;
			
			if(itype==SMS){
				if(packet_len!=sizeof(GMMP_CONTROL_REQUEST_SMS_HDR)+sizeof(p_gw_control_sms_req->time_stamp))
				{
					printf("GMMP_Control_Process> time stamp data length err\n");
					ret = -1;
					goto FINISH;
				}
				printf("time sync value : %x\n",p_gw_control_sms_req->time_stamp);
				time_stamp = p_gw_control_sms_req->time_stamp;
			}
			else{
				printf("time sync value : %x\n",p_gw_control_req->time_stamp);
				time_stamp = p_gw_control_req->time_stamp;
			}
			
#ifdef LITTLE_ENDIAN
			time_t_temp = htonl(time_stamp);
#else
			time_t_temp = time_stamp;
#endif
			ptm=localtime(&time_t_temp);
			printf("time : %d %d %d %d %d %d\n",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
			
#ifndef TACOC_STANDALONE
#if 0 // TODO: api fix
			timesync_info_t timesync_info;
			timesync_info.opt=MDMC_TIMESYNC_LOCAL_TIME;
			timesync_info.julian_time.year=ptm->tm_year+1900;
			timesync_info.julian_time.month=ptm->tm_mon+1;
			timesync_info.julian_time.day=ptm->tm_mday;
			timesync_info.julian_time.hour=ptm->tm_hour;
			timesync_info.julian_time.minute=ptm->tm_min;
			timesync_info.julian_time.second=ptm->tm_sec;			
			mdmc_device_time_sync_call(&timesync_info,&err,clnt_mdmc);
#endif
			mdmc_api_time_sync_wrapper();
#endif
			break;
		}
		case CTRL_PAUSE:
			//�ܸ� �Ͻ����� - �ֱ⺸���� �ߴ�
			gmmp_set_delivery_state(0);
			break;
		case CTRL_RESTART:
			//�ܸ� ������ - �ֱ⺸���� ����
			gmmp_set_delivery_state(1);
			set_delivery_timer();
			break;
		case CTRL_SIGNAL_POWER_CHECK:
		{
			int num_of_rssi;
#ifndef TACOC_STANDALONE
			//��ȣ����üũ at Ŀ�ǵ� �ǽ�
			num_of_rssi = atcmd_get_rssi();
#else
			num_of_rssi = 100;
#endif
			
#ifdef LITTLE_ENDIAN
			gw_control_noti.signal_power = htonl(num_of_rssi);
#else
			gw_control_noti.signal_power = num_of_rssi;
#endif
			message_size = 4;
			break;
		}
		case CTRL_DIAGNOSTIC:
		{
#ifndef TACOC_STANDALONE
			int check_device;
			check_device =(	MDMC_DIAG_DEVICE(MDMC_DIAG_DIAL_CHECK) | 
							MDMC_DIAG_DEVICE(MDMC_DIAG_NETWORK_CHECK) | 
							MDMC_DIAG_DEVICE(MDMC_DEVICE_CHECK_2) );

			mdmc_device_diag(&check_device, &err, clnt_mdmc);
			printf("GMMP_Control_Process> mdmc_device_diag res[%x]\n", err);
			if( err & MDMC_DIAG_DEVICE(MDMC_DIAG_DIAL_CHECK) &&
				err & MDMC_DIAG_DEVICE(MDMC_DIAG_NETWORK_CHECK) &&
				err & MDMC_DIAG_DEVICE(MDMC_DEVICE_CHECK_2) )
			{
				gw_control_noti.diag_status=0;
			}
			else
			{
				gw_control_noti.diag_status=1;
			}
			message_size = 1;
#else
			gw_control_noti.diag_status = 0;
			message_size = 1;
#endif
			break;
		}
		case CTRL_PROFILE_RESET:
			//������ request ���ǽ�. ������ ���� response �޼����� �������� ������ �Ǵ°�? Ȯ�ο���.
			err=GMMP_Profile_Request();
			if(err!=0)
			{
				goto FINISH;
			}
			break;
		case CTRL_STATUS_CHECK:
			if(gmmp_get_delivery_state())
			{
				gw_control_noti.status_check.on_off=0;
				gw_control_noti.status_check.run_pause=0;
			}
			else
			{
				gw_control_noti.status_check.on_off=1;
				gw_control_noti.status_check.run_pause=1;
			}
			message_size = 2;
			break;
		case CTRL_FW_DOWNLOAD:
		{
			sleep(3);
			
			GMMP_FTP_INFO_RESPONSE_HDR gw_ftp_res;
			char file[255];
			gw_control_noti.response_type1.start_time=gmmp_get_time_stamp();
			err=send_ftp_info_request(control_type,transaction_id,&gw_ftp_res);
			
			//���� �ٿ��ε� �ǽ�
			sprintf(file,"%s/%s",gw_ftp_res.file_path,gw_ftp_res.file_name);

#ifndef TACOC_STANDALONE
			if(err==0){
				err=app_download(gw_ftp_res.ftp_server_ip,
								 gw_ftp_res.ftp_server_port,
								 gw_ftp_res.ftp_server_id,
								 gw_ftp_res.ftp_server_password,
								 file);
			}
#endif
			if(err==-1){
				gw_control_noti.response_type1.result=0x0d;
			}
			else{
				gw_control_noti.response_type1.result=0;
			}
			gw_control_noti.response_type1.end_time=gmmp_get_time_stamp();
			
			message_size = 9;
			break;
		}
		case CTRL_FW_UPGRADE:
			gw_control_noti.response_type1.start_time=gmmp_get_time_stamp();
			//�߿��� ���׷��̵� �ǽ�. ���� �÷ο� Ȯ�� �ʿ�
			gw_control_noti.response_type1.end_time=gmmp_get_time_stamp();
			gw_control_noti.response_type1.result=1;
			message_size = 9;
			break;
		case CTRL_APP_DOWNLOAD:
		{
			sleep(3);
			
			GMMP_FTP_INFO_RESPONSE_HDR gw_ftp_res;
			char file[255];
			gw_control_noti.response_type1.start_time=gmmp_get_time_stamp();
			err=send_ftp_info_request(control_type,transaction_id,&gw_ftp_res);
			
			//���� �ٿ��ε� �ǽ�
			sprintf(file,"%s/%s",gw_ftp_res.file_path,gw_ftp_res.file_name);

#ifndef TACOC_STANDALONE
			if(err==0){
				err=app_download(gw_ftp_res.ftp_server_ip,
								 gw_ftp_res.ftp_server_port,
								 gw_ftp_res.ftp_server_id,
								 gw_ftp_res.ftp_server_password,
								 file);
			}
#endif
			if(err==-1){
				gw_control_noti.response_type1.result=0x0d;
			}
			else{
				gw_control_noti.response_type1.result=0;
			}
			gw_control_noti.response_type1.end_time=gmmp_get_time_stamp();
			
			message_size = 9;
			break;
		}
		case CTRL_APP_UPDATE:
			gw_control_noti.response_type1.start_time=gmmp_get_time_stamp();
			
			//���� ������Ʈ �ǽ�
			if(itype==SMS){
				if(packet_len!=sizeof(GMMP_CONTROL_REQUEST_SMS_HDR)+sizeof(p_gw_control_sms_req->app_update_request))
				{
					printf("GMMP_Control_Process> app update data length err\n");
					ret = -1;
					goto FINISH;
				}
#ifndef TACOC_STANDALONE
				err=app_update(p_gw_control_sms_req->app_update_request.filename);
#endif
			}
			else{
#ifndef TACOC_STANDALONE
				err=app_update(p_gw_control_req->app_update_request.filename);
#endif
			}

			if(err==-1){
				gw_control_noti.response_type1.result=0x0d;
			}
			else{
				gw_control_noti.response_type1.result=0;
			}			
			gw_control_noti.response_type1.end_time=gmmp_get_time_stamp();
			message_size = 9;
			break;
		case CTRL_REMOTE_ACCESS:
		{
			GMMP_REMOTE_INFO_REQUEST_HDR gw_remote_res;
			gw_control_noti.response_type1.start_time=gmmp_get_time_stamp();
			send_remote_request(transaction_id,&gw_remote_res);
			//����Ʈ �׼��� �ǽ�. ���� �÷ο� Ȯ�� �ʿ�
			gw_control_noti.response_type1.end_time=gmmp_get_time_stamp();
			gw_control_noti.response_type1.result=1;
			message_size = 9;
			break;
		}
		default:
			printf("GMMP_Control_Process> Wrong control type err[%d]\n",control_type);
	}
	
	sleep(1);
	err = send_control_notification(control_type, transaction_id,message_size,err,&gw_control_noti,&gw_control_noti_res);
	if(err < 0)
	{
		ret = -1;
		printf("GMMP_Control_Process> send control response err[%d]\n", err);
		goto FINISH;
	}

FINISH:
	if(p_gw_control_sms_req!=NULL){
		free(p_gw_control_sms_req);
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}

#ifndef TACOC_STANDALONE
int tx_sms_to_tacoc(char *sender, char *smsdata){
	if(GMMP_Control_Process(smsdata, strlen(smsdata), SMS))
	{
		return -1;
	}
	return 0;
}

int tx_data_to_tacoc(int type, unsigned char *stream,int len){
	int ret;
	int txsize;
	GMMP_CIP_DATA_PACKET *txdata;
	gzlib_compress(stream, len);
	txsize=get_encode_length()+sizeof(GMMP_CIP_DATA_HEADER);
	txdata=(GMMP_CIP_DATA_PACKET *)malloc(txsize);
	
	setup_cip_header(&(txdata->head), MSG_TYPE_DTG_DATA);

	memcpy(&(txdata->data),get_encode_buffer(),get_encode_length());
	ret=GMMP_Delivery_Packet(COLLECT_DATA,MEDIA_TYPE_MSG_HTTP,(unsigned char *)txdata,txsize);
	set_delivery_timer();

	free(txdata);
	if(ret)
	{
		return -1;
	}
	return 0;
}

int breakdown_report(int integer)
{
	if(integer == 0)
	{
		send_dtg_etc_packet_to_gmmp(MSG_TYPE_DTG_BREAKDOWN);
		printf("breakdw msg: dtg problem occur\n");
	}
	else if(integer == 1)
	{	
		printf("breakdw msg: dtg working\n");
	}
	else
		printf("breakdw msg: incorrect code\n");
	
	return 0;
}

void mdmc_power_off()
{
	send_device_de_registration();
	send_dtg_etc_packet_to_gmmp(MSG_TYPE_DE_REGISTRATION);
}


int get_ip(char *ip)
{
	int fd;
	int ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "ppp0", IFNAMSIZ-1);
	ret=ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	/* display result */
	/* ret==-1 fail, ret==0 success */
	if(ret == 0)
	{
		strncpy(ip,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),16);
	}
	else
	{
		strncpy(ip,"123.123.123.123",16);
	}
	return ret;
}

// return value
// -1 : md5check ������ ����
// 0 : ���ٸ� ���� ó�� ���� �Լ��� ������ ����
int app_download(char *addr, int port, char *id,char *pass,char *file)
{
	return update_ftp_download(addr, port, id, pass, file);
}

// return value
// -1 : argp �Ű������� �߸��� ���� ���� �� ����
// 0 : ���ٸ� ���� ó�� ���� �Լ��� ������ ����
int app_update(char *file_name)
{
    int ret = 0;

    ret = update_cmd(3,file_name,"/system/UPDATE/");

    return ret;
}

void setup_cip_header(GMMP_CIP_DATA_HEADER *packet,int type)
{
	get_ip(packet->ip);
	packet->msg_type=type;
	memcpy(packet->phone_number,gmmp_get_auth_id(),PHONE_NUMBER_LEN);
	printf("[%s] [%04x] [%s]\n",packet->phone_number,type,packet->ip);
}

int send_dtg_etc_packet_to_gmmp(int type)
{
	int ret;
	GMMP_CIP_DATA_HEADER packet;
	setup_cip_header(&packet, type);
	ret=GMMP_Delivery_Packet(COLLECT_DATA,MEDIA_TYPE_MSG_HTTP,(unsigned char *)&packet,sizeof(GMMP_CIP_DATA_HEADER));

	if(ret)
	{
		return -1;
	}
	return 0;
}
#endif

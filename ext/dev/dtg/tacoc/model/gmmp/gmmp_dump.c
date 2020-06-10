#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "gmmp_net.h"

void packet_dump(char *debug_title, int type, void *packet)
{
	GMMP_COMMON_HEADER *pgw_common_hdr;

	if(type == GW_CONTROL_REQUEST_SMS_PACKET_DUMP) {
		printf("++++ %s ++++\n", debug_title);
		GMMP_CONTROL_REQUEST_SMS_HDR *pgw_control_req;
		pgw_control_req = (GMMP_CONTROL_REQUEST_SMS_HDR *)packet;

#ifdef LITTLE_ENDIAN
		printf("tid : [%x]\n", htonl(pgw_control_req->omp_transaction_id));
#else
		printf("tid : [%x]\n", pgw_control_req->omp_transaction_id);
#endif
		printf("control_type : [%x]\n", pgw_control_req->control_type);
		printf("long sentence flag : [%x]\n", pgw_control_req->long_sentence_flag);
		printf("device_id : [%s]\n", pgw_control_req->device_id);
		printf("---- %s ----\n", debug_title);
		return;
	}
	
	printf("++++ %s ++++\n", debug_title);
	pgw_common_hdr=(GMMP_COMMON_HEADER *)packet;
	printf("version : [0x%02x]\n", pgw_common_hdr->version);
#ifdef LITTLE_ENDIAN
	printf("msg_length : [%d]\n", htons(pgw_common_hdr->msg_length));
#else
	printf("msg_length : [%d]\n", pgw_common_hdr->msg_length);
#endif
	printf("msg_type : [0x%02x]\n", pgw_common_hdr->msg_type);
#ifdef LITTLE_ENDIAN
	printf("time_stamp : [0x%08x]\n", htonl(pgw_common_hdr->time_stamp));
	printf("total_count : [0x%02x]\n", htons(pgw_common_hdr->total_count));
	printf("current_count : [0x%02x]\n", htons(pgw_common_hdr->current_count));
#else
	printf("time_stamp : [0x%08x]\n", pgw_common_hdr->time_stamp);
	printf("total_count : [0x%02x]\n", pgw_common_hdr->total_count);
	printf("current_count : [0x%02x]\n", pgw_common_hdr->current_count);
#endif
	printf("auth_id : [%s]\n", pgw_common_hdr->auth_id);
	printf("auth_key : [%s]\n", pgw_common_hdr->auth_key);
#ifdef LITTLE_ENDIAN
	printf("transaction_id : [0x%08x]\n", htonl(pgw_common_hdr->transaction_id));
#else
	printf("transaction_id : [0x%08x]\n", pgw_common_hdr->transaction_id);
#endif
	printf("reserved1 : [0x%02x]\n", pgw_common_hdr->reserved1);
	printf("reserved2 : [0x%02x]\n", pgw_common_hdr->reserved2);
	
	if(type == GW_REG_REQUEST_PACKET_DUMP) {
		GMMP_GW_REQUEST_HDR *pgw_reg_req_hdr;
		pgw_reg_req_hdr = (GMMP_GW_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_reg_req_hdr->domain_code);
		printf("manufacture_id : [%s]\n", pgw_reg_req_hdr->manufacture_id);

	} else if(type == GW_REG_RESPONSE_PACKET_DUMP) {
		GMMP_GW_RESPONSE_HDR *pgw_reg_resp_hdr;
		pgw_reg_resp_hdr = (GMMP_GW_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_reg_resp_hdr->domain_code);
		printf("gw_id : [%s]\n", pgw_reg_resp_hdr->gw_id);
		printf("result_code : [0x%02x]\n", pgw_reg_resp_hdr->result_code);
		
	} else if(type == GW_PROFILE_REQUEST_PACKET_DUMP) {
		GMMP_PROFILE_REQUEST_HDR *pgw_profile_req;
		pgw_profile_req = (GMMP_PROFILE_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_profile_req->domain_code);
		printf("gw_id : [%s]\n", pgw_profile_req->gw_id);
		printf("dev_id : [%s]\n", pgw_profile_req->device_id);
		
	} else if(type == GW_PROFILE_RESPONSE_PACKET_DUMP) {
		GMMP_PROFILE_RESPONSE_HDR *pgw_profile_res;
		pgw_profile_res = (GMMP_PROFILE_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_profile_res->domain_code);
		printf("gw_id : [%s]\n", pgw_profile_res->gw_id);
		printf("dev_id : [%s]\n", pgw_profile_res->device_id);
		printf("result code : [0x%02x]\n", pgw_profile_res->result_code);
		printf("heartbeat_period : [%d]\n", htonl(pgw_profile_res->heartbeat_period));

		printf("reporting_period : [%d]\n", htonl(pgw_profile_res->reporting_period));
		printf("report_offset : [%d]\n", htonl(pgw_profile_res->report_offset));
		printf("response_timeout : [%d]\n", htonl(pgw_profile_res->response_timeout));

		printf("model : [%s]\n", pgw_profile_res->model);
		printf("firmware_version : [%s]\n", pgw_profile_res->firmware_version);
		printf("software_version : [%s]\n", pgw_profile_res->software_version);
		printf("hardware_version : [%s]\n", pgw_profile_res->hardware_version);
	} else if(type == GW_DELIVERY_REQUEST_PACKET_DUMP) {
		GMMP_DELIVERY_REQUEST_HDR *pgw_delivery_req;
		pgw_delivery_req = (GMMP_DELIVERY_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_delivery_req->domain_code);
		printf("gw_id : [%s]\n", pgw_delivery_req->gw_id);
		printf("dev_id : [%s]\n", pgw_delivery_req->device_id);
		printf("report type : [0x%02x]\n", pgw_delivery_req->report_type);
		printf("media type : [0x%02x]\n", pgw_delivery_req->media_type);
		
		//printf("message body : [%s]\n", pgw_delivery_req->messsage_body);
	} else if(type == GW_DELIVERY_RESPONSE_PACKET_DUMP) {
		GMMP_DELIVERY_RESPONSE_HDR *pgw_delivery_res;
		pgw_delivery_res = (GMMP_DELIVERY_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_delivery_res->domain_code);
		printf("gw_id : [%s]\n", pgw_delivery_res->gw_id);
		printf("dev_id : [%s]\n", pgw_delivery_res->device_id);
		printf("result code : [0x%02x]\n", pgw_delivery_res->result_code);
		if(pgw_delivery_res->result_code == 0x08){
			printf("offset time : [%d]\n", htonl(pgw_delivery_res->offset_time));
		}
	} else if(type == GW_CONTROL_REQUEST_PACKET_DUMP) {
		GMMP_CONTROL_REQUEST_HDR *pgw_control_req;
		pgw_control_req = (GMMP_CONTROL_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_control_req->domain_code);
		printf("gw_id : [%s]\n", pgw_control_req->gw_id);
		printf("dev_id : [%s]\n", pgw_control_req->device_id);
		printf("control_type : [%x]\n", pgw_control_req->control_type);
	}else if(type == GW_CONTROL_RESPONSE_PACKET_DUMP) {
		GMMP_CONTROL_RESPONSE_HDR *pgw_contol_res;
		pgw_contol_res = (GMMP_CONTROL_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_contol_res->domain_code);
		printf("gw_id : [%s]\n", pgw_contol_res->gw_id);
		printf("dev_id : [%s]\n", pgw_contol_res->device_id);
		printf("control_type : [%x]\n", pgw_contol_res->control_type);
		printf("result_code : [%x]\n", pgw_contol_res->result_code);
	} else if(type == GW_FTP_REQUEST_PACKET_DUMP) {
		GMMP_FTP_INFO_REQUEST_HDR *pgw_ftp_req;
		pgw_ftp_req = (GMMP_FTP_INFO_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_ftp_req->domain_code);
		printf("gw_id : [%s]\n", pgw_ftp_req->gw_id);
		printf("dev_id : [%s]\n", pgw_ftp_req->device_id);
		printf("control_type : [%x]\n", pgw_ftp_req->control_type);
	} else if(type == GW_FTP_RESPONSE_PACKET_DUMP) {
		GMMP_FTP_INFO_RESPONSE_HDR *pgw_ftp_res;
		pgw_ftp_res = (GMMP_FTP_INFO_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_ftp_res->domain_code);
		printf("gw_id : [%s]\n", pgw_ftp_res->gw_id);
		printf("dev_id : [%s]\n", pgw_ftp_res->device_id);
		printf("control_type : [%x]\n", pgw_ftp_res->control_type);
		printf("result code : [0x%02x]\n", pgw_ftp_res->result_code);
		printf("ftp_server_ip : [%s]\n", pgw_ftp_res->ftp_server_ip);
		printf("ftp_server_port : [%d]\n", pgw_ftp_res->ftp_server_port);
		printf("ftp_server_id : [%s]\n", pgw_ftp_res->ftp_server_id);
		printf("ftp_server_password : [%s]\n", pgw_ftp_res->ftp_server_password);
		printf("file_path : [%s]\n", pgw_ftp_res->file_path);
		printf("file_name : [%s]\n", pgw_ftp_res->file_name);
		printf("file_hash : [%s]\n", pgw_ftp_res->file_hash);
		printf("file_size : [%d]\n", pgw_ftp_res->file_size);
	} else if(type == GW_HEARTBEAT_REQUEST_PACKET_DUMP) {
		GMMP_HEARTBEAT_REQUEST_HDR *pgw_heartbeat_req;
		pgw_heartbeat_req = (GMMP_HEARTBEAT_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_heartbeat_req->domain_code);
		printf("gw_id : [%s]\n", pgw_heartbeat_req->gw_id);
	} else if(type == GW_HEARTBEAT_RESPONSE_PACKET_DUMP) {
		GMMP_HEARTBEAT_RESPONSE_HDR *pgw_heartbeat_res;
		pgw_heartbeat_res = (GMMP_HEARTBEAT_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_heartbeat_res->domain_code);
		printf("gw_id : [%s]\n", pgw_heartbeat_res->gw_id);
	} else if(type == GW_DEREG_REQUEST_PACKET_DUMP) {
		GMMP_DEREGISTRATION_REQUEST_HDR *pgw_dereg_req;
		pgw_dereg_req = (GMMP_DEREGISTRATION_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_dereg_req->domain_code);
		printf("gw_id : [%s]\n", pgw_dereg_req->gw_id);
	} else if(type == GW_DEREG_RESPONSE_PACKET_DUMP) {
		GMMP_DEREGISTRATION_RESPONSE_HDR *pgw_dereg_res;
		pgw_dereg_res = (GMMP_DEREGISTRATION_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_dereg_res->domain_code);
		printf("gw_id : [%s]\n", pgw_dereg_res->gw_id);
		printf("result_code : [%x]\n", pgw_dereg_res->result_code);
	} else if(type == GW_DEREG_RESPONSE_PACKET_DUMP) {
		GMMP_DEREGISTRATION_RESPONSE_HDR *pgw_dereg_res;
		pgw_dereg_res = (GMMP_DEREGISTRATION_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_dereg_res->domain_code);
		printf("gw_id : [%s]\n", pgw_dereg_res->gw_id);
		printf("result_code : [%x]\n", pgw_dereg_res->result_code);
	} else if(type == GW_REMOTE_REQUEST_PACKET_DUMP) {
		GMMP_REMOTE_INFO_REQUEST_HDR *pgw_remote_req;
		pgw_remote_req = (GMMP_REMOTE_INFO_REQUEST_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_remote_req->domain_code);
		printf("gw_id : [%s]\n", pgw_remote_req->gw_id);
		printf("device_id : [%s]\n", pgw_remote_req->device_id);
		printf("control_type : [%x]\n", pgw_remote_req->control_type);
	} else if(type == GW_REMOTE_RESPONSE_PACKET_DUMP) {
		GMMP_REMOTE_INFO_RESPONSE_HDR *pgw_remote_res;
		pgw_remote_res = (GMMP_REMOTE_INFO_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_remote_res->domain_code);
		printf("gw_id : [%s]\n", pgw_remote_res->gw_id);
		printf("device_id : [%s]\n", pgw_remote_res->device_id);
		printf("control_type : [%x]\n", pgw_remote_res->control_type);
		printf("result_code : [%x]\n", pgw_remote_res->result_code);
		printf("remote_server_ip : [%s]\n", pgw_remote_res->remote_server_ip);
		printf("remote_server_port : [%u]\n", pgw_remote_res->remote_server_port);
		printf("remote_server_id : [%s]\n", pgw_remote_res->remote_server_id);
		printf("remote_server_password : [%s]\n", pgw_remote_res->remote_server_password);
		printf("accessible_start_time : [%x]\n", pgw_remote_res->accessible_start_time);
		printf("accessible_end_time : [%x]\n", pgw_remote_res->accessible_end_time);
	} else if(type == GW_CONTROL_NOTIFICATION_PACKET_DUMP) {
		GMMP_CONTROL_NOTIFICATION_HDR *pgw_control_noti;
		pgw_control_noti = (GMMP_CONTROL_NOTIFICATION_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_control_noti->domain_code);
		printf("gw_id : [%s]\n", pgw_control_noti->gw_id);
		printf("device_id : [%s]\n", pgw_control_noti->device_id);
		printf("control_type : [%x]\n", pgw_control_noti->control_type);
		printf("result_code : [%x]\n", pgw_control_noti->result_code);
		printf("message_body : [%02x%02x%02x%02x]\n",pgw_control_noti->message_body[0],pgw_control_noti->message_body[1],
			pgw_control_noti->message_body[2],pgw_control_noti->message_body[3]);
	} else if(type == GW_CONTROL_NOTIFICATION_RESPONSE_PACKET_DUMP) {
		GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR *pgw_control_noti_res;
		pgw_control_noti_res = (GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR *)packet;

		printf("domain_code : [%s]\n", pgw_control_noti_res->domain_code);
		printf("gw_id : [%s]\n", pgw_control_noti_res->gw_id);
		printf("device_id : [%s]\n", pgw_control_noti_res->device_id);
		printf("control_type : [%x]\n", pgw_control_noti_res->control_type);
		printf("result_code : [%x]\n", pgw_control_noti_res->result_code);
	}
	printf("---- %s ----\n\n\n", debug_title);
}
